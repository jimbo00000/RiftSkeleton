// OVRSDK06AppSkeleton.h

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
class OVRSDK06AppSkeleton : public AppSkeleton
{
public:
    OVRSDK06AppSkeleton();
    virtual ~OVRSDK06AppSkeleton();

    void initHMD() {}

    void ToggleVignette() {}
    void ToggleTimeWarp() {}
    void ToggleOverdrive() {}
    void ToggleLowPersistence() {}
    void ToggleMirrorToWindow() {}
    void ToggleDynamicPrediction() {}

    // Direct mode and SDK rendering hooks
    void AttachToWindow(void* pWindow) {}
#if defined(OVR_OS_WIN32)
    void setWindow(HWND w) {}
#endif
    ovrSizei getHmdResolution() const { return{ 1920, 1080 }; }
    ovrVector2i getHmdWindowPos() const { return{ 1920, 0 }; }
    bool UsingDebugHmd() const { return false; }
    bool UsingDirectMode() const { return true; }

    void display_stereo_undistorted() const {}
    void display_sdk() const {}
    void display_client() const {}

private: // Disallow copy ctor and assignment operator
    OVRSDK06AppSkeleton(const OVRSDK06AppSkeleton&);
    OVRSDK06AppSkeleton& operator=(const OVRSDK06AppSkeleton&);
};
