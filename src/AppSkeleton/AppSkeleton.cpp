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
}

void AppSkeleton::ResetAllTransformations()
{
    m_chassisPos.x = 0.0f;
    m_chassisPos.y = 1.27f; // my sitting height
    m_chassisPos.z = 1.0f;
    m_chassisYaw = 0.0f;
}
