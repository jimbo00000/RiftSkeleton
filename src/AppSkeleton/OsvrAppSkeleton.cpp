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
#include <fstream>

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

///@brief Load the mesh that was saved by saveDistortionMeshToFile in RiftAppSkeleton.cpp.
/// This means you have to run that skeleton once to get the meshL.dat and meshR.dat files saved
/// with the proper parameters for your HMD.
void loadDistortionMeshFromFile(ovrDistortionMesh& mesh, const std::string filename)
{
    if (filename.empty())
        return;
    std::ifstream file;
    file.open(filename.c_str(), std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "loadDistortionMeshFromFile: could not open file " << filename << std::endl;
    }
    file.read(reinterpret_cast <char*>(&mesh.VertexCount), sizeof(mesh.VertexCount));
    file.read(reinterpret_cast <char*>(&mesh.IndexCount), sizeof(mesh.IndexCount));
    mesh.pVertexData = new ovrDistortionVertex[mesh.VertexCount];
    mesh.pIndexData = new unsigned short[mesh.IndexCount];
    file.read(reinterpret_cast <char*>(mesh.pVertexData), mesh.VertexCount*sizeof(ovrDistortionVertex));
    file.read(reinterpret_cast <char*>(mesh.pIndexData), mesh.IndexCount*sizeof(unsigned short));
    file.close();
}

void destroyDistortionMesh(ovrDistortionMesh& mesh)
{
    delete mesh.pVertexData;
    delete mesh.pIndexData;
}

void OsvrAppSkeleton::_initPresentDistMesh(ShaderWithVariables& shader, int eyeIdx)
{
    // Init left and right VAOs separately
    shader.bindVAO();

    ovrDistortionMesh& mesh = m_DistMeshes[eyeIdx];
    GLuint vertVbo = 0;
    glGenBuffers(1, &vertVbo);
    shader.AddVbo("vPosition", vertVbo);
    glBindBuffer(GL_ARRAY_BUFFER, vertVbo);
    glBufferData(GL_ARRAY_BUFFER, mesh.VertexCount * sizeof(ovrDistortionVertex), &mesh.pVertexData[0].ScreenPosNDC.x, GL_STATIC_DRAW);

    const int a_pos = shader.GetAttrLoc("vPosition");
    glVertexAttribPointer(a_pos, 4, GL_FLOAT, GL_FALSE, sizeof(ovrDistortionVertex), NULL);
    glEnableVertexAttribArray(a_pos);

    const int a_texR = shader.GetAttrLoc("vTexR");
    if (a_texR > -1)
    {
        glVertexAttribPointer(a_texR, 2, GL_FLOAT, GL_FALSE, sizeof(ovrDistortionVertex),
            reinterpret_cast<void*>(offsetof(ovrDistortionVertex, TanEyeAnglesR)));
        glEnableVertexAttribArray(a_texR);
    }

    const int a_texG = shader.GetAttrLoc("vTexG");
    if (a_texG > -1)
    {
        glVertexAttribPointer(a_texG, 2, GL_FLOAT, GL_FALSE, sizeof(ovrDistortionVertex),
            reinterpret_cast<void*>(offsetof(ovrDistortionVertex, TanEyeAnglesG)));
        glEnableVertexAttribArray(a_texG);
    }

    const int a_texB = shader.GetAttrLoc("vTexB");
    if (a_texB > -1)
    {
        glVertexAttribPointer(a_texB, 2, GL_FLOAT, GL_FALSE, sizeof(ovrDistortionVertex),
            reinterpret_cast<void*>(offsetof(ovrDistortionVertex, TanEyeAnglesB)));
        glEnableVertexAttribArray(a_texB);
    }


    GLuint elementVbo = 0;
    glGenBuffers(1, &elementVbo);
    shader.AddVbo("elements", elementVbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementVbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.IndexCount * sizeof(GLushort), &mesh.pIndexData[0], GL_STATIC_DRAW);

    // We have copies of the mesh in GL, but keep a count of indices around for the GL draw call.
    const unsigned int tmp = mesh.IndexCount;
    destroyDistortionMesh(mesh);
    mesh.IndexCount = tmp;

    glBindVertexArray(0);
}

int OsvrAppSkeleton::ConfigureRendering()
{
    const hmdRes hr = getHmdResolution();
    deallocateFBO(m_renderBuffer);
    allocateFBO(m_renderBuffer, hr.w, hr.h);

    loadDistortionMeshFromFile(m_DistMeshes[0], "meshL.dat");
    loadDistortionMeshFromFile(m_DistMeshes[1], "meshR.dat");
    _initPresentDistMesh(m_presentDistMeshL, 0);
    _initPresentDistMesh(m_presentDistMeshR, 1);

    return 0;
}

void OsvrAppSkeleton::RecenterPose()
{
}

void OsvrAppSkeleton::_DrawToRenderBuffer() const
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
}

void OsvrAppSkeleton::display_stereo_undistorted() const
{
    _DrawToRenderBuffer();
    const hmdRes hr = getHmdResolution();
    glViewport(0, 0, hr.w, hr.h);
    _PresentFboDistorted();
}

void OsvrAppSkeleton::display_stereo_distorted() const
{
    _DrawToRenderBuffer();
    const hmdRes hr = getHmdResolution();
    glViewport(0, 0, hr.w, hr.h);
    _PresentFboUndistorted();
}

void OsvrAppSkeleton::_PresentFboUndistorted() const
{
    glClearColor(0.f, 0.f, 1.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

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

void OsvrAppSkeleton::_PresentFboDistorted() const
{
    // Apply distortion from mesh loaded from OVR SDK.
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // Now draw the distortion mesh...
    for (int eyeNum = 0; eyeNum < 2; eyeNum++)
    {
        const ShaderWithVariables& eyeShader =
            eyeNum == 0 ? m_presentDistMeshL : m_presentDistMeshR;
        const GLuint prog = eyeShader.prog();
        glUseProgram(prog);
        eyeShader.bindVAO();
        {
            const ovrDistortionMesh& mesh = m_DistMeshes[eyeNum];
            ///@note Once again, these values are shamelessly ripped from the running
            /// RiftAppSkeleton in Debug mode.
            const glm::vec2 uvscale = glm::vec2(0.232447237, 0.375487238);
            const glm::vec2 uvoff = eyeNum == 0 ?
                glm::vec2(0.246082067, 0.500000000) :
                glm::vec2(0.753917933, 0.500000000);

            glUniform2f(eyeShader.GetUniLoc("EyeToSourceUVOffset"), uvoff.x, uvoff.y);
            glUniform2f(eyeShader.GetUniLoc("EyeToSourceUVScale"), uvscale.x, uvscale.y);

            ///@todo Timewarp
            {
                glm::mat4 id(1.f);
                glUniformMatrix4fv(eyeShader.GetUniLoc("EyeRotationStart"), 1, false, glm::value_ptr(id));
                glUniformMatrix4fv(eyeShader.GetUniLoc("EyeRotationEnd"), 1, false, glm::value_ptr(id));
            }

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_renderBuffer.tex);
            glUniform1i(eyeShader.GetUniLoc("fboTex"), 0);

            glUniform1f(eyeShader.GetUniLoc("fboScale"), m_fboScale);

            glDrawElements(
                GL_TRIANGLES,
                mesh.IndexCount,
                GL_UNSIGNED_SHORT,
                0);
        }
        glBindVertexArray(0);
        glUseProgram(0);
    }
}
