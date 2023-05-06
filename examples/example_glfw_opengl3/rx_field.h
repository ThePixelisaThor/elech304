#pragma once
#include "wall.h"
#include "glm/glm.hpp"
#include "ray.h"

void compute_all_rays_hitting_tx_fields(int reflection_count, Wall* walls, int wall_count, glm::vec2 tx, glm::vec2 rx, Ray* rays_hitting_rx, int& ray_count);
float compute_energy_fields(std::vector<Ray>& rays_hitting, float pulsation, float equivalent_resistance);
void compute_ray_fields(glm::vec2& tx, glm::vec2& rx, glm::vec2 direction, Ray& ray, bool& hit, Wall* walls, int wall_count,
    int* walls_to_reflect, std::complex<float> energy, float total_distance, int max_reflexion);
