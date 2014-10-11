// AppSkeleton.cpp

#ifdef _WIN32
#  define WINDOWS_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#endif
#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "AppSkeleton.h"

AppSkeleton::AppSkeleton()
: m_scene()
, m_hydraScene()
, m_ovrScene()
, m_scenes()

, m_fboScale(1.0f)
, m_presentFbo()
, m_presentDistMeshL()
, m_presentDistMeshR()
, m_chassisYaw(0.0f)
, m_fm()
, m_hyif()
, m_rtSize(640,480)
, m_keyboardMove(0.0f)
, m_joystickMove(0.0f)
, m_mouseMove(0.0f)
, m_keyboardYaw(0.0f)
, m_joystickYaw(0.0f)
, m_mouseDeltaYaw(0.0f)
{
    // Add as many scenes here as you like. They will share color and depth buffers,
    // so drawing one after the other should just result in pixel-perfect integration -
    // provided they all do forward rendering. Per-scene deferred render passes will
    // take a little bit more work.
    m_scenes.push_back(&m_scene);
    m_scenes.push_back(&m_hydraScene);
    m_scenes.push_back(&m_ovrScene);

    // Give this scene a pointer to get live Hydra data for display
    m_hydraScene.SetFlyingMousePointer(&m_fm);

    ResetAllTransformations();
}

AppSkeleton::~AppSkeleton()
{
    m_fm.Destroy();
}

void AppSkeleton::SetFBOScale(float s)
{
    m_fboScale = s;
    m_fboScale = std::max(0.05f, m_fboScale);
    m_fboScale = std::min(1.0f, m_fboScale);
}

void AppSkeleton::ResetAllTransformations()
{
    m_chassisPos.x = 0.0f;
    m_chassisPos.y = 1.27f; // my sitting height
    m_chassisPos.z = 1.0f;
    m_chassisYaw = 0.0f;
}

void AppSkeleton::initGL()
{
    for (std::vector<IScene*>::iterator it = m_scenes.begin();
        it != m_scenes.end();
        ++it)
    {
        IScene* pScene = *it;
        if (pScene != NULL)
        {
            pScene->initGL();
        }
    }

    m_presentFbo.initProgram("presentfbo");
    _initPresentFbo();
    m_presentDistMeshL.initProgram("presentmesh");
    m_presentDistMeshR.initProgram("presentmesh");
    // Init the present mesh VAO *after* initVR, which creates the mesh

    // sensible initial value?
    allocateFBO(m_renderBuffer, 800, 600);
    m_fm.Init();
}


void AppSkeleton::_initPresentFbo()
{
    m_presentFbo.bindVAO();

    const float verts[] = {
        -1, -1,
        1, -1,
        1, 1,
        -1, 1
    };
    const float texs[] = {
        0, 0,
        1, 0,
        1, 1,
        0, 1,
    };

    GLuint vertVbo = 0;
    glGenBuffers(1, &vertVbo);
    m_presentFbo.AddVbo("vPosition", vertVbo);
    glBindBuffer(GL_ARRAY_BUFFER, vertVbo);
    glBufferData(GL_ARRAY_BUFFER, 4*2*sizeof(GLfloat), verts, GL_STATIC_DRAW);
    glVertexAttribPointer(m_presentFbo.GetAttrLoc("vPosition"), 2, GL_FLOAT, GL_FALSE, 0, NULL);

    GLuint texVbo = 0;
    glGenBuffers(1, &texVbo);
    m_presentFbo.AddVbo("vTex", texVbo);
    glBindBuffer(GL_ARRAY_BUFFER, texVbo);
    glBufferData(GL_ARRAY_BUFFER, 4*2*sizeof(GLfloat), texs, GL_STATIC_DRAW);
    glVertexAttribPointer(m_presentFbo.GetAttrLoc("vTex"), 2, GL_FLOAT, GL_FALSE, 0, NULL);

    glEnableVertexAttribArray(m_presentFbo.GetAttrLoc("vPosition"));
    glEnableVertexAttribArray(m_presentFbo.GetAttrLoc("vTex"));

    glUseProgram(m_presentFbo.prog());
    {
        glm::mat4 id(1.0f);
        glUniformMatrix4fv(m_presentFbo.GetUniLoc("mvmtx"), 1, false, glm::value_ptr(id));
        glUniformMatrix4fv(m_presentFbo.GetUniLoc("prmtx"), 1, false, glm::value_ptr(id));
    }
    glUseProgram(0);

    glBindVertexArray(0);
}

void AppSkeleton::_resetGLState() const
{
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthRangef(0.0f, 1.0f);
    glDepthFunc(GL_LESS);

    glDisable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
}

void AppSkeleton::_DrawScenes(const float* pMview, const float* pPersp) const
{
    for (std::vector<IScene*>::const_iterator it = m_scenes.begin();
        it != m_scenes.end();
        ++it)
    {
        const IScene* pScene = *it;
        if (pScene != NULL)
        {
            pScene->RenderForOneEye(pMview, pPersp);
        }
    }
}

void AppSkeleton::_drawSceneMono() const
{
    _resetGLState();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const glm::vec3 EyePos(m_chassisPos.x, m_chassisPos.y, m_chassisPos.z);
    const glm::vec3 LookVec(0.0f, 0.0f, -1.0f);
    const glm::vec3 up(0.0f, 1.0f, 0.0f);
    glm::mat4 lookat = glm::lookAt(EyePos, EyePos + LookVec, up);
    lookat = glm::translate(lookat, EyePos);
    lookat = glm::rotate(lookat, m_chassisYaw, glm::vec3(0.0f, 1.0f, 0.0f));
    lookat = glm::translate(lookat, -EyePos);

    const glm::ivec2 vp = getRTSize();
    const glm::mat4 persp = glm::perspective(
        90.0f,
        static_cast<float>(vp.x)/static_cast<float>(vp.y),
        0.004f,
        500.0f);

    _DrawScenes(glm::value_ptr(lookat), glm::value_ptr(persp));
}

void AppSkeleton::display_raw() const
{
    const glm::ivec2 vp = getRTSize();
    glViewport(0, 0, vp.x, vp.y);
    _drawSceneMono();
}

void AppSkeleton::display_buffered(bool setViewport) const
{
    bindFBO(m_renderBuffer, m_fboScale);
    _drawSceneMono();
    unbindFBO();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    if (setViewport)
    {
        const glm::ivec2 vp = getRTSize();
        glViewport(0, 0, vp.x, vp.y);
    }

    // Present FBO to screen
    const GLuint prog = m_presentFbo.prog();
    glUseProgram(prog);
    m_presentFbo.bindVAO();
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_renderBuffer.tex);
        glUniform1i(m_presentFbo.GetUniLoc("fboTex"), 0);

        // This is the only uniform that changes per-frame
        glUniform1f(m_presentFbo.GetUniLoc("fboScale"), m_fboScale);

        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }
    glBindVertexArray(0);
    glUseProgram(0);
}

void AppSkeleton::resize(int w, int h)
{
    m_rtSize.x = w;
    m_rtSize.y = h;
}
