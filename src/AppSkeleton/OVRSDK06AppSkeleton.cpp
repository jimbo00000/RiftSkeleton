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
, m_pTexSet(NULL)
, m_pMirrorTex(NULL)
{
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
    //deallocateFBO(m_renderBuffer);
    ovrHmd_DestroySwapTextureSet(m_Hmd, m_pTexSet);
    ovrHmd_Destroy(m_Hmd);
    ovr_Shutdown();
}

void OVRSDK06AppSkeleton::initHMD()
{
    ovrInitParams initParams; memset(&initParams, 0, sizeof(ovrInitParams));
    if (ovrSuccess != ovr_Initialize(NULL))
    {
        LOG_INFO("Failed to initialize the Oculus SDK");
    }

    if (ovrSuccess != ovrHmd_Create(0, &m_Hmd))
    {
        if (ovrSuccess != ovrHmd_CreateDebug(ovrHmd_DK2, &m_Hmd)) {
            LOG_INFO("Could not create HMD");
        }
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

    if (m_pTexSet)
    {
        ovrHmd_DestroySwapTextureSet(m_Hmd, m_pTexSet);
        m_pTexSet = nullptr;
    }

    // Set up eye fields of view
    ovrLayerEyeFov layer = m_layerEyeFov;
    layer.Header.Type = ovrLayerType_EyeFov;
    layer.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;

    // Create swap set of optimal size based on resolution and FOV info from headset
    const ovrEyeType eye = ovrEyeType::ovrEye_Left;
    const ovrFovPort& fov = layer.Fov[eye] = m_Hmd->MaxEyeFov[eye];
    const ovrSizei& size = layer.Viewport[eye].Size = ovrHmd_GetFovTextureSize(m_Hmd, eye, fov, 1.f);
    LOG_INFO("FOV Texture requested size: %d x %d", size.w, size.h);

    if (!OVR_SUCCESS(ovrHmd_CreateSwapTextureSetGL(m_Hmd, GL_RGBA, size.w, size.h, &m_pTexSet)))
    {
        LOG_ERROR("Unable to create swap textures");
        return;
    }
    LOG_INFO("Swap textures created: %d textures", m_pTexSet->TextureCount);
    for (int i = 0; i < m_pTexSet->TextureCount; ++i)
    {
        const ovrGLTextureData* pGLData = reinterpret_cast<ovrGLTextureData*>(&m_pTexSet->Textures[i]);
        LOG_INFO("Swap tex[%d] = %d", i, pGLData->TexId);
    }

    if (m_pMirrorTex)
    {
        ovrHmd_DestroyMirrorTexture(m_Hmd, m_pMirrorTex);
    }
    ovrResult result = ovrHmd_CreateMirrorTextureGL(m_Hmd, GL_RGBA, size.w, size.h, &m_pMirrorTex);
    if (!OVR_SUCCESS(result))
    {
        LOG_ERROR("Unable to create mirror texture");
        return;
    }
    const ovrGLTextureData* pGLData = reinterpret_cast<ovrGLTextureData*>(m_pMirrorTex);
    LOG_INFO("Mirror texture created: %d.", pGLData->TexId);

#if 0
    for (int i = 0; i < color->TextureCount; ++i)
    {
        ovrGLTexture& ovrTex = (ovrGLTexture&)color->Textures[i];
        glBindTexture(GL_TEXTURE_2D, ovrTex.OGL.TexId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
#endif

    glBindTexture(GL_TEXTURE_2D, 0);

    for (ovrEyeType eye = ovrEyeType::ovrEye_Left;
        eye < ovrEyeType::ovrEye_Count;
        eye = static_cast<ovrEyeType>(eye + 1))
    {
        const ovrFovPort& fov = layer.Fov[eye] = m_Hmd->MaxEyeFov[eye];
        const ovrSizei& size = layer.Viewport[eye].Size = ovrHmd_GetFovTextureSize(m_Hmd, eye, fov, 1.f);
        layer.Viewport[eye].Pos = { 0, 0 };
        LOG_INFO("Eye %d tex size: %dx%d", eye, size.w, size.h);

        ovrEyeRenderDesc & erd = m_eyeRenderDescs[eye];
        erd = ovrHmd_GetRenderDesc(m_Hmd, eye, m_Hmd->MaxEyeFov[eye]);
        ovrMatrix4f ovrPerspectiveProjection =
            ovrMatrix4f_Projection(erd.Fov, .1f, 10000.f, ovrProjection_RightHanded);
        //projections[eye] = ovr::toGlm(ovrPerspectiveProjection);
        //eyeOffsets[eye] = erd.HmdToEyeViewOffset;

        // Allocate the frameBuffer that will hold the scene, and then be
        // re-rendered to the screen with distortion
        //eyeFbos[eye] = SwapTexFboPtr(new SwapTextureFramebufferWrapper(hmd));
        //eyeFbos[eye]->Init(ovr::toGlm(size));
        //layer.ColorTexture[eye] = eyeFbos[eye]->color;
    }
}

ovrSizei OVRSDK06AppSkeleton::getHmdResolution() const
{
    if (m_Hmd == NULL)
    {
        return { 800, 600 };
    }
    return m_Hmd->Resolution;
}
