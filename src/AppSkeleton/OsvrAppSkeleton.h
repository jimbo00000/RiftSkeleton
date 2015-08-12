// OsvrAppSkeleton.h

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

struct hmdRes {
    int w, h;
};

struct winPos {
    int x, y;
};

///@brief Encapsulates as much of the VR viewer state as possible,
/// pushing all viewer-independent stuff to Scene.
class OsvrAppSkeleton : public AppSkeleton
{
public:
    OsvrAppSkeleton();
    virtual ~OsvrAppSkeleton();

    void initHMD();
    void initVR(bool swapBackBufferDims = false);
    void exitVR();
    void RecenterPose();
    int ConfigureRendering();

    void display_stereo_undistorted() const;

    // Hardcoded dimensions to match default Rift settings in Extended Mode.
    // Set screen orientation to Landscape (flipped).
    hmdRes getHmdResolution() const { return { 1920, 1080 }; }
    winPos getHmdWindowPos() const { return {1920, 0}; }

private: // Disallow copy ctor and assignment operator
    OsvrAppSkeleton(const OsvrAppSkeleton&);
    OsvrAppSkeleton& operator=(const OsvrAppSkeleton&);
};
