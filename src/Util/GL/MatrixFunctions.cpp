// MatrixFunctions.cpp

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

glm::mat4 makeChassisMatrix_glm(
    float chassisYaw,
    glm::vec3 chassisPos)
{
    return
        glm::translate(glm::mat4(1.f), chassisPos) *
        glm::rotate(glm::mat4(1.f), -chassisYaw, glm::vec3(0,1,0));
}
