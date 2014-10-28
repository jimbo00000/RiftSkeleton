// RiftAppSkeleton.cpp

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

#include <OVR.h>

#include "RiftAppSkeleton.h"
#include "ShaderFunctions.h"
#include "GLUtils.h"

RiftAppSkeleton::RiftAppSkeleton()
: m_Hmd(NULL)
, m_usingDebugHmd(false)
{
    m_eyeOri = OVR::Quatf();
}

RiftAppSkeleton::~RiftAppSkeleton()
{
}

void RiftAppSkeleton::RecenterPose()
{
    if (m_Hmd == NULL)
        return;
    ovrHmd_RecenterPose(m_Hmd);
}

ovrSizei RiftAppSkeleton::getHmdResolution() const
{
    if (m_Hmd == NULL)
    {
        ovrSizei empty = {0, 0};
        return empty;
    }
    return m_Hmd->Resolution;
}

ovrVector2i RiftAppSkeleton::getHmdWindowPos() const
{
    if (m_Hmd == NULL)
    {
        ovrVector2i empty = {0, 0};
        return empty;
    }
    return m_Hmd->WindowsPos;
}

glm::ivec2 RiftAppSkeleton::getRTSize() const
{
    const ovrSizei& sz = m_Cfg.OGL.Header.RTSize;
    return glm::ivec2(sz.w, sz.h);
}

glm::mat4 RiftAppSkeleton::getUserViewMatrix() const
{
    const OVR::Matrix4f rotmtx = 
          OVR::Matrix4f::RotationY(-m_chassisYaw)
        * OVR::Matrix4f(m_eyeOri);

    return glm::make_mat4(&rotmtx.Transposed().M[0][0]);
}

///@brief Set this up early so we can get the HMD's display dimensions to create a window.
void RiftAppSkeleton::initHMD()
{
    ovr_Initialize();

    m_Hmd = ovrHmd_Create(0);
    if (m_Hmd == NULL)
    {
        m_Hmd = ovrHmd_CreateDebug(ovrHmd_DK1);
        m_usingDebugHmd = true;
    }

    m_ovrScene.SetHmdPointer(m_Hmd);
}

void RiftAppSkeleton::initVR()
{
    if (m_Hmd != NULL)
    {
        const ovrBool ret = ovrHmd_ConfigureTracking(m_Hmd,
            ovrTrackingCap_Orientation | ovrTrackingCap_MagYawCorrection | ovrTrackingCap_Position,
            ovrTrackingCap_Orientation);
        if (ret == 0)
        {
            std::cerr << "Error calling ovrHmd_ConfigureTracking." << std::endl;
        }
    }

    m_Cfg.OGL.Header.RTSize = getHmdResolution();

    ///@todo Do we need to choose here?
    ConfigureSDKRendering();
    ConfigureClientRendering();

    _initPresentDistMesh(m_presentDistMeshL, 0);
    _initPresentDistMesh(m_presentDistMeshR, 1);
}

void RiftAppSkeleton::_initPresentDistMesh(ShaderWithVariables& shader, int eyeIdx)
{
    // Init left and right VAOs separately
    shader.bindVAO();

    ovrDistortionMesh& mesh = m_DistMeshes[eyeIdx];
    GLuint vertVbo = 0;
    glGenBuffers(1, &vertVbo);
    shader.AddVbo("vPosition", vertVbo);
    glBindBuffer(GL_ARRAY_BUFFER, vertVbo);
    glBufferData(GL_ARRAY_BUFFER, mesh.VertexCount * sizeof(ovrDistortionVertex), &mesh.pVertexData[0].ScreenPosNDC.x, GL_STATIC_DRAW);

    const int a_pos = shader.GetAttrLoc("vPosition");
    glVertexAttribPointer(a_pos, 4, GL_FLOAT, GL_FALSE, sizeof(ovrDistortionVertex), NULL);
    glEnableVertexAttribArray(a_pos);

    const int a_texR = shader.GetAttrLoc("vTexR");
    if (a_texR > -1)
    {
        glVertexAttribPointer(a_texR, 2, GL_FLOAT, GL_FALSE, sizeof(ovrDistortionVertex),
            reinterpret_cast<void*>(offsetof(ovrDistortionVertex, TanEyeAnglesR)));
        glEnableVertexAttribArray(a_texR);
    }

    const int a_texG = shader.GetAttrLoc("vTexG");
    if (a_texG > -1)
    {
        glVertexAttribPointer(a_texG, 2, GL_FLOAT, GL_FALSE, sizeof(ovrDistortionVertex),
            reinterpret_cast<void*>(offsetof(ovrDistortionVertex, TanEyeAnglesG)));
        glEnableVertexAttribArray(a_texG);
    }

    const int a_texB = shader.GetAttrLoc("vTexB");
    if (a_texB > -1)
    {
        glVertexAttribPointer(a_texB, 2, GL_FLOAT, GL_FALSE, sizeof(ovrDistortionVertex),
            reinterpret_cast<void*>(offsetof(ovrDistortionVertex, TanEyeAnglesB)));
        glEnableVertexAttribArray(a_texB);
    }


    GLuint elementVbo = 0;
    glGenBuffers(1, &elementVbo);
    shader.AddVbo("elements", elementVbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementVbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.IndexCount * sizeof(GLushort), &mesh.pIndexData[0], GL_STATIC_DRAW);

    // We have copies of the mesh in GL, but keep a count of indices around for the GL draw call.
    const unsigned int tmp = mesh.IndexCount;
    ovrHmd_DestroyDistortionMesh(&mesh);
    mesh.IndexCount = tmp;

    glBindVertexArray(0);
}

void RiftAppSkeleton::exitVR()
{
    deallocateFBO(m_renderBuffer);
    ovrHmd_Destroy(m_Hmd);
    ovr_Shutdown();
}

// Active GL context is required for the following
int RiftAppSkeleton::ConfigureSDKRendering()
{
    if (m_Hmd == NULL)
        return 1;
    ovrSizei l_TextureSizeLeft = ovrHmd_GetFovTextureSize(m_Hmd, ovrEye_Left, m_Hmd->DefaultEyeFov[0], 1.0f);
    ovrSizei l_TextureSizeRight = ovrHmd_GetFovTextureSize(m_Hmd, ovrEye_Right, m_Hmd->DefaultEyeFov[1], 1.0f);
    ovrSizei l_TextureSize;
    l_TextureSize.w = l_TextureSizeLeft.w + l_TextureSizeRight.w;
    l_TextureSize.h = (l_TextureSizeLeft.h>l_TextureSizeRight.h ? l_TextureSizeLeft.h : l_TextureSizeRight.h);

    m_EyeTexture[0].OGL.Header.API = ovrRenderAPI_OpenGL;
    m_EyeTexture[0].OGL.Header.TextureSize.w = l_TextureSize.w;
    m_EyeTexture[0].OGL.Header.TextureSize.h = l_TextureSize.h;
    m_EyeTexture[0].OGL.Header.RenderViewport.Pos.x = 0;
    m_EyeTexture[0].OGL.Header.RenderViewport.Pos.y = 0;
    m_EyeTexture[0].OGL.Header.RenderViewport.Size.w = l_TextureSize.w/2;
    m_EyeTexture[0].OGL.Header.RenderViewport.Size.h = l_TextureSize.h;
    m_EyeTexture[0].OGL.TexId = m_renderBuffer.tex;

    // Right eye the same, except for the x-position in the texture.
    m_EyeTexture[1] = m_EyeTexture[0];
    m_EyeTexture[1].OGL.Header.RenderViewport.Pos.x = (l_TextureSize.w+1) / 2;


    // Oculus Rift eye configurations...
    m_EyeFov[0] = m_Hmd->DefaultEyeFov[0];
    m_EyeFov[1] = m_Hmd->DefaultEyeFov[1];

    m_Cfg.OGL.Header.API = ovrRenderAPI_OpenGL;
    m_Cfg.OGL.Header.Multisample = 0;



    const int distortionCaps =
        ovrDistortionCap_Chromatic |
        ovrDistortionCap_TimeWarp;
    ovrHmd_ConfigureRendering(m_Hmd, &m_Cfg.Config, distortionCaps, m_EyeFov, m_EyeRenderDesc);

    return 0;
}

int RiftAppSkeleton::ConfigureClientRendering()
{
    if (m_Hmd == NULL)
        return 1;
    ovrSizei l_TextureSizeLeft = ovrHmd_GetFovTextureSize(m_Hmd, ovrEye_Left, m_Hmd->DefaultEyeFov[0], 1.0f);
    ovrSizei l_TextureSizeRight = ovrHmd_GetFovTextureSize(m_Hmd, ovrEye_Right, m_Hmd->DefaultEyeFov[1], 1.0f);
    ovrSizei l_TextureSize;
    l_TextureSize.w = l_TextureSizeLeft.w + l_TextureSizeRight.w;
    l_TextureSize.h = std::max(l_TextureSizeLeft.h, l_TextureSizeRight.h);

    m_EyeTexture[0].OGL.Header.API = ovrRenderAPI_OpenGL;
    m_EyeTexture[0].OGL.Header.TextureSize.w = l_TextureSize.w;
    m_EyeTexture[0].OGL.Header.TextureSize.h = l_TextureSize.h;
    m_EyeTexture[0].OGL.Header.RenderViewport.Pos.x = 0;
    m_EyeTexture[0].OGL.Header.RenderViewport.Pos.y = 0;
    m_EyeTexture[0].OGL.Header.RenderViewport.Size.w = l_TextureSize.w/2;
    m_EyeTexture[0].OGL.Header.RenderViewport.Size.h = l_TextureSize.h;
    m_EyeTexture[0].OGL.TexId = m_renderBuffer.tex;

    // Right eye the same, except for the x-position in the texture.
    m_EyeTexture[1] = m_EyeTexture[0];
    m_EyeTexture[1].OGL.Header.RenderViewport.Pos.x = (l_TextureSize.w+1) / 2;



    // Renderbuffer init - we can use smaller subsets of it easily
    deallocateFBO(m_renderBuffer);
    allocateFBO(m_renderBuffer, l_TextureSize.w, l_TextureSize.h);

    m_RenderViewports[0] = m_EyeTexture[0].OGL.Header.RenderViewport;
    m_RenderViewports[1] = m_EyeTexture[1].OGL.Header.RenderViewport;

    const int distortionCaps =
        ovrDistortionCap_Chromatic |
        ovrDistortionCap_TimeWarp |
        ovrDistortionCap_Vignette;

    // Generate distortion mesh for each eye
    for (int eyeNum = 0; eyeNum < 2; eyeNum++)
    {
        // Allocate & generate distortion mesh vertices.
        ovrHmd_CreateDistortionMesh(
            m_Hmd,
            m_EyeRenderDesc[eyeNum].Eye,
            m_EyeRenderDesc[eyeNum].Fov,
            distortionCaps,
            &m_DistMeshes[eyeNum]);
    }
    return 0;
}

///@brief The HSW will be displayed by default when using SDK rendering.
void RiftAppSkeleton::DismissHealthAndSafetyWarning() const
{
    ovrHSWDisplayState hswDisplayState;
    ovrHmd_GetHSWDisplayState(m_Hmd, &hswDisplayState);
    if (hswDisplayState.Displayed)
    {
        ovrHmd_DismissHSWDisplay(m_Hmd);
    }
}

///@brief The HSW will be displayed by default when using SDK rendering.
/// This function will detect a moderate tap on the Rift via the accelerometer
/// and dismiss the warning.
void RiftAppSkeleton::CheckForTapToDismissHealthAndSafetyWarning() const
{
    // Health and Safety Warning display state.
    ovrHSWDisplayState hswDisplayState;
    ovrHmd_GetHSWDisplayState(m_Hmd, &hswDisplayState);
    if (hswDisplayState.Displayed)
    {
        // Detect a moderate tap on the side of the HMD.
        const ovrTrackingState ts = ovrHmd_GetTrackingState(m_Hmd, ovr_GetTimeInSeconds());
        if (ts.StatusFlags & ovrStatus_OrientationTracked)
        {
            const OVR::Vector3f v(ts.RawSensorData.Accelerometer.x,
                ts.RawSensorData.Accelerometer.y,
                ts.RawSensorData.Accelerometer.z);
            // Arbitrary value and representing moderate tap on the side of the DK2 Rift.
            if (v.LengthSq() > 250.f)
            {
                ovrHmd_DismissHSWDisplay(m_Hmd);
            }
        }
    }
}

///@todo Even though this function shares most of its code with client rendering,
/// which appears to work fine, it is non-convergable. It appears that the projection
/// matrices for each eye are too far apart? Could be modelview...
void RiftAppSkeleton::display_stereo_undistorted() const
{
    ovrHmd hmd = m_Hmd;
    if (hmd == NULL)
        return;

    //ovrFrameTiming hmdFrameTiming =
    ovrHmd_BeginFrameTiming(hmd, 0);

    bindFBO(m_renderBuffer, m_fboScale);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (int eyeIndex = 0; eyeIndex < ovrEye_Count; eyeIndex++)
    {
        ovrEyeType eye = hmd->EyeRenderOrder[eyeIndex];
        ovrPosef eyePose = ovrHmd_GetHmdPosePerEye(hmd, eye);

        const ovrGLTexture& otex = m_EyeTexture[eye];
        const ovrRecti& rvp = otex.OGL.Header.RenderViewport;
        glViewport(
            static_cast<int>(m_fboScale * rvp.Pos.x),
            static_cast<int>(m_fboScale * rvp.Pos.y),
            static_cast<int>(m_fboScale * rvp.Size.w),
            static_cast<int>(m_fboScale * rvp.Size.h));

        OVR::Quatf orientation = OVR::Quatf(eyePose.Orientation);
        OVR::Matrix4f proj = ovrMatrix4f_Projection(
            m_EyeRenderDesc[eye].Fov,
            0.01f, 10000.0f, true);

        //m_EyeRenderDesc[eye].DistortedViewport;
        OVR::Vector3f EyePos = OVR::Vector3f(m_chassisPos.x, m_chassisPos.y, m_chassisPos.z);
        OVR::Matrix4f view = OVR::Matrix4f(orientation.Inverted())
            * OVR::Matrix4f::RotationY(m_chassisYaw)
            * OVR::Matrix4f::Translation(-EyePos);
        OVR::Matrix4f eyeview = OVR::Matrix4f::Translation(m_EyeRenderDesc[eye].HmdToEyeViewOffset) * view;

        _resetGLState();

        _DrawScenes(&eyeview.Transposed().M[0][0], &proj.Transposed().M[0][0]);
    }
    unbindFBO();

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // Present FBO to screen
    const GLuint prog = m_presentFbo.prog();
    glUseProgram(prog);
    m_presentFbo.bindVAO();
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_renderBuffer.tex);
        glUniform1i(m_presentFbo.GetUniLoc("fboTex"), 0);

        // This is the only uniform that changes per-frame
        glUniform1f(m_presentFbo.GetUniLoc("fboScale"), m_fboScale);

        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }
    glBindVertexArray(0);
    glUseProgram(0);

    ovrHmd_EndFrameTiming(hmd);
}

void RiftAppSkeleton::display_sdk() const
{
    ovrHmd hmd = m_Hmd;
    if (hmd == NULL)
        return;

    //const ovrFrameTiming hmdFrameTiming =
    ovrHmd_BeginFrame(m_Hmd, 0);

    bindFBO(m_renderBuffer);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // For passing to EndFrame once rendering is done
    ovrPosef renderPose[2];
    ovrTexture eyeTexture[2];

    for (int eyeIndex=0; eyeIndex<ovrEye_Count; eyeIndex++)
    {
        const ovrEyeType eye = hmd->EyeRenderOrder[eyeIndex];
        const ovrPosef eyePose = ovrHmd_GetHmdPosePerEye(m_Hmd, eye);
        m_eyeOri = eyePose.Orientation; // cache this for movement direction

        const ovrGLTexture& otex = m_EyeTexture[eye];
        const ovrRecti& rvp = otex.OGL.Header.RenderViewport;
        glViewport(
            rvp.Pos.x,
            rvp.Pos.y,
            rvp.Size.w,
            rvp.Size.h
            );

        // Get Projection and ModelView matrici from the device...
        const OVR::Matrix4f l_ProjectionMatrix = ovrMatrix4f_Projection(
            m_EyeRenderDesc[eye].Fov, 0.3f, 100.0f, true);

        const OVR::Matrix4f eyePoseMatrix =
            OVR::Matrix4f::Translation(eyePose.Position)
            * OVR::Matrix4f(OVR::Quatf(eyePose.Orientation));

        const OVR::Matrix4f l_ModelViewMatrix =
            OVR::Matrix4f::Translation(-OVR::Vector3f(m_EyeRenderDesc[eye].HmdToEyeViewOffset)) // not sure why negative...
            * eyePoseMatrix.Inverted()
            * OVR::Matrix4f::RotationY(m_chassisYaw)
            * OVR::Matrix4f::Translation(-OVR::Vector3f(m_chassisPos.x, m_chassisPos.y, m_chassisPos.z));

        _resetGLState();

        _DrawScenes(&l_ModelViewMatrix.Transposed().M[0][0], &l_ProjectionMatrix.Transposed().M[0][0]);

        renderPose[eyeIndex] = eyePose;
        eyeTexture[eyeIndex] = m_EyeTexture[eye].Texture;
    }
    unbindFBO();

    ovrHmd_EndFrame(m_Hmd, renderPose, eyeTexture);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
}

void RiftAppSkeleton::display_client() const
{
    const ovrHmd hmd = m_Hmd;
    if (hmd == NULL)
        return;

    //ovrFrameTiming hmdFrameTiming =
    ovrHmd_BeginFrameTiming(hmd, 0);

    bindFBO(m_renderBuffer, m_fboScale);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (int eyeIndex = 0; eyeIndex < ovrEye_Count; eyeIndex++)
    {
        const ovrEyeType eye = hmd->EyeRenderOrder[eyeIndex];
        const ovrPosef eyePose = ovrHmd_GetHmdPosePerEye(hmd, eye);
        m_eyeOri = eyePose.Orientation; // cache this for movement direction

        const ovrGLTexture& otex = m_EyeTexture[eye];
        const ovrRecti& rvp = otex.OGL.Header.RenderViewport;
        glViewport(
            static_cast<int>(m_fboScale * rvp.Pos.x),
            static_cast<int>(m_fboScale * rvp.Pos.y),
            static_cast<int>(m_fboScale * rvp.Size.w),
            static_cast<int>(m_fboScale * rvp.Size.h));

        const OVR::Matrix4f proj = ovrMatrix4f_Projection(
            m_EyeRenderDesc[eye].Fov,
            0.01f, 10000.0f, true);

        ///@todo Should we be using this variable?
        //m_EyeRenderDesc[eye].DistortedViewport;

        const OVR::Matrix4f eyePoseMatrix =
            OVR::Matrix4f::Translation(eyePose.Position)
            * OVR::Matrix4f(OVR::Quatf(eyePose.Orientation));

        const OVR::Matrix4f view =
            OVR::Matrix4f::Translation(m_EyeRenderDesc[eye].HmdToEyeViewOffset)
            * eyePoseMatrix.Inverted()
            * OVR::Matrix4f::RotationY(m_chassisYaw)
            * OVR::Matrix4f::Translation(-OVR::Vector3f(m_chassisPos.x, m_chassisPos.y, m_chassisPos.z));

        _resetGLState();

        _DrawScenes(&view.Transposed().M[0][0], &proj.Transposed().M[0][0]);
    }
    unbindFBO();


    const glm::ivec2 vp = getRTSize();
    glViewport(0, 0, vp.x, vp.y);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // Now draw the distortion mesh...
    for(int eyeNum = 0; eyeNum < 2; eyeNum++)
    {
        const ShaderWithVariables& eyeShader = eyeNum == 0 ?
            m_presentDistMeshL :
            m_presentDistMeshR;
        const GLuint prog = eyeShader.prog();
        glUseProgram(prog);
        eyeShader.bindVAO();
        {
            const ovrDistortionMesh& mesh = m_DistMeshes[eyeNum];

            ovrVector2f uvScaleOffsetOut[2];
            ovrHmd_GetRenderScaleAndOffset(
                m_EyeFov[eyeNum],
                m_EyeTexture[eyeNum].Texture.Header.TextureSize,
                m_EyeTexture[eyeNum].OGL.Header.RenderViewport,
                uvScaleOffsetOut );

            const ovrVector2f uvscale = uvScaleOffsetOut[0];
            const ovrVector2f uvoff = uvScaleOffsetOut[1];

            glUniform2f(eyeShader.GetUniLoc("EyeToSourceUVOffset"), uvoff.x, uvoff.y);
            glUniform2f(eyeShader.GetUniLoc("EyeToSourceUVScale"), uvscale.x, uvscale.y);


#if 0
            // Setup shader constants
            DistortionData.Shaders->SetUniform2f(
                "EyeToSourceUVScale",
                DistortionData.UVScaleOffset[eyeNum][0].x,
                DistortionData.UVScaleOffset[eyeNum][0].y);
            DistortionData.Shaders->SetUniform2f(
                "EyeToSourceUVOffset",
                DistortionData.UVScaleOffset[eyeNum][1].x,
                DistortionData.UVScaleOffset[eyeNum][1].y);

            if (distortionCaps & ovrDistortionCap_TimeWarp)
            { // TIMEWARP - Additional shader constants required
                ovrMatrix4f timeWarpMatrices[2];
                ovrHmd_GetEyeTimewarpMatrices(HMD, (ovrEyeType)eyeNum, eyeRenderPoses[eyeNum], timeWarpMatrices);
                //WARNING!!! These matrices are transposed in SetUniform4x4f, before being used by the shader.
                DistortionData.Shaders->SetUniform4x4f("EyeRotationStart", Matrix4f(timeWarpMatrices[0]));
                DistortionData.Shaders->SetUniform4x4f("EyeRotationEnd", Matrix4f(timeWarpMatrices[1]));
            }

            // Perform distortion
            pRender->Render(
                &distortionShaderFill,
                DistortionData.MeshVBs[eyeNum],
                DistortionData.MeshIBs[eyeNum]);
#endif

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_renderBuffer.tex);
            glUniform1i(eyeShader.GetUniLoc("fboTex"), 0);

            // This is the only uniform that changes per-frame
            glUniform1f(eyeShader.GetUniLoc("fboScale"), m_fboScale);


            glDrawElements(
                GL_TRIANGLES,
                mesh.IndexCount,
                GL_UNSIGNED_SHORT,
                0);
        }
        glBindVertexArray(0);
        glUseProgram(0);
    }

    ovrHmd_EndFrameTiming(hmd);
}
