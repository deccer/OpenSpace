#pragma once

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

struct TTransformComponent {
    glm::vec3 LocalPosition;
    glm::quat LocalOrientation;
    glm::vec3 LocalScale;
};
