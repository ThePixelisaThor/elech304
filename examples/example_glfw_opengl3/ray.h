#pragma once
#include "glm/glm.hpp"
using namespace glm;

#include "../libs/GL/glew.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <vector>
#include "render_utils.h"
#include <complex>

class Ray {
public:
    vec2 origin;
    vec2 end;
    vec2 direction = end - origin;
    float length = sqrt(pow(direction.x, 2) + pow(direction.y, 2));
    vec2 normalized_direction = normalize(direction);
    float start_energy; // relative, 1.0 is no loss
    float end_energy; // before reflection
    short reflection_count;
    short transmission_count; // one wall = one transmission
    short base_station_id;

public:
    void create(GLuint& VBO, GLuint& VAO, GLuint& CBO);

    void draw(GLuint& VBO, GLuint& CBO);
};
