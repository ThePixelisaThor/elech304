#include "fancy_vector.h"

FancyVector::FancyVector() {
}

FancyVector::FancyVector(glm::vec2 v1, glm::vec2 v2) {
    a = v1;
    b = v2;
    u = v2 - v1;
    n = glm::vec2(u.y, -u.x);
    normalized_u = glm::normalize(u);
    normalized_n = glm::normalize(n);
}
