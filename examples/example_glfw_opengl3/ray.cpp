#include "ray.h"
#include "glm/glm.hpp"
using namespace glm;

#include "../libs/GL/glew.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <vector>
#include "render_utils.h"
#include <complex>
#include "wall.h"
#include "coefficients.h"


void create_ray(GLuint& VBO, GLuint& VAO, GLuint& CBO, Ray ray, Color color) {
    create_line(ray.origin, ray.end, VBO, VAO, CBO, color);
}

void create_rays(GLuint*& VBO, GLuint*& VAO, GLuint*& CBO, std::vector<Ray> rays, int length, Color color) {
    for (int i = 0; i < length; i++)
        create_line(rays[i].origin, rays[i].end, VBO[i], VAO[i], CBO[i], color);
}

void Ray::create(GLuint& VBO, GLuint& VAO, GLuint& CBO, Color color) {
    create_ray(VBO, VAO, CBO, *this, color);
}

void Ray::create(GLuint& VBO, GLuint& VAO, GLuint& CBO) {
    /* create_ray(VBO, VAO, CBO, *this, Color{1 - energy, // hopefully this gives a good gradient
        (energy > 0.33f && energy < 0.66f) ? (energy - 0.33f) * 2 : energy / 2.f,
        (energy > 0.5f) ? (energy - 0.5f) * 2.f : 0.f });*/
    create_ray(VBO, VAO, CBO, *this, (energy > 0.75f) ? RED : (energy > 0.5f) ? YELLOW : (energy > 0.25f) ? GREEN : BLUE);
}

void Ray::draw(GLuint& VBO, GLuint& CBO) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    draw_array(VBO, CBO);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void Ray::create_draw(Color color) {
    unsigned int VBOs[10], VAOs[10], CBOs[10]; // je pense qu'il faut pas que ce soit un tableau, à fix
    create_arrays(VBOs, VAOs, CBOs, 10);

    create(VBOs[1], VAOs[1], CBOs[1], color);
    draw(VBOs[1], CBOs[1]);
}

void Ray::create_draw() {
    unsigned int VBOs[10], VAOs[10], CBOs[10]; // idem
    create_arrays(VBOs, VAOs, CBOs, 10);

    create(VBOs[1], VAOs[1], CBOs[1]); // la couleur change en fonction de l'énergie
    draw(VBOs[1], CBOs[1]);
}

coefficients compute_reflection_coefficients(float incident_angle_cos, std::complex<float> impedance_air, Wall wall) {

    float incident_angle_sin = sqrt(1 - incident_angle_cos * incident_angle_cos);
    float transmitted_angle_sin = sqrt(1.0 / wall.relative_perm) * incident_angle_sin; // snell descartes
    float transmitted_angle_cos = sqrt(1 - transmitted_angle_sin * transmitted_angle_sin);
    float transmitted_depth = wall.depth / transmitted_angle_cos; // transmitted angle cos should never be zero

    std::complex<float> one{ 1, 0 }; // 1 + 0j
    std::complex<float> j{ 0, 1 }; // 0 + 1j
    std::complex<float> two{ 2, 0 }; // 2 + 0j
    std::complex<float> reflection_coef = (wall.impedance * incident_angle_cos - impedance_air * transmitted_angle_cos) / (wall.impedance * incident_angle_cos + impedance_air * transmitted_angle_cos);
    std::complex<float> transmission_coef = (2.f * wall.impedance * incident_angle_cos) / (wall.impedance * incident_angle_cos + impedance_air * transmitted_angle_cos);
    std::complex<float> transmission_coef_reverse = (2.f * impedance_air * incident_angle_cos) / (wall.impedance * incident_angle_cos + impedance_air * transmitted_angle_cos);

    return coefficients{ reflection_coef, transmission_coef, transmission_coef_reverse };
}

float compute_total_transmission(float incident_angle_cos, coefficients c, Wall wall) {
    float incident_angle_sin = sqrt(1 - incident_angle_cos * incident_angle_cos);
    float transmitted_angle_sin = sqrt(1.0 / wall.relative_perm) * incident_angle_sin; // snell descartes
    float transmitted_angle_cos = sqrt(1 - transmitted_angle_sin * transmitted_angle_sin);
    float transmitted_depth = wall.depth / transmitted_angle_cos; // transmitted angle cos should never be zero
    std::complex<float> one{ 1, 0 }; // 1 + 0j
    std::complex<float> two{ 2, 0 }; // 2 + 0j
    std::complex<float> j{ 0, 1 }; // 0 + 1j

    std::complex<float> transmission_coef_after = c.transmission * c.transmission_reverse * exp(-wall.gamma * transmitted_depth);
    transmission_coef_after /= one - c.reflection * c.reflection * exp(-two * wall.gamma * transmitted_depth) * exp(j * wall.beta * two * transmitted_depth * transmitted_angle_sin * incident_angle_sin);

    std::complex<double> transmission_coef_squared = pow(transmission_coef_after, 2);

    return sqrt(pow(transmission_coef_squared.real(), 2) + pow(transmission_coef_squared.imag(), 2));
}
