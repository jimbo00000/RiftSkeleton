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
}

AppSkeleton::~AppSkeleton()
{
}
