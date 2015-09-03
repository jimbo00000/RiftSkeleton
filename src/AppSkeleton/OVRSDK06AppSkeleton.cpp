// OVRSDK06AppSkeleton.cpp
// Huge thanks to jherico
// https://github.com/jherico/OculusRiftInAction/blob/ea0231ad045187c8a5819a801ce4a2fae63301aa/examples/cpp/Example_SelfContained.cpp

#ifdef _WIN32
#  define WINDOWS_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#endif
#include <GL/glew.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <algorithm>
#include <iostream>
#include <fstream>

#include <OVR.h>

#include "OVRSDK06AppSkeleton.h"
#include "ShaderFunctions.h"
#include "MatrixFunctions.h"
#include "GLUtils.h"
#include "Logger.h"

#define USE_OVR_PERF_LOGGING

#ifndef PROJECT_NAME
// This macro should be defined in CMakeLists.txt
#define PROJECT_NAME "RiftSkeleton"
#endif

OVRSDK06AppSkeleton::OVRSDK06AppSkeleton()
: m_Hmd(NULL)
, m_pMirrorTex(NULL)
, m_frameIndex(0)
, m_usingDebugHmd(false)
{
    m_pTexSet[0] = NULL;
    m_pTexSet[1] = NULL;
}

OVRSDK06AppSkeleton::~OVRSDK06AppSkeleton()
{
}

void OVRSDK06AppSkeleton::RecenterPose()
{
    if (m_Hmd == NULL)
        return;
    ovrHmd_RecenterPose(m_Hmd);
}

void OVRSDK06AppSkeleton::exitVR()
{
    for (int i = 0; i < 2; ++i)
    {
        ovrHmd_DestroySwapTextureSet(m_Hmd, m_pTexSet[i]);
    }
    ovrHmd_Destroy(m_Hmd);
    ovr_Shutdown();
}

void OVRSDK06AppSkeleton::initHMD()
{
    ovrInitParams initParams = { 0 };
    if (ovrSuccess != ovr_Initialize(NULL))
    {
        LOG_INFO("Failed to initialize the Oculus SDK");
    }

    if (ovrSuccess != ovrHmd_Create(0, &m_Hmd))
    {
        LOG_INFO("Could not create HMD");
        if (ovrSuccess != ovrHmd_CreateDebug(ovrHmd_DK2, &m_Hmd))
        {
            LOG_ERROR("Could not create Debug HMD");
        }
        m_usingDebugHmd = true;
    }

    const ovrBool ret = ovrHmd_ConfigureTracking(m_Hmd,
        ovrTrackingCap_Orientation | ovrTrackingCap_MagYawCorrection | ovrTrackingCap_Position,
        ovrTrackingCap_Orientation);
    if (!OVR_SUCCESS(ret))
    {
        LOG_ERROR("Error calling ovrHmd_ConfigureTracking");
    }
}

///@brief Called once a GL context has been set up.
void OVRSDK06AppSkeleton::initVR(bool swapBackBufferDims)
{
    if (m_Hmd == NULL)
        return;

    for (int i = 0; i < 2; ++i)
    {
        if (m_pTexSet[i])
        {
            ovrHmd_DestroySwapTextureSet(m_Hmd, m_pTexSet[i]);
            m_pTexSet[i] = nullptr;
        }
    }

    // Set up eye fields of view
    ovrLayerEyeFov& layer = m_layerEyeFov;
    layer.Header.Type = ovrLayerType_EyeFov;
    layer.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;

    // Create eye render target textures and FBOs
    for (ovrEyeType eye = ovrEyeType::ovrEye_Left;
        eye < ovrEyeType::ovrEye_Count;
        eye = static_cast<ovrEyeType>(eye + 1))
    {
        const ovrFovPort& fov = layer.Fov[eye] = m_Hmd->MaxEyeFov[eye];
        const ovrSizei& size = layer.Viewport[eye].Size = ovrHmd_GetFovTextureSize(m_Hmd, eye, fov, 1.f);
        layer.Viewport[eye].Pos = { 0, 0 };
        LOG_INFO("Eye %d tex : %dx%d @ (%d,%d)", eye, size.w, size.h,
            layer.Viewport[eye].Pos.x, layer.Viewport[eye].Pos.y);

        ovrEyeRenderDesc & erd = m_eyeRenderDescs[eye];
        erd = ovrHmd_GetRenderDesc(m_Hmd, eye, m_Hmd->MaxEyeFov[eye]);
        ovrMatrix4f ovrPerspectiveProjection =
            ovrMatrix4f_Projection(erd.Fov, .1f, 10000.f, ovrProjection_RightHanded);
        m_eyeProjections[eye] = glm::transpose(glm::make_mat4(&ovrPerspectiveProjection.M[0][0]));
        m_eyeOffsets[eye] = erd.HmdToEyeViewOffset;

        // Allocate the frameBuffer that will hold the scene, and then be
        // re-rendered to the screen with distortion
        if (!OVR_SUCCESS(ovrHmd_CreateSwapTextureSetGL(m_Hmd, GL_RGBA, size.w, size.h, &m_pTexSet[eye])))
        {
            LOG_ERROR("Unable to create swap textures");
            return;
        }
        ovrSwapTextureSet& swapSet = *m_pTexSet[eye];
        for (int i = 0; i < swapSet.TextureCount; ++i)
        {
            const ovrGLTexture& ovrTex = (ovrGLTexture&)swapSet.Textures[i];
            glBindTexture(GL_TEXTURE_2D, ovrTex.OGL.TexId);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }

        // Manually assemble swap FBO
        m_swapFBO.w = size.w;
        m_swapFBO.h = size.h;
        glGenFramebuffers(1, &m_swapFBO.id);
        glBindFramebuffer(GL_FRAMEBUFFER, m_swapFBO.id);
        const int idx = 0;
        const ovrGLTextureData* pGLData = reinterpret_cast<ovrGLTextureData*>(&swapSet.Textures[idx]);
        m_swapFBO.tex = pGLData->TexId;
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_swapFBO.tex, 0);

        m_swapFBO.depth = 0;
        glGenRenderbuffers(1, &m_swapFBO.depth);
        glBindRenderbuffer(GL_RENDERBUFFER, m_swapFBO.depth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, size.w, size.h);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_swapFBO.depth);

        // Check status
        {
            const GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
            if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
            {
                LOG_ERROR("Framebuffer status incomplete: %d %x", status, status);
            }
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        layer.ColorTexture[eye] = m_pTexSet[eye];
    }

    // Mirror texture for displaying to desktop window
    if (m_pMirrorTex)
    {
        ovrHmd_DestroyMirrorTexture(m_Hmd, m_pMirrorTex);
    }

    const ovrEyeType eye = ovrEyeType::ovrEye_Left;
    const ovrFovPort& fov = layer.Fov[eye] = m_Hmd->MaxEyeFov[eye];
    const ovrSizei& size = layer.Viewport[eye].Size = ovrHmd_GetFovTextureSize(m_Hmd, eye, fov, 1.f);
    ovrResult result = ovrHmd_CreateMirrorTextureGL(m_Hmd, GL_RGBA, size.w, size.h, &m_pMirrorTex);
    if (!OVR_SUCCESS(result))
    {
        LOG_ERROR("Unable to create mirror texture");
        return;
    }

    // Manually assemble mirror FBO
    m_mirrorFBO.w = size.w;
    m_mirrorFBO.h = size.h;
    glGenFramebuffers(1, &m_mirrorFBO.id);
    glBindFramebuffer(GL_FRAMEBUFFER, m_mirrorFBO.id);
    const ovrGLTextureData* pMirrorGLData = reinterpret_cast<ovrGLTextureData*>(m_pMirrorTex);
    m_mirrorFBO.tex = pMirrorGLData->TexId;
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_mirrorFBO.tex, 0);

    // Check status
    {
        const GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
        if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
        {
            LOG_ERROR("Framebuffer status incomplete: %d %x", status, status);
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glEnable(GL_DEPTH_TEST);
}

ovrSizei OVRSDK06AppSkeleton::getHmdResolution() const
{
    if (m_Hmd == NULL)
    {
        return { 800, 600 };
    }
    return m_Hmd->Resolution;
}

void OVRSDK06AppSkeleton::display_sdk() const
{
    ovrHmd hmd = m_Hmd;
    if (hmd == NULL)
        return;

    ovrTrackingState outHmdTrackingState = { 0 };
    ovrHmd_GetEyePoses(m_Hmd, m_frameIndex, m_eyeOffsets,
        m_eyePoses, &outHmdTrackingState);

    for (ovrEyeType eye = ovrEyeType::ovrEye_Left;
        eye < ovrEyeType::ovrEye_Count;
        eye = static_cast<ovrEyeType>(eye + 1))
    {
        const ovrSwapTextureSet& swapSet = *m_pTexSet[eye];
        glBindFramebuffer(GL_FRAMEBUFFER, m_swapFBO.id);
        ovrGLTexture& tex = (ovrGLTexture&)(swapSet.Textures[swapSet.CurrentIndex]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex.OGL.TexId, 0);
        {
            // Handle render target resolution scaling
            m_layerEyeFov.Viewport[eye].Size = ovrHmd_GetFovTextureSize(m_Hmd, eye, m_layerEyeFov.Fov[eye], m_fboScale);
            ovrRecti& vp = m_layerEyeFov.Viewport[eye];
            if (m_layerEyeFov.Header.Flags & ovrLayerFlag_TextureOriginAtBottomLeft)
            {
                ///@note It seems that the render viewport should be vertically centered within the swapSet texture.
                /// See also OculusWorldDemo.cpp:1443 - "The usual OpenGL viewports-don't-match-UVs oddness."
                const int texh = swapSet.Textures[swapSet.CurrentIndex].Header.TextureSize.h;
                vp.Pos.y = (texh - vp.Size.h) / 2;
            }

            glViewport(vp.Pos.x, vp.Pos.y, vp.Size.w, vp.Size.h);

            glClearColor(0.f, 0.f, 0.f, 0.f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Render the scene for the current eye
            const ovrPosef& eyePose = m_eyePoses[eye];
            const glm::mat4 viewLocal = makeMatrixFromPose(eyePose);
            const glm::mat4 viewWorld = makeWorldToChassisMatrix() * viewLocal;
            const glm::mat4& proj = m_eyeProjections[eye];
            _DrawScenes(
                glm::value_ptr(glm::inverse(viewWorld)),
                glm::value_ptr(proj),
                glm::value_ptr(glm::inverse(viewLocal)));

            m_layerEyeFov.RenderPose[eye] = eyePose;
        }
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    ovrLayerEyeFov& layer = m_layerEyeFov;
    ovrLayerHeader* layers = &layer.Header;
    ovrResult result = ovrHmd_SubmitFrame(hmd, m_frameIndex, NULL, &layers, 1);

    // Increment counters in swap texture set
    for (ovrEyeType eye = ovrEyeType::ovrEye_Left;
        eye < ovrEyeType::ovrEye_Count;
        eye = static_cast<ovrEyeType>(eye + 1))
    {
        ovrSwapTextureSet& swapSet = *m_pTexSet[eye];
        ++swapSet.CurrentIndex %= swapSet.TextureCount;
    }

    // Blit output to main app window to show something on screen in addition
    // to what's in the Rift. This could optionally be the distorted texture
    // from the OVR SDK's mirror texture, or perhaps a single eye's undistorted
    // view, or even a third-person render(at a performance cost).
    if (true)
    {
        glViewport(0, 0, m_appWindowSize.w, m_appWindowSize.h);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_mirrorFBO.id);
        glBlitFramebuffer(
            0, m_mirrorFBO.h, m_mirrorFBO.w, 0,
            0, 0, m_appWindowSize.w, m_appWindowSize.h,
            GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    }

    ++m_frameIndex;
}
