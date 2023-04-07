#pragma once
#include "vector_math.h"

bool is_reflection_possible(vec2 s, vec2 k, vec2 n);
float compute_reflection(vec2 tx, vec2 rx, FancyVector wall);
void trace_rays(vec2 tx, vec2 rx, std::vector<FancyVector> walls, unsigned int VBO[], unsigned int VAO[], unsigned int CBO[]);
