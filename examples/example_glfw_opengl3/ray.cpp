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


void create_ray(GLuint& VBO, GLuint& VAO, GLuint& CBO, Ray ray, Color color) {
    create_line(ray.origin, ray.end, VBO, VAO, CBO, color);
}

void create_rays(GLuint*& VBO, GLuint*& VAO, GLuint*& CBO, std::vector<Ray> rays, int length, Color color) {
    for (int i = 0; i < length; i++)
        create_line(rays[i].origin, rays[i].end, VBO[i], VAO[i], CBO[i], color);
}

void Ray::create(GLuint& VBO, GLuint& VAO, GLuint& CBO) {
    create_ray(VBO, VAO, CBO, *this, Color{ 1 - end_energy, // hopefully this gives a good gradient
        (end_energy > 0.33f && end_energy < 0.66f) ? (end_energy - 0.33f) * 2 : end_energy / 2.f,
        (end_energy > 0.5f) ? (end_energy - 0.5f) * 2.f : 0.f });
}

void Ray::draw(GLuint& VBO, GLuint& CBO) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    draw_array(VBO, CBO);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

struct coefficients {
    std::complex<float> reflection;
    std::complex<float> transmission;
};

coefficients compute_reflection_coefficients(float incident_angle_cos, float wall_relative_perm, float wall_depth,
                                    std::complex<float> impedance_air, std::complex<float> impedance_wall) {

    float incident_angle_sin = sqrt(1 - incident_angle_cos * incident_angle_cos);
    float transmitted_angle_sin = sqrt(1.0 / wall_relative_perm) * incident_angle_sin; // snell descartes
    float transmitted_angle_cos = sqrt(1 - transmitted_angle_sin * transmitted_angle_sin);
    float transmitted_depth = wall_depth / transmitted_angle_cos; // transmitted angle cos should never be zero

    float alpha;
    float beta; std::complex<float> beta_complex{ 0, beta };
    std::complex<float> gamma{ alpha, beta };
    std::complex<float> one{ 1, 0 }; // 1 + 0j
    std::complex<float> j{ 0, 1 }; // 0 + 1j
    std::complex<float> two{ 2, 0 }; // 2 + 0j

    // perpendicular polarisation
    std::complex<float> reflection_coef = (impedance_wall * incident_angle_cos - impedance_air * transmitted_angle_cos) / (impedance_wall * incident_angle_cos + impedance_air * transmitted_angle_cos);
    std::complex<float> squared_reflection_coef = reflection_coef * reflection_coef;
    // plane wave, result of a convering infinite serie
    std::complex<float> transmission_coef = (one - squared_reflection_coef)* exp(-gamma * transmitted_depth);
    transmission_coef /= (one - squared_reflection_coef) * exp(-two * gamma * transmitted_depth) * exp(j * beta_complex * two * transmitted_depth * transmitted_angle_sin * transmitted_angle_cos);

    return coefficients{ reflection_coef, transmission_coef };
}

