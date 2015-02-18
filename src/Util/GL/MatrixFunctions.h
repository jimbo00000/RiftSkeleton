// MatrixFunctions.h

#pragma once

#include <glm/glm.hpp>
#include <OVR.h>

glm::mat4 makeChassisMatrix_glm(
    float chassisYaw,
    glm::vec3 chassisPos);

OVR::Matrix4f makeOVRMatrixFromGlmMatrix(const glm::mat4& gm);
