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
#include "MatrixFunctions.h"

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

    ConfigureRendering();
}

void OsvrAppSkeleton::exitVR()
{
    osvrClientShutdown(ctx);
}

int OsvrAppSkeleton::ConfigureRendering()
{
    const hmdRes hr = getHmdResolution();
    deallocateFBO(m_renderBuffer);
    allocateFBO(m_renderBuffer, hr.w, hr.h);

    return 0;
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

    ///@todo Timewarp, late latching
    const glm::vec3 poset(
        state.translation.data[0],
        state.translation.data[1],
        state.translation.data[2]);
    const glm::quat poser(
        osvrQuatGetW(&(state.rotation)),
        osvrQuatGetX(&(state.rotation)),
        osvrQuatGetY(&(state.rotation)),
        osvrQuatGetZ(&(state.rotation)));

#if 0
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
#endif

    // Draw
    const float m_fboScale = 1.f; ///@todo buffer scaling
    bindFBO(m_renderBuffer, m_fboScale);

    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Ripped from OVR's function output at runtime
    const float pL[] = {
        0.929788947, 0.000000000, -0.0156717598, 0.000000000,
        0.000000000, 0.750974476, 0.000000000, 0.000000000,
        0.000000000, 0.000000000, -1.00000095, -0.0100000100,
        0.000000000, 0.000000000, -1.00000000, 0.000000000,
    };
    const float pR[] = {
        0.929788947, 0.000000000, 0.0156717598, 0.000000000,
        0.000000000, 0.750974476, 0.000000000, 0.000000000,
        0.000000000, 0.000000000, -1.00000095, -0.0100000100,
        0.000000000, 0.000000000, -1.00000000, 0.000000000,
    };
    const glm::mat4 projL = glm::transpose(glm::make_mat4(pL));
    const glm::mat4 projR = glm::transpose(glm::make_mat4(pR));

    const hmdRes hr = getHmdResolution();
    const float ipd = .064; ///@todo User configuration
    const float halfIpd = ipd * .5f;
    for (int e = 0; e < 2; ++e) // eye loop
    {
        // Assume side-by-side configuration
        glViewport(e*hr.w/2, 0, hr.w / 2, hr.h);
        const float eyeTx = e == 0 ? -halfIpd : halfIpd;
        const glm::mat4 viewLocal = glm::translate(
            makeMatrixFromPoseComponents(poset, poser),
            glm::vec3(eyeTx, 0.f, 0.f));
        const glm::mat4 viewWorld = makeWorldToChassisMatrix() * viewLocal;
        _resetGLState();
        _DrawScenes(
            glm::value_ptr(glm::inverse(viewWorld)),
            e == 0 ? glm::value_ptr(projL) : glm::value_ptr(projR),
            glm::value_ptr(glm::inverse(viewLocal)));
    }
    unbindFBO();


    // Set up for present to HMD screen
    glClearColor(0.f, 0.f, 1.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glViewport(0, 0, hr.w, hr.h);

    // Present FBO to screen
    const GLuint prog = m_presentFbo.prog();
    glUseProgram(prog);
    m_presentFbo.bindVAO();
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_renderBuffer.tex);
        glUniform1i(m_presentFbo.GetUniLoc("fboTex"), 0);
        glUniform1f(m_presentFbo.GetUniLoc("fboScale"), m_fboScale);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }
    glBindVertexArray(0);
    glUseProgram(0);
}
