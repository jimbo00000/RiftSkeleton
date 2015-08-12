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
}

void OsvrAppSkeleton::exitVR()
{
}

void OsvrAppSkeleton::RecenterPose()
{
}

void OsvrAppSkeleton::display_stereo_undistorted() const
{
    osvr::clientkit::ClientContext context(
        "com.osvr.exampleclients.TrackerState");

    // This is just one of the paths. You can also use:
    // /me/hands/right
    // /me/head
    osvr::clientkit::Interface head =
        context.getInterface("/me/head");


    // Note that there is not currently a tidy C++ wrapper for
    // state access, so we're using the C API call directly here.
    OSVR_PoseState state;
    OSVR_TimeValue timestamp;
    OSVR_ReturnCode ret =
        osvrGetPoseState(head.get(), &timestamp, &state);
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

    glClearColor(1, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
}
