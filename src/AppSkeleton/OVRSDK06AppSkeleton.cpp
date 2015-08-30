// OVRSDK06AppSkeleton.cpp
// Huge thanks to jherico
// https://github.com/jherico/OculusRiftInAction/blob/ea0231ad045187c8a5819a801ce4a2fae63301aa/examples/cpp/Example_SelfContained.cpp

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
#include <fstream>

#include <OVR.h>

#include "OVRSDK06AppSkeleton.h"
#include "ShaderFunctions.h"
#include "MatrixFunctions.h"
#include "GLUtils.h"
#include "Logger.h"

#define USE_OVR_PERF_LOGGING

#ifndef PROJECT_NAME
// This macro should be defined in CMakeLists.txt
#define PROJECT_NAME "RiftSkeleton"
#endif

OVRSDK06AppSkeleton::OVRSDK06AppSkeleton()
{
}

OVRSDK06AppSkeleton::~OVRSDK06AppSkeleton()
{
}
void OVRSDK06AppSkeleton::RecenterPose()
{
    if (m_Hmd == NULL)
        return;
    ovrHmd_RecenterPose(m_Hmd);
}

void OVRSDK06AppSkeleton::exitVR()
{
    //deallocateFBO(m_renderBuffer);
    ovrHmd_Destroy(m_Hmd);
    ovr_Shutdown();
}

void OVRSDK06AppSkeleton::initHMD()
{
    ovrInitParams initParams; memset(&initParams, 0, sizeof(ovrInitParams));
    if (ovrSuccess != ovr_Initialize(NULL))
    {
        LOG_INFO("Failed to initialize the Oculus SDK");
    }

    if (ovrSuccess != ovrHmd_Create(0, &m_Hmd))
    {
        if (ovrSuccess != ovrHmd_CreateDebug(ovrHmd_DK2, &m_Hmd)) {
            LOG_INFO("Could not create HMD");
        }
    }
   // hmdNativeResolution = ivec2(hmd->Resolution.w, hmd->Resolution.h);
}


