// OVRSDK08AppSkeleton.h

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

#include <Kernel/OVR_Types.h> // Pull in OVR_OS_* defines 
#include <OVR_CAPI.h>
#include <OVR_CAPI_GL.h>

// A surface displayed by OVR compositor in-world and an FBO to draw to it.
struct worldQuad {
    ovrLayerQuad m_layerQuad;
    ovrSwapTextureSet* m_pQuadTex;
    FBO fbo;
    bool m_showQuadInWorld;
    glm::vec3 m_quadLocation;
    glm::vec3 m_quadRotation;
};

///@brief Encapsulates as much of the VR viewer state as possible,
/// pushing all viewer-independent stuff to Scene.
class OVRSDK08AppSkeleton : public AppSkeleton
{
public:
    enum MirrorType {
        MirrorNone = 0,
        MirrorDistorted,
        MirrorUndistorted,
        NumMirrorTypes
    };

    OVRSDK08AppSkeleton();
    virtual ~OVRSDK08AppSkeleton();

    void initHMD();
    void initVR(bool swapBackBufferDims = false);
    void RecenterPose();
    void exitVR();

    void ToggleMirroringType();
    void TogglePerfHud();
    void ToggleQuadInWorld();

    void SetAppWindowSize(ovrSizei sz) { m_appWindowSize = sz; AppSkeleton::resize(sz.w, sz.h); }

    ovrSizei getHmdResolution() const;

    void display_stereo_undistorted() { display_sdk(); }
    void display_sdk() const;
    void display_client() const { display_sdk(); }

protected:
    void _DestroySwapTextures();
    void _InitQuadLayer(
        worldQuad& quad,
        const ovrSizei size);
    void _DrawToTweakbarQuad() const;
    void _DrawToSecondQuad() const;
    void BlitLeftEyeRenderToUndistortedMirrorTexture() const;

    ovrHmd m_Hmd;
    ovrEyeRenderDesc m_eyeRenderDescs[ovrEye_Count];
    ovrVector3f m_eyeOffsets[ovrEye_Count];
    glm::mat4 m_eyeProjections[ovrEye_Count];

    mutable ovrPosef m_eyePoses[ovrEye_Count];
    mutable ovrLayerEyeFov m_layerEyeFov;
    mutable int m_frameIndex;

    ovrTexture* m_pMirrorTex;
    ovrSwapTextureSet* m_pTexSet[ovrEye_Count];
    FBO m_swapFBO;
    FBO m_mirrorFBO;
    FBO m_undistortedFBO;

    ovrSizei m_appWindowSize;
    MirrorType m_mirror;
    ovrPerfHudMode m_perfHudMode;

public:
    worldQuad m_tweakbarQuad;
    worldQuad m_secondQuad;

private: // Disallow copy ctor and assignment operator
    OVRSDK08AppSkeleton(const OVRSDK08AppSkeleton&);
    OVRSDK08AppSkeleton& operator=(const OVRSDK08AppSkeleton&);
};
