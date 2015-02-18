// MatrixFunctions.cpp

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <OVR.h>

glm::mat4 makeChassisMatrix_glm(
    float chassisYaw,
    glm::vec3 chassisPos)
{
    return
        glm::translate(glm::mat4(1.f), chassisPos) *
        glm::rotate(glm::mat4(1.f), -chassisYaw, glm::vec3(0,1,0));
}

///@return an equivalent OVR matrix to the given glm one.
/// OVR uses DX's left-handed convention, so transpose is necessary.
OVR::Matrix4f makeOVRMatrixFromGlmMatrix(const glm::mat4& glm_m)
{
    OVR::Matrix4f ovr_m;
    memcpy(
        reinterpret_cast<float*>(&ovr_m.M[0][0]),
        glm::value_ptr(glm::transpose(glm_m)),
        16*sizeof(float));
    return ovr_m; // copied on return
}
