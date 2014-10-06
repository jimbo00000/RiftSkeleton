// Scene.cpp

#include "Scene.h"

#ifdef __APPLE__
#include "opengl/gl.h"
#endif

#ifdef _WIN32
#  define WINDOWS_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#endif

#define _USE_MATH_DEFINES
#include <math.h>

#include <stdlib.h>
#include <string.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <GL/glew.h>

#include "Logger.h"

Scene::Scene()
: m_basic()
, m_plane()
, m_phaseVal(0.0f)
, m_cubeScale(1.0f)
, m_amplitude(1.0f)
{
}

Scene::~Scene()
{
}

void Scene::initGL()
{
    m_basic.initProgram("basic");
    m_basic.bindVAO();
    _InitCubeAttributes();
    glBindVertexArray(0);

    m_plane.initProgram("basicplane");
    m_plane.bindVAO();
    _InitPlaneAttributes();
    glBindVertexArray(0);
}

///@brief While the basic VAO is bound, gen and bind all buffers and attribs.
void Scene::_InitCubeAttributes()
{
    const glm::vec3 minPt(0,0,0);
    const glm::vec3 maxPt(1,1,1);
    const glm::vec3 verts[] = {
        minPt,
        glm::vec3(maxPt.x, minPt.y, minPt.z),
        glm::vec3(maxPt.x, maxPt.y, minPt.z),
        glm::vec3(minPt.x, maxPt.y, minPt.z),
        glm::vec3(minPt.x, minPt.y, maxPt.z),
        glm::vec3(maxPt.x, minPt.y, maxPt.z),
        maxPt,
        glm::vec3(minPt.x, maxPt.y, maxPt.z)
    };

    GLuint vertVbo = 0;
    glGenBuffers(1, &vertVbo);
    m_basic.AddVbo("vPosition", vertVbo);
    glBindBuffer(GL_ARRAY_BUFFER, vertVbo);
    glBufferData(GL_ARRAY_BUFFER, 8*3*sizeof(GLfloat), verts, GL_STATIC_DRAW);
    glVertexAttribPointer(m_basic.GetAttrLoc("vPosition"), 3, GL_FLOAT, GL_FALSE, 0, NULL);

    GLuint colVbo = 0;
    glGenBuffers(1, &colVbo);
    m_basic.AddVbo("vColor", colVbo);
    glBindBuffer(GL_ARRAY_BUFFER, colVbo);
    glBufferData(GL_ARRAY_BUFFER, 8*3*sizeof(GLfloat), verts, GL_STATIC_DRAW);
    glVertexAttribPointer(m_basic.GetAttrLoc("vColor"), 3, GL_FLOAT, GL_FALSE, 0, NULL);

    glEnableVertexAttribArray(m_basic.GetAttrLoc("vPosition"));
    glEnableVertexAttribArray(m_basic.GetAttrLoc("vColor"));

    const unsigned int quads[] = {
        0,3,2, 1,0,2, // ccw
        4,5,6, 7,4,6,
        1,2,6, 5,1,6,
        2,3,7, 6,2,7,
        3,0,4, 7,3,4,
        0,1,5, 4,0,5,
    };
    GLuint quadVbo = 0;
    glGenBuffers(1, &quadVbo);
    m_basic.AddVbo("elements", quadVbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadVbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 12*3*sizeof(GLuint), quads, GL_STATIC_DRAW);
}

///@brief While the basic VAO is bound, gen and bind all buffers and attribs.
void Scene::_InitPlaneAttributes()
{
    const glm::vec3 minPt(-10.0f, 0.0f, -10.0f);
    const glm::vec3 maxPt(10.0f, 0.0f, 10.0f);
    const float verts[] = {
        minPt.x, minPt.y, minPt.z,
        minPt.x, minPt.y, maxPt.z,
        maxPt.x, minPt.y, maxPt.z,
        maxPt.x, minPt.y, minPt.z,
    };
    GLuint vertVbo = 0;
    glGenBuffers(1, &vertVbo);
    m_plane.AddVbo("vPosition", vertVbo);
    glBindBuffer(GL_ARRAY_BUFFER, vertVbo);
    glBufferData(GL_ARRAY_BUFFER, 4*3*sizeof(GLfloat), verts, GL_STATIC_DRAW);
    glVertexAttribPointer(m_plane.GetAttrLoc("vPosition"), 3, GL_FLOAT, GL_FALSE, 0, NULL);

    const float texs[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
    };
    GLuint colVbo = 0;
    glGenBuffers(1, &colVbo);
    m_plane.AddVbo("vTexCoord", colVbo);
    glBindBuffer(GL_ARRAY_BUFFER, colVbo);
    glBufferData(GL_ARRAY_BUFFER, 4*2*sizeof(GLfloat), texs, GL_STATIC_DRAW);
    glVertexAttribPointer(m_plane.GetAttrLoc("vTexCoord"), 2, GL_FLOAT, GL_FALSE, 0, NULL);

    glEnableVertexAttribArray(m_plane.GetAttrLoc("vPosition"));
    glEnableVertexAttribArray(m_plane.GetAttrLoc("vTexCoord"));

    const unsigned int tris[] = {
        0,3,2, 1,0,2, // ccw
    };
    GLuint triVbo = 0;
    glGenBuffers(1, &triVbo);
    m_plane.AddVbo("elements", triVbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, triVbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 2*3*sizeof(GLuint), tris, GL_STATIC_DRAW);
}

// Draw an RGB color cube
void Scene::DrawColorCube() const
{
    m_basic.bindVAO();
    glDrawElements(GL_TRIANGLES,
                   6*3*2, // 6 triangle pairs
                   GL_UNSIGNED_INT,
                   0);
    glBindVertexArray(0);
}

/// Draw a circle of color cubes(why not)
void Scene::_DrawBouncingCubes(const glm::mat4& modelview) const
{
    const int numCubes = 12;
    for (int i=0; i<numCubes; ++i)
    {
        const float radius = 15.0f;
        const float posPhase = 2.0f * (float)M_PI * (float)i / (float)numCubes;
        const glm::vec3 cubePosition(radius * sin(posPhase), 0.0f, radius * cos(posPhase));

        const float frequency = 3.0f;
        const float amplitude = m_amplitude;
        float oscVal = amplitude * sin(frequency * (m_phaseVal + posPhase));

        glm::mat4 sinmtx = glm::translate(
            modelview,
            glm::vec3(cubePosition.x, oscVal, cubePosition.z));
        sinmtx = glm::scale(sinmtx, glm::vec3(m_cubeScale));

        glUniformMatrix4fv(m_basic.GetUniLoc("mvmtx"), 1, false, glm::value_ptr(sinmtx));
        DrawColorCube();
    }
}


void Scene::_DrawScenePlanes(const glm::mat4& modelview) const
{
    m_plane.bindVAO();
    {
        // floor
        glDrawElements(GL_TRIANGLES,
                       3*2, // 2 triangle pairs
                       GL_UNSIGNED_INT,
                       0);

        const float ceilHeight = 3.0f;
        glm::mat4 ceilmtx = glm::translate(
            modelview,
            glm::vec3(0.0f, ceilHeight, 0.0f));

        glUniformMatrix4fv(m_basic.GetUniLoc("mvmtx"), 1, false, glm::value_ptr(ceilmtx));

        // ceiling
        glDrawElements(GL_TRIANGLES,
                       3*2, // 2 triangle pairs
                       GL_UNSIGNED_INT,
                       0);
    }
    glBindVertexArray(0);
}


/// Draw the scene(matrices have already been set up).
void Scene::DrawScene(
    const glm::mat4& modelview,
    const glm::mat4& projection,
    const glm::mat4& object) const
{
    glUseProgram(m_plane.prog());
    {
        glUniformMatrix4fv(m_plane.GetUniLoc("mvmtx"), 1, false, glm::value_ptr(modelview));
        glUniformMatrix4fv(m_plane.GetUniLoc("prmtx"), 1, false, glm::value_ptr(projection));

        _DrawScenePlanes(modelview);
    }
    glUseProgram(0);

    glUseProgram(m_basic.prog());
    {
        glUniformMatrix4fv(m_basic.GetUniLoc("mvmtx"), 1, false, glm::value_ptr(modelview));
        glUniformMatrix4fv(m_basic.GetUniLoc("prmtx"), 1, false, glm::value_ptr(projection));

        _DrawBouncingCubes(modelview);

        (void)object;
#if 0
        glm::mat4 objectMatrix = modelview;
        objectMatrix = glm::translate(objectMatrix, glm::vec3(0.0f, 1.0f, 0.0f)); // Raise rotation center above floor
        // Rotate about cube center
        objectMatrix = glm::translate(objectMatrix, glm::vec3(0.5f));
        objectMatrix *= object;
        objectMatrix = glm::translate(objectMatrix, glm::vec3(-0.5f));
        glUniformMatrix4fv(m_basic.GetUniLoc("mvmtx"), 1, false, glm::value_ptr(objectMatrix));
        DrawColorCube();
#endif
    }
    glUseProgram(0);
}


void Scene::RenderForOneEye(const float* pMview, const float* pPersp) const
{
    if (m_bDraw == false)
        return;

    const glm::mat4 modelview = glm::make_mat4(pMview);
    const glm::mat4 projection = glm::make_mat4(pPersp);
    const glm::mat4 object = glm::mat4(1.0f);

    DrawScene(modelview, projection, object);
}

void Scene::timestep(float dt)
{
    m_phaseVal += dt;
}
