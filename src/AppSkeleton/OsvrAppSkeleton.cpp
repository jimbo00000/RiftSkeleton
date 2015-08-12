// OsvrAppSkeleton.cpp

#include <osvr/ClientKit/Context.h>
#include <osvr/ClientKit/Interface.h>
#include <osvr/ClientKit/InterfaceStateC.h>

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

#include "OsvrAppSkeleton.h"

OsvrAppSkeleton::OsvrAppSkeleton()
{
}

OsvrAppSkeleton::~OsvrAppSkeleton()
{
}

void OsvrAppSkeleton::initHMD()
{
}

void OsvrAppSkeleton::initVR(bool swapBackBufferDims)
{
    ctx = osvrClientInit("com.osvr.jimbo00000.RiftSkeleton", 0);
    osvrClientGetInterface(ctx, "/me/head", &head);
}

void OsvrAppSkeleton::exitVR()
{
    osvrClientShutdown(ctx);
}

void OsvrAppSkeleton::RecenterPose()
{
}

void OsvrAppSkeleton::display_stereo_undistorted() const
{
    osvrClientUpdate(ctx);

    OSVR_PoseState state;
    OSVR_TimeValue timestamp;
    OSVR_ReturnCode ret =
        osvrGetPoseState(head, &timestamp, &state);
    if (OSVR_RETURN_SUCCESS != ret) {
        std::cout << "No pose state!" << std::endl;
    }
    else {
        std::cout << "Got POSE state: Position = ("
            //<< state.translation.data[0] << ", "
            //<< state.translation.data[1] << ", "
            //<< state.translation.data[2] << "), orientation = ("
            << osvrQuatGetW(&(state.rotation)) << ", ("
            << osvrQuatGetX(&(state.rotation)) << ", "
            << osvrQuatGetY(&(state.rotation)) << ", "
            << osvrQuatGetZ(&(state.rotation)) << ")"
            << std::endl;
    }

    // Draw
    glClearColor(1, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
}
