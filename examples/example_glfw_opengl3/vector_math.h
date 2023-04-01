#pragma once
#include "glm/glm.hpp"
#include "fancy_vector.h"
#include <vector>
using namespace glm;

vec2 getIntersection(vec2 direction, vec2 center, std::vector<FancyVector> walls, FancyVector& wall, bool& found);
