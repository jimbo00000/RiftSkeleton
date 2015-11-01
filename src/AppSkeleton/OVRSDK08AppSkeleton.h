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

    void ToggleVignette() {}
    void ToggleTimeWarp() {}
    void ToggleOverdrive() {}
    void ToggleLowPersistence() {}
    void ToggleDynamicPrediction() {}
    void ToggleMirroringType();
    void ToggleQuadInWorld() { m_showQuadInWorld = !m_showQuadInWorld; }
    void TogglePerfHud();

    void SetAppWindowSize(ovrSizei sz) { m_appWindowSize = sz; }

    ovrSizei getHmdResolution() const;
    bool UsingDebugHmd() const { return m_usingDebugHmd; }

    void display_stereo_undistorted() { display_sdk(); }
    void display_sdk() const;
    void display_client() const { display_sdk(); }

protected:
    void _DestroySwapTextures();
    void _InitQuadLayer(
        const ovrSizei size,
        ovrSwapTextureSet** pQuadTex,
        ovrLayerQuad& layer,
        FBO& quadFBO);
    void _DrawToTweakbarQuad() const;
    void BlitLeftEyeRenderToUndistortedMirrorTexture() const;

    ovrHmd m_Hmd;
    ovrEyeRenderDesc m_eyeRenderDescs[ovrEye_Count];
    ovrVector3f m_eyeOffsets[ovrEye_Count];
    glm::mat4 m_eyeProjections[ovrEye_Count];

    mutable ovrPosef m_eyePoses[ovrEye_Count];
    mutable ovrLayerEyeFov m_layerEyeFov;
    mutable ovrLayerQuad m_layerQuad;
    mutable int m_frameIndex;

    ovrTexture* m_pMirrorTex;
    ovrSwapTextureSet* m_pTexSet[ovrEye_Count];
    ovrSwapTextureSet* m_pQuadTex;
    FBO m_swapFBO;
    FBO m_quadFBO;
    FBO m_mirrorFBO;
    FBO m_undistortedFBO;

    ovrSizei m_appWindowSize;
    bool m_usingDebugHmd;
    MirrorType m_mirror;
    bool m_showQuadInWorld;
    ovrPerfHudMode m_perfHudMode;

public:
    glm::vec3 m_quadLocation;
    glm::vec3 m_quadRotation;

private: // Disallow copy ctor and assignment operator
    OVRSDK08AppSkeleton(const OVRSDK08AppSkeleton&);
    OVRSDK08AppSkeleton& operator=(const OVRSDK08AppSkeleton&);
};
