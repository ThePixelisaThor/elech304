#pragma once
#include <examples/libs/GL/glew.h>
const GLint WIDTH = 1920;
const GLint HEIGHT = 1080;

const int zone_count_x = 200;
const int zone_count_y = 140;
const int reflection_count = 3;
const int reflection_count_shown = 1;
const int shown_rays_at_once = 100;

const int line_count = 10; // for the ray casting
const float circle_radius = 7.f;

const float min_energy = -80.f + 3.333f; // 100 Mb/s
const float max_energy = -60.f; // 350 Mb/s

constexpr auto PI = 3.1415926535f;
