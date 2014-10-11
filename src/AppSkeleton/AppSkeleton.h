// AppSkeleton.h

#pragma once

#ifdef __APPLE__
#include "opengl/gl.h"
#endif

#ifdef _WIN32
#  define WINDOWS_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#endif

#include "FBO.h"
#include "Scene.h"
#include "HydraScene.h"
#include "OVRScene.h"

#include "FlyingMouse.h"
#include "VirtualTrackball.h"

class AppSkeleton
{
public:
    AppSkeleton();
    virtual ~AppSkeleton();

    void ResetAllTransformations();
    void initGL();
    void _DrawScenes(const float* pMview, const float* pPersp) const;
    void display_raw() const;
    void display_buffered(bool setViewport=true) const;
    void timestep(float dt);
    void resize(int w, int h);

public:
    // This public section is for exposing state variables to AntTweakBar
    Scene m_scene;
    HydraScene m_hydraScene;
    OVRScene m_ovrScene;

    float GetFBOScale() const { return m_fboScale; }
    void SetFBOScale(float s);
#ifdef USE_ANTTWEAKBAR
    float* GetFBOScalePointer() { return &m_fboScale; }
#endif

protected:
    void _initPresentFbo();
    void _resetGLState() const;
    void _drawSceneMono() const;
    virtual glm::ivec2 getRTSize() const { return m_rtSize; }
    virtual glm::mat4 getUserViewMatrix() const;

    std::vector<IScene*> m_scenes;
    FBO m_renderBuffer;
    float m_fboScale;
    ShaderWithVariables m_presentFbo;
    ShaderWithVariables m_presentDistMeshL;
    ShaderWithVariables m_presentDistMeshR;

    glm::vec3 m_chassisPos;
    float m_chassisYaw;

    VirtualTrackball m_hyif;

    glm::ivec2 m_rtSize;

public:
    FlyingMouse m_fm;
    glm::vec3 m_keyboardMove;
    glm::vec3 m_joystickMove;
    glm::vec3 m_mouseMove;
    float m_keyboardYaw;
    float m_joystickYaw;
    float m_mouseDeltaYaw;

private: // Disallow copy ctor and assignment operator
    AppSkeleton(const AppSkeleton&);
    AppSkeleton& operator=(const AppSkeleton&);
};
