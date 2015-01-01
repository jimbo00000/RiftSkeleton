// SpaceCursor.cpp

#include "SpaceCursor.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <vector>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

SpaceCursor::SpaceCursor()
: m_basic()
{
}

SpaceCursor::~SpaceCursor()
{
}

void SpaceCursor::initGL()
{
    m_basic.initProgram("basic");
    m_basic.bindVAO();
    _InitOriginAttributes();
    glBindVertexArray(0);
}

///@brief While the basic VAO is bound, gen and bind all buffers and attribs.
void SpaceCursor::_InitOriginAttributes()
{
    const glm::vec3 verts[] = {
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),

        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),

        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f),
    };

    const glm::vec3 cols[] = {
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),

        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),

        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::vec3(0.0f, 0.0f, 1.0f),
    };

    GLuint vertVbo = 0;
    glGenBuffers(1, &vertVbo);
    m_basic.AddVbo("vPosition", vertVbo);
    glBindBuffer(GL_ARRAY_BUFFER, vertVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts)*3*sizeof(GLfloat), verts, GL_STATIC_DRAW);
    glVertexAttribPointer(m_basic.GetAttrLoc("vPosition"), 3, GL_FLOAT, GL_FALSE, 0, NULL);

    GLuint colVbo = 0;
    glGenBuffers(1, &colVbo);
    m_basic.AddVbo("vColor", colVbo);
    glBindBuffer(GL_ARRAY_BUFFER, colVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cols)*3*sizeof(GLfloat), cols, GL_STATIC_DRAW);
    glVertexAttribPointer(m_basic.GetAttrLoc("vColor"), 3, GL_FLOAT, GL_FALSE, 0, NULL);

    glEnableVertexAttribArray(m_basic.GetAttrLoc("vPosition"));
    glEnableVertexAttribArray(m_basic.GetAttrLoc("vColor"));
}

void SpaceCursor::_DrawOrigin(int verts) const
{
    m_basic.bindVAO();
    glDrawArrays(GL_LINES, 0, verts);
    glBindVertexArray(0);
}

// Draw the scene(matrices have already been set up).
void SpaceCursor::DrawScene(
    const glm::mat4& modelview,
    const glm::mat4& projection) const
{
    glUseProgram(m_basic.prog());
    {
        glUniformMatrix4fv(m_basic.GetUniLoc("mvmtx"), 1, false, glm::value_ptr(modelview));
        glUniformMatrix4fv(m_basic.GetUniLoc("prmtx"), 1, false, glm::value_ptr(projection));

        _DrawOrigin(6);
    }
    glUseProgram(0);
}

void SpaceCursor::RenderForOneEye(const float* pMview, const float* pPersp) const
{
    const glm::mat4 modelview = glm::make_mat4(pMview);
    const glm::mat4 projection = glm::make_mat4(pPersp);

    DrawScene(modelview, projection);
}
