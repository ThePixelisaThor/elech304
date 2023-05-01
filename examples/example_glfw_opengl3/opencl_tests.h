#pragma once
#include "glm/glm.hpp"
#include "wall.h"

void test_opencl();
void run_algo(int zone_count_x, int zone_count_y, int reflection_count, Wall walls[], int wall_count, glm::vec2 tx);
void run_final_algo(int zone_count_x, int zone_count_y, int reflection_count, Wall walls[], int wall_count, glm::vec2 tx, float pulsation,
    float* energy_pointer, int* ray_count_pointer);
void run_algo_antenna(int zone_count_x, int zone_count_y, int reflection_count, Wall walls[], int wall_count, glm::vec2 tx, float pulsation, float* energy,
    int* ray_count, int antenna_id, float antenna_angle);
