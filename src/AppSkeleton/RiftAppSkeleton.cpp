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

#define USE_OVR_PERF_LOGGING

RiftAppSkeleton::RiftAppSkeleton()
: m_Hmd(NULL)
, m_usingDebugHmd(false)
, m_directHmdMode(true)
{
    m_eyeOri = OVR::Quatf();
    memset(m_logUserData, 0, 256);
#ifdef USE_OVR_PERF_LOGGING
    sprintf(m_logUserData, "RiftSkeleton");
    // Unfortunately, the OVR perf log does not appear to re-read data
    // at the given user pointer each log entry(~1Hz). Is this a bug?
#endif
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
    const ovrSizei& sz = m_Cfg.OGL.Header.BackBufferSize;
    return glm::ivec2(sz.w, sz.h);
}

glm::mat4 RiftAppSkeleton::getUserViewMatrix() const
{
    const glm::quat q(m_eyeOri.w, m_eyeOri.x, m_eyeOri.y, m_eyeOri.z);
    return glm::rotate(glm::mat4(1.f), -m_chassisYaw, glm::vec3(0,1,0)) *
        glm::mat4_cast(q);
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

#ifdef USE_OVR_PERF_LOGGING
    ovrHmd_StartPerfLog(m_Hmd, "RiftSkeletonxxx-PerfLog.csv", m_logUserData);
#endif

    ///@todo Why does ovrHmd_GetEnabledCaps always return 0 when querying the caps
    /// through the field in ovrHmd appears to work correctly?
    //const unsigned int caps = ovrHmd_GetEnabledCaps(m_Hmd);
    const unsigned int caps = m_Hmd->HmdCaps;
    if ((caps & ovrHmdCap_ExtendDesktop) != 0)
    {
        m_directHmdMode = false;
    }

    m_ovrScene.SetHmdPointer(m_Hmd);

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
}

void RiftAppSkeleton::initVR()
{
    m_Cfg.OGL.Header.BackBufferSize = getHmdResolution();

    ConfigureRendering();
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
#ifdef USE_OVR_PERF_LOGGING
    ovrHmd_StopPerfLog(m_Hmd);
#endif

    deallocateFBO(m_renderBuffer);
    ovrHmd_Destroy(m_Hmd);
    ovr_Shutdown();
}

/// Add together the render target size fields of the HMD laid out side-by-side.
ovrSizei calculateCombinedTextureSize(ovrHmd pHmd)
{
    ovrSizei texSz = {0};
    if (pHmd == NULL)
        return texSz;

    const ovrSizei szL = ovrHmd_GetFovTextureSize(pHmd, ovrEye_Left, pHmd->DefaultEyeFov[ovrEye_Left], 1.f);
    const ovrSizei szR = ovrHmd_GetFovTextureSize(pHmd, ovrEye_Right, pHmd->DefaultEyeFov[ovrEye_Right], 1.f);
    texSz.w = szL.w + szR.w;
    texSz.h = std::max(szL.h, szR.h);
    return texSz;
}

///@brief Writes to m_EyeTexture and m_EyeFov
int RiftAppSkeleton::ConfigureRendering()
{
    if (m_Hmd == NULL)
        return 1;

    const ovrSizei texSz = calculateCombinedTextureSize(m_Hmd);
    deallocateFBO(m_renderBuffer);
    allocateFBO(m_renderBuffer, texSz.w, texSz.h);

    ovrGLTexture& texL = m_EyeTexture[ovrEye_Left];
    ovrGLTextureData& texDataL = texL.OGL;
    ovrTextureHeader& hdrL = texDataL.Header;

    hdrL.API = ovrRenderAPI_OpenGL;
    hdrL.TextureSize.w = texSz.w;
    hdrL.TextureSize.h = texSz.h;
    hdrL.RenderViewport.Pos.x = 0;
    hdrL.RenderViewport.Pos.y = 0;
    hdrL.RenderViewport.Size.w = texSz.w / 2;
    hdrL.RenderViewport.Size.h = texSz.h;
    texDataL.TexId = m_renderBuffer.tex;

    // Right eye the same, except for the x-position in the texture.
    ovrGLTexture& texR = m_EyeTexture[ovrEye_Right];
    texR = texL;
    texR.OGL.Header.RenderViewport.Pos.x = (texSz.w + 1) / 2;

    for (int ei=0; ei<ovrEye_Count; ++ei)
    {
        m_EyeFov[ei] = m_Hmd->DefaultEyeFov[ei];
    }

    return 0;
}

///@brief Active GL context is required for the following
/// Writes to m_Cfg
int RiftAppSkeleton::ConfigureSDKRendering()
{
    if (m_Hmd == NULL)
        return 1;

    m_Cfg.OGL.Header.API = ovrRenderAPI_OpenGL;
    m_Cfg.OGL.Header.Multisample = 0;

    const int distortionCaps =
        ovrDistortionCap_Chromatic |
        ovrDistortionCap_TimeWarp |
        ovrDistortionCap_Vignette;
    ovrHmd_ConfigureRendering(m_Hmd, &m_Cfg.Config, distortionCaps, m_EyeFov, m_EyeRenderDesc);

    return 0;
}

///@brief Writes to m_EyeRenderDesc, m_EyeRenderDesc and m_DistMeshes
int RiftAppSkeleton::ConfigureClientRendering()
{
    if (m_Hmd == NULL)
        return 1;

    const int distortionCaps =
        ovrDistortionCap_Chromatic |
        ovrDistortionCap_TimeWarp |
        ovrDistortionCap_Vignette;
    for (int eyeIndex = 0; eyeIndex < ovrEye_Count; eyeIndex++)
    {
        // Using an idiomatic loop though initializing in eye render order is not necessary.
        const ovrEyeType eye = m_Hmd->EyeRenderOrder[eyeIndex];

        m_EyeRenderDesc[eye] = ovrHmd_GetRenderDesc(m_Hmd, eye, m_EyeFov[eye]);
        m_RenderViewports[eye] = m_EyeTexture[eye].OGL.Header.RenderViewport;
        ovrHmd_CreateDistortionMesh(
            m_Hmd,
            m_EyeRenderDesc[eye].Eye,
            m_EyeRenderDesc[eye].Fov,
            distortionCaps,
            &m_DistMeshes[eye]);
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
    if (hswDisplayState.Displayed == false)
        return;

    // Detect a moderate tap on the side of the HMD.
    const ovrTrackingState ts = ovrHmd_GetTrackingState(m_Hmd, ovr_GetTimeInSeconds());
    if (ts.StatusFlags & ovrStatus_OrientationTracked)
    {
        const OVR::Vector3f v(ts.RawSensorData.Accelerometer);
        // Arbitrary value and representing moderate tap on the side of the DK2 Rift.
        if (v.LengthSq() > 250.f)
        {
            ovrHmd_DismissHSWDisplay(m_Hmd);
        }
    }
}

void RiftAppSkeleton::timestep(double absTime, double dt)
{
    AppSkeleton::timestep(absTime, dt);

#ifdef USE_OVR_PERF_LOGGING
    ///@todo Should the logger re-read data at this user pointer for each entry?
    const glm::ivec2 rtsz = getRTSize();
    sprintf(m_logUserData, "RT%dx%d", rtsz.x, rtsz.y);
#endif
}

/// Scale the parallax translation and head pose motion vector by the head size
/// dictated by the shader. Thanks to the elegant design decision of putting the
/// head's default position at the origin, this is simple.
OVR::Matrix4f makeModelviewMatrix(
    ovrPosef eyePose,
    float chassisYaw,
    ovrVector3f chassisPos)
{
    const OVR::Matrix4f eyePoseMatrix =
        OVR::Matrix4f::Translation(OVR::Vector3f(eyePose.Position))
        * OVR::Matrix4f(OVR::Quatf(eyePose.Orientation));

    const OVR::Matrix4f view =
        eyePoseMatrix.Inverted()
        * OVR::Matrix4f::RotationY(chassisYaw)
        * OVR::Matrix4f::Translation(-OVR::Vector3f(chassisPos));

    return view;
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
    //ovrHmd_BeginFrame(hmd, 0);

    bindFBO(m_renderBuffer, m_fboScale);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ovrVector3f e2v[2] = {
        OVR::Vector3f(m_EyeRenderDesc[0].HmdToEyeViewOffset),
        OVR::Vector3f(m_EyeRenderDesc[1].HmdToEyeViewOffset),
    };

    ovrTrackingState outHmdTrackingState;
    ovrPosef outEyePoses[2];
    ovrHmd_GetEyePoses(
        hmd,
        0,
        e2v, // could this parameter be const?
        outEyePoses,
        &outHmdTrackingState);

    // For passing to EndFrame once rendering is done
    ovrPosef renderPose[2];
    ovrTexture eyeTexture[2];
    for (int eyeIndex = 0; eyeIndex < ovrEye_Count; eyeIndex++)
    {
        const ovrEyeType e = hmd->EyeRenderOrder[eyeIndex];

        const ovrPosef eyePose = outEyePoses[e];
        renderPose[e] = eyePose;
        eyeTexture[e] = m_EyeTexture[e].Texture;
        m_eyeOri = eyePose.Orientation; // cache this for movement direction

        const ovrGLTexture& otex = m_EyeTexture[e];
        const ovrRecti& rvp = otex.OGL.Header.RenderViewport;
        const ovrRecti rsc = {
            static_cast<int>(m_fboScale * rvp.Pos.x),
            static_cast<int>(m_fboScale * rvp.Pos.y),
            static_cast<int>(m_fboScale * rvp.Size.w),
            static_cast<int>(m_fboScale * rvp.Size.h)
        };
        glViewport(rsc.Pos.x, rsc.Pos.y, rsc.Size.w, rsc.Size.h);

        const OVR::Matrix4f proj = ovrMatrix4f_Projection(
            m_EyeRenderDesc[e].Fov,
            0.01f, 10000.0f, true);

        const OVR::Matrix4f view = makeModelviewMatrix(
            eyePose,
            m_chassisYaw,
            OVR::Vector3f(m_chassisPos.x, m_chassisPos.y, m_chassisPos.z));

        _resetGLState();

        _DrawScenes(&view.Transposed().M[0][0], &proj.Transposed().M[0][0]);
    }
    unbindFBO();

    //ovrHmd_EndFrame(m_Hmd, renderPose, eyeTexture);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    const int w = m_Cfg.OGL.Header.BackBufferSize.w;
    const int h = m_Cfg.OGL.Header.BackBufferSize.h;
    glViewport(0, 0, w, h);

    // Present FBO to screen
    const GLuint prog = m_presentFbo.prog();
    glUseProgram(prog);
    m_presentFbo.bindVAO();
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_renderBuffer.tex);
        glUniform1i(m_presentFbo.GetUniLoc("fboTex"), 0);
        glUniform1f(m_presentFbo.GetUniLoc("fboScale"), m_fboScale);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }
    glBindVertexArray(0);
    glUseProgram(0);
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

    ovrVector3f e2v[2] = {
        m_EyeRenderDesc[0].HmdToEyeViewOffset,
        m_EyeRenderDesc[1].HmdToEyeViewOffset,
    };
    ovrTrackingState outHmdTrackingState;
    ovrPosef outEyePoses[2];
    ovrHmd_GetEyePoses(
        hmd,
        0,
        e2v, // could this parameter be const?
        outEyePoses,
        &outHmdTrackingState);

    // For passing to EndFrame once rendering is done
    ovrPosef renderPose[2];
    ovrTexture eyeTexture[2];
    for (int eyeIndex=0; eyeIndex<ovrEye_Count; eyeIndex++)
    {
        const ovrEyeType e = hmd->EyeRenderOrder[eyeIndex];

        const ovrPosef eyePose = outEyePoses[e];
        renderPose[e] = eyePose;
        eyeTexture[e] = m_EyeTexture[e].Texture;
        m_eyeOri = eyePose.Orientation; // cache this for chassis movement direction

        const ovrGLTexture& otex = m_EyeTexture[e];
        const ovrRecti& rvp = otex.OGL.Header.RenderViewport;
        glViewport(
            static_cast<int>(m_fboScale * rvp.Pos.x),
            static_cast<int>(m_fboScale * rvp.Pos.y),
            static_cast<int>(m_fboScale * rvp.Size.w),
            static_cast<int>(m_fboScale * rvp.Size.h)
            );

        const OVR::Matrix4f proj = ovrMatrix4f_Projection(
            m_EyeRenderDesc[e].Fov, 0.01f, 100.0f, true);
        
        const OVR::Matrix4f view = makeModelviewMatrix(
            eyePose,
            m_chassisYaw,
            OVR::Vector3f(m_chassisPos.x, m_chassisPos.y, m_chassisPos.z));

        _resetGLState();

        _DrawScenes(&view.Transposed().M[0][0], &proj.Transposed().M[0][0]);
    }
    unbindFBO();

    // Inform SDK of downscaled texture target size(performance scaling)
    for (int i=0; i<ovrEye_Count; ++i)
    {
        const ovrSizei& ts = m_EyeTexture[i].Texture.Header.TextureSize;
        ovrRecti& rr = eyeTexture[i].Header.RenderViewport;
        rr.Size.w = static_cast<int>(static_cast<float>(ts.w/2) * m_fboScale);
        rr.Size.h = static_cast<int>(static_cast<float>(ts.h) * m_fboScale);
        rr.Pos.x = i * rr.Size.w;
    }
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

    ovrVector3f e2v[2];
    e2v[0] = m_EyeRenderDesc[0].HmdToEyeViewOffset;
    e2v[1] = m_EyeRenderDesc[1].HmdToEyeViewOffset;
    ovrTrackingState outHmdTrackingState;
    ovrPosef outEyePoses[2];
    ovrHmd_GetEyePoses(
        hmd,
        0,
        e2v,
        outEyePoses,
        &outHmdTrackingState);

    for (int eyeIndex = 0; eyeIndex < ovrEye_Count; eyeIndex++)
    {
        const ovrEyeType eye = hmd->EyeRenderOrder[eyeIndex];
        const ovrPosef eyePose = outEyePoses[eye];
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

        const OVR::Matrix4f view = makeModelviewMatrix(
            eyePose,
            m_chassisYaw,
            OVR::Vector3f(m_chassisPos.x, m_chassisPos.y, m_chassisPos.z));

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
