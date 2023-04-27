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
#include "coefficients.h"
#include "wall.h"

class Ray {
public:
    vec2 origin;
    vec2 end;
    vec2 direction = end - origin;
    float length = sqrt(pow(direction.x, 2) + pow(direction.y, 2));
    vec2 normalized_direction = normalize(direction);
    float energy; // a ray transmitted through a wall creates a new ray
    short base_station_id = 1;
    float total_distance_travelled; // stores the distance of previous rays that created this one

    Ray(vec2 _origin, vec2 _end, float _energy, float _total_distance_travelled) {
        origin = _origin;
        end = _end;
        energy = _energy;
        total_distance_travelled = _total_distance_travelled;
    }
    Ray() : Ray(vec2(0, 0), vec2(0, 0), -1.f, 0.f) {} // default constructor to use with arrays

public:
    void create(GLuint& VBO, GLuint& VAO, GLuint& CBO, Color color);

    void create(GLuint& VBO, GLuint& VAO, GLuint& CBO);

    void draw(GLuint& VBO, GLuint& CBO);

    void create_draw(Color color);

    void create_draw();
};

coefficients compute_reflection_coefficients(float incident_angle_cos, std::complex<float> impedance_air, Wall wall);
float compute_total_transmission(float incident_angle_cos, coefficients c, Wall wall);
