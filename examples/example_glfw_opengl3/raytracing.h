#pragma once
#include "vector_math.h"
#include "wall.h"
#include "ray.h"
#include <set>
#include <unordered_set>

bool is_reflection_possible(vec2 s, vec2 k, vec2 n);
float compute_reflection(vec2 tx, vec2 rx, FancyVector wall, bool& found);
vec2 getMirrorWithWall(vec2 point, FancyVector wall);
void generate_rays_direction(vec2 tx, vec2 rx, vec2 fake_rx, std::vector<Wall> walls, int previous,
    std::vector<Ray>& all_rays, std::vector<int> walls_to_reflect, std::vector<vec2> &fake_rx_done, int max_reflexion, int counter);
vec2 getIntersection(vec2 direction, vec2 origin, std::vector<Wall> walls, Wall& wall_, bool& found, float min_distance);
void compute_ray(vec2 tx, vec2 rx, vec2 direction, vec2 ray_origin, std::vector<Ray>& all_rays, std::vector<Ray>& buffer, std::vector<Wall> walls, std::vector<int>& walls_to_reflect, float start_energy, float total_distance, int reflexion_left, int max_reflexion);
void generate_new_rays(vec2 tx, vec2 rx, vec2 hit_point, vec2 direction_before, std::vector<Wall> walls, Wall wall, std::vector<Ray>& all_rays, std::vector<Ray>& buffer, std::vector<int>& walls_to_reflect, float start_energy, float total_distance, int reflexion_left, int max_reflexion);
