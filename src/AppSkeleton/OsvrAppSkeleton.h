// OsvrAppSkeleton.h

#pragma once

#include <osvr/ClientKit/Interface.h>
#include <osvr/ClientKit/InterfaceStateC.h>

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

///@note These structs ripped from the Oculus SDK with glm::vec's subbed in
typedef struct ovrDistortionVertex_
{
    glm::vec2 ScreenPosNDC;    ///< [-1,+1],[-1,+1] over the entire framebuffer.
    float       TimeWarpFactor;  ///< Lerp factor between time-warp matrices. Can be encoded in Pos.z.
    float       VignetteFactor;  ///< Vignette fade factor. Can be encoded in Pos.w.
    glm::vec2 TanEyeAnglesR;   ///< The tangents of the horizontal and vertical eye angles for the red channel.
    glm::vec2 TanEyeAnglesG;   ///< The tangents of the horizontal and vertical eye angles for the green channel.
    glm::vec2 TanEyeAnglesB;   ///< The tangents of the horizontal and vertical eye angles for the blue channel.
} ovrDistortionVertex;

/// Describes a full set of distortion mesh data, filled in by ovrHmd_CreateDistortionMesh.
/// Contents of this data structure, if not null, should be freed by ovrHmd_DestroyDistortionMesh.
typedef struct ovrDistortionMesh_
{
    ovrDistortionVertex* pVertexData; ///< The distortion vertices representing each point in the mesh.
    unsigned short*      pIndexData;  ///< Indices for connecting the mesh vertices into polygons.
    unsigned int         VertexCount; ///< The number of vertices in the mesh.
    unsigned int         IndexCount;  ///< The number of indices in the mesh.
} ovrDistortionMesh;

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
    winPos getHmdWindowPos() const { return{ 1920, 0 }; }

protected:
    void _initPresentDistMesh(ShaderWithVariables& shader, int eyeIdx);

    OSVR_ClientContext ctx;
    OSVR_ClientInterface head;
    ovrDistortionMesh m_DistMeshes[2];

private: // Disallow copy ctor and assignment operator
    OsvrAppSkeleton(const OsvrAppSkeleton&);
    OsvrAppSkeleton& operator=(const OsvrAppSkeleton&);
};
