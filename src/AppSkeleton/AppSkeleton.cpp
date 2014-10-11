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

void AppSkeleton::resize(int w, int h)
{
    (void)w;
    (void)h;
    //m_Cfg.OGL.Header.RTSize.w = w;
    //m_Cfg.OGL.Header.RTSize.h = h;

    //const int l_DistortionCaps = ovrDistortionCap_Chromatic | ovrDistortionCap_TimeWarp;
    ///@warning this crashes the app. What are we supposed to do here???
    //ovrHmd_ConfigureRendering(m_Hmd, &m_Cfg.Config, l_DistortionCaps, m_EyeFov, m_EyeRenderDesc);
}
