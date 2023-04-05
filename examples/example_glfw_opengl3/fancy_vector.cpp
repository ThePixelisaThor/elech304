#include "fancy_vector.h"

FancyVector::FancyVector() {
}

FancyVector::FancyVector(glm::vec2 v1, glm::vec2 v2) {
    a = v1;
    b = v2;
    // u = glm::normalize(v2 - v1);
    u = v2 - v1;
    n = glm::vec2(u.y, -u.x);
}
