// OVRSDK06AppSkeleton.cpp

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
