#pragma once
#include "wall.h"
#include "glm/glm.hpp"
#include "ray.h"

void compute_all_rays_hitting_rx_cpu(int reflection_count, Wall* walls, int wall_count, glm::vec2 tx, glm::vec2 rx, Ray* rays_hitting_rx, int& ray_count);
float compute_energy_cpu(Ray* rays_hitting, int& ray_count);
