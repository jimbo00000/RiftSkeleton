// RiftAppSkeleton.h

#pragma once

#ifdef __APPLE__
#include "opengl/gl.h"
#endif

#ifdef _WIN32
#  define WINDOWS_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#endif

#include "AppSkeleton.h"

#include <OVR_CAPI.h>
#include <OVR_CAPI_GL.h>


///@brief Encapsulates as much of the VR viewer state as possible,
/// pushing all viewer-independent stuff to Scene.
class RiftAppSkeleton : public AppSkeleton
{
public:
    RiftAppSkeleton();
    virtual ~RiftAppSkeleton();

    void initHMD();
    void initVR();
    void exitVR();

    void RecenterPose();
    void SetChassisPosition(glm::vec3 p) { m_chassisPos = p; }
    ovrSizei getHmdResolution() const;
    ovrVector2i getHmdWindowPos() const;
    GLuint getRenderBufferTex() const { return m_renderBuffer.tex; }
    float GetFboScale() const { return m_fboScale; }
    bool UsingDebugHmd() const { return m_usingDebugHmd; }

    int ConfigureSDKRendering();
    int ConfigureClientRendering();

#if defined(OVR_OS_WIN32)
    void setWindow(HWND w) { m_Cfg.OGL.Window = w; }
#elif defined(OVR_OS_LINUX)
    void setWindow(Window Win, Display* Disp)
    {
        m_Cfg.OGL.Win = Win;
        m_Cfg.OGL.Disp = Disp;
    }
#endif

    void DismissHealthAndSafetyWarning() const;
    void CheckForTapToDismissHealthAndSafetyWarning() const;

    void display_stereo_undistorted();// const;
    void display_sdk();// const;
    void display_client();// const;
    void timestep(float dt);


    float GetFBOScale() const { return m_fboScale; }
    void SetFBOScale(float s);
#ifdef USE_ANTTWEAKBAR
    float* GetFBOScalePointer() { return &m_fboScale; }
#endif

protected:
    void _initPresentDistMesh(ShaderWithVariables& shader, int eyeIdx);
    virtual glm::ivec2 getRTSize() const;

    ovrHmd m_Hmd;
    ovrFovPort m_EyeFov[2];
    ovrGLConfig m_Cfg;
    ovrEyeRenderDesc m_EyeRenderDesc[2];
    ovrGLTexture m_EyeTexture[2];
    bool m_usingDebugHmd;

    // For client rendering
    ovrRecti m_RenderViewports[2];
    ovrVector2f m_uvScaleOffsetOut[4];
    ovrDistortionMesh m_DistMeshes[2];
    ovrQuatf m_eyeOri;


private: // Disallow copy ctor and assignment operator
    RiftAppSkeleton(const RiftAppSkeleton&);
    RiftAppSkeleton& operator=(const RiftAppSkeleton&);
};
