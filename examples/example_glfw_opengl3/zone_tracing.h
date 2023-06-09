#pragma once
#include "vector_math.h"
#include "wall.h"
#include "ray.h"

void generate_ray_hitting_rx(vec2 tx, vec2 rx, std::vector<Wall>& walls, std::vector<Ray>& ray_hitting, int max_reflexion);
void compute_ray_zone(vec2 tx, vec2 rx, vec2 direction, vec2 ray_origin, std::vector<Ray>& rays_hitting, std::vector<Wall> &walls, std::vector<int>& walls_to_reflect, float start_energy, float total_distance, int reflexion_left, int max_reflexion);
float compute_energy(std::vector<Ray>& rays_hitting);
