// SpaceCursor.h

#pragma once

#ifdef _WIN32
#  define WINDOWS_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#endif
#include <stdlib.h>
#include <GL/glew.h>

#include <glm/glm.hpp>

#include "IScene.h"
#include "FlyingMouse.h"
#include "ShaderWithVariables.h"

///@brief 
class SpaceCursor
{
public:
    SpaceCursor();
    virtual ~SpaceCursor();

    virtual void initGL();
    virtual void RenderForOneEye(const float* pMview, const float* pPersp) const;

protected:
    void _InitOriginAttributes();
    void _DrawOrigin(int verts) const;
    void DrawScene(
        const glm::mat4& modelview,
        const glm::mat4& projection) const;

protected:
    ShaderWithVariables m_basic;

private: // Disallow copy ctor and assignment operator
    SpaceCursor(const SpaceCursor&);
    SpaceCursor& operator=(const SpaceCursor&);
};
