#pragma once
#include "wall.h"
#include "glm/glm.hpp"
#include "ray.h"

void compute_all_rays_hitting_tx_cpu(int reflection_count, Wall* walls, int wall_count, glm::vec2 tx, glm::vec2 rx, Ray* rays_hitting_rx, int& ray_count);
float compute_energy_cpu(Ray* rays_hitting, int& ray_count);
void generate_all_mirrors(int& reflection_count, glm::vec2* all_mirrors, int* each_reflection_count, int* walls_to_reflect,
    Wall* walls, int& wall_count, int& mirror_count, glm::vec2& tx, int& total_count);
void compute_ray_cpu(glm::vec2& tx, glm::vec2& rx, glm::vec2 direction, Ray& ray, bool& hit, Wall* walls, int wall_count,
    int* walls_to_reflect, float energy, float total_distance, int max_reflexion);
