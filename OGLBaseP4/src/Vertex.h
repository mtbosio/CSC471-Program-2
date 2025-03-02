#pragma once

#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;

    Vertex(glm::vec3 pos, glm::vec3 norm, glm::vec2 tex)
        : position(pos), normal(norm), texCoord(tex) {}
};
