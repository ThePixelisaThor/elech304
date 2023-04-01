#include "fancy_vector.h"

FancyVector::FancyVector() {
}

FancyVector::FancyVector(glm::vec2 v1, glm::vec2 v2, glm::vec2 v3) {
    a = v1;
    b = v2;
    v = v3;
}
