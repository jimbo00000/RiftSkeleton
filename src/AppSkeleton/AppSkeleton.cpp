// AppSkeleton.cpp

#ifdef _WIN32
#  define WINDOWS_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#endif
#include <GL/glew.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "AppSkeleton.h"

AppSkeleton::AppSkeleton()
: m_scene()
, m_hydraScene()
#ifdef USE_OCULUSSDK
, m_ovrScene()
#endif
, m_scenes()

, m_fboScale(1.0f)
, m_presentFbo()
, m_presentDistMeshL()
, m_presentDistMeshR()
, m_chassisYaw(0.0f)
, m_fm()
, m_hyif()
, m_rtSize(800, 600)
, m_rayHitsScene(false)
, m_spaceCursor()
, m_spaceCursorPos()
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
#ifdef USE_OCULUSSDK
    m_scenes.push_back(&m_ovrScene);
#endif

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

glm::mat4 makeChassisMatrix_glm(
    float chassisYaw,
    glm::vec3 chassisPos)
{
    return
        glm::translate(glm::mat4(1.f), chassisPos) *
        glm::rotate(glm::mat4(1.f), -chassisYaw, glm::vec3(0,1,0));
}

glm::mat4 AppSkeleton::getUserViewMatrix() const
{
    return makeChassisMatrix_glm(m_chassisYaw, m_chassisPos);
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

    m_spaceCursor.initGL();
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

void AppSkeleton::_DrawScenes(
    const float* pMvWorld,
    const float* pPersp,
    const float* pMvLocal) const
{
    for (std::vector<IScene*>::const_iterator it = m_scenes.begin();
        it != m_scenes.end();
        ++it)
    {
        const IScene* pScene = *it;
        if (pScene != NULL)
        {
            const float* pMv = pScene->m_bChassisLocalSpace ? pMvLocal : pMvWorld;
            pScene->RenderForOneEye(pMv, pPersp);
        }
    }

    // Draw scene hit cursor
    if (m_rayHitsScene)
    {
        const glm::mat4 modelview = glm::make_mat4(pMvWorld);
        glm::mat4 cursMtx = glm::translate(modelview, m_spaceCursorPos);
        m_spaceCursor.RenderForOneEye(glm::value_ptr(cursMtx), pPersp);
    }
}

void AppSkeleton::_checkSceneIntersections(glm::vec3 origin, glm::vec3 dir)
{
    m_rayHitsScene = false;
    for (std::vector<IScene*>::const_iterator it = m_scenes.begin();
        it != m_scenes.end();
        ++it)
    {
        const IScene* pScene = *it;
        if (pScene != NULL)
        {
            float t = 0.f;
            glm::vec3 hit;
            glm::vec3 norm;
            if (pScene->RayIntersects(glm::value_ptr(origin), glm::value_ptr(dir),
                &t, glm::value_ptr(hit), glm::value_ptr(norm)))
            {
                m_rayHitsScene = true;
                m_spaceCursorPos = hit;
            }
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
    glm::mat4 lookat = glm::lookAt(glm::vec3(0.f), LookVec, up);
    lookat = glm::rotate(lookat, m_chassisYaw, glm::vec3(0.0f, 1.0f, 0.0f));
    lookat = glm::translate(lookat, -EyePos);

    const glm::ivec2 vp = getRTSize();
    const glm::mat4 persp = glm::perspective(
        90.0f,
        static_cast<float>(vp.x)/static_cast<float>(vp.y),
        0.004f,
        500.0f);

    const glm::mat4 mvLocal = glm::lookAt(glm::vec3(0.f), LookVec, up);

    _DrawScenes(glm::value_ptr(lookat), glm::value_ptr(persp), glm::value_ptr(mvLocal));
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

void AppSkeleton::timestep(double absTime, double dt)
{
    for (std::vector<IScene*>::iterator it = m_scenes.begin();
        it != m_scenes.end();
        ++it)
    {
        IScene* pScene = *it;
        if (pScene != NULL)
        {
            pScene->timestep(absTime, dt);
        }
    }

    glm::vec3 hydraMove = glm::vec3(0.0f, 0.0f, 0.0f);
#ifdef USE_SIXENSE
    const sixenseAllControllerData& state = m_fm.GetCurrentState();
    for (int i = 0; i<2; ++i)
    {
        const sixenseControllerData& cd = state.controllers[i];
        const float moveScale = pow(10.0f, cd.trigger);
        hydraMove.x += cd.joystick_x * moveScale;

        const FlyingMouse::Hand h = static_cast<FlyingMouse::Hand>(i);
        if (m_fm.IsPressed(h, SIXENSE_BUTTON_JOYSTICK)) ///@note left hand does not work
            hydraMove.y += cd.joystick_y * moveScale;
        else
            hydraMove.z -= cd.joystick_y * moveScale;
    }
#endif

    glm::vec3 move_dt = (m_keyboardMove + m_joystickMove + m_mouseMove + hydraMove) * static_cast<float>(dt);

    // Move in the direction the viewer is facing.
    const glm::vec4 mv4 = getUserViewMatrix() * glm::vec4(move_dt, 0.0f);
    m_chassisPos += glm::vec3(mv4);

    m_chassisYaw += (m_keyboardYaw + m_joystickYaw + m_mouseDeltaYaw) * dt;

    m_fm.updateHydraData();
    m_hyif.updateHydraData(m_fm, 1.0f);

    const FlyingMouse::Hand h = FlyingMouse::Right;
    if (m_fm.ControllerIsOnBase(h) == false)
    {
        glm::vec3 origin3;
        glm::vec3 dir3;
        m_fm.GetControllerOriginAndDirection(h, origin3, dir3);

        const glm::mat4 chasMat = makeChassisMatrix_glm(m_chassisYaw, m_chassisPos);
        origin3 = glm::vec3(chasMat * glm::vec4(origin3, 1.f));
        dir3 = glm::vec3(chasMat * glm::vec4(dir3, 0.f));

        _checkSceneIntersections(origin3, dir3);
    }
}

void AppSkeleton::resize(int w, int h)
{
    m_rtSize.x = w;
    m_rtSize.y = h;
}

void AppSkeleton::OnMouseButton(int button, int action)
{
    if (action == 1) // glfw button press
    {
        if (button == 1) // glfw right click
        {
            const glm::vec3 sittingHeight = glm::vec3(0.f, 1.27f, 0.f);
            m_chassisPos = m_spaceCursorPos + sittingHeight;
        }
    }
}

void AppSkeleton::OnMouseMove(int x, int y)
{
    // Calculate eye space ray through mouse pointer
    const glm::vec2 uv01 = glm::vec2(
        static_cast<float>(x) / static_cast<float>(m_rtSize.x),
        1.f - static_cast<float>(y) / static_cast<float>(m_rtSize.y)
        );
    const glm::vec2 uv_11 = 2.f*uv01 - glm::vec2(1.f);

    const float aspect = static_cast<float>(m_rtSize.x) / static_cast<float>(m_rtSize.y);
    const float tanHalfFov = 4.f;

    const glm::vec3 fwd(0.f, 0.f, -1.f);
    const glm::vec3 rt(1.f, 0.f, 0.f);
    const glm::vec3 up(0.0f, 1.0f, 0.0f);

    const glm::vec3 localOrigin = glm::vec3(0.f);
    const glm::vec3 localRay = glm::normalize(
        aspect*fwd +
        tanHalfFov * uv_11.x * rt +
        tanHalfFov * uv_11.y * up);

    // Transform ray into world space
    const glm::mat4 mv = getUserViewMatrix();
    const glm::vec3 origin3 = glm::vec3(mv * glm::vec4(localOrigin,1.f));
    const glm::vec3 dir3 = glm::vec3(mv * glm::vec4(localRay,0.f));

    _checkSceneIntersections(origin3, dir3);
}

void AppSkeleton::OnMouseWheel(double x, double y)
{
    const float rotationIncrement = 30.f * M_PI / 180.f;
    m_chassisYaw += x * rotationIncrement;
}
