#pragma once
#include "glm/glm.hpp"

class FancyVector {
public:
    glm::vec2 a;
    glm::vec2 b;
    glm::vec2 v;

    FancyVector();
    FancyVector(glm::vec2 a, glm::vec2 b, glm::vec2 v);
};
