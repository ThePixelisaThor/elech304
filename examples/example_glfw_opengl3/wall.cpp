#include "wall.h"
#include "glm/glm.hpp"
using namespace glm;

#include "../libs/GL/glew.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <vector>
#include <complex>

Wall::Wall(int _id, float _relative_perm, float _conductivity, float _pulsation, FancyVector _fancy_vector, float _depth, int _material_id) {
    id = _id;
    relative_perm = _relative_perm;
    conductivity = _conductivity;
    fancy_vector = _fancy_vector;
    depth = _depth;
    set_impedance(_pulsation);
    set_gamma(_pulsation);
    pulsation = _pulsation;
    material_id = _material_id;
}

void Wall::set_impedance(float pulsation) {
    impedance = compute_impedance(relative_perm, conductivity, pulsation);
}

void Wall::set_gamma(float pulsation) {
    gamma = compute_gamma(relative_perm, conductivity, pulsation);

    alpha = gamma.real();
    beta = gamma.imag();
}

std::complex<float> compute_impedance(float relative_perm, float conductivity, float pulsation) {
    return sqrt(std::complex<float> {4.f * 3.14159265f * (float) pow(10, -7), 0} / (std::complex<float> {8.854f * (float) pow(10, -12) * relative_perm, -conductivity / pulsation}));
}

std::complex<float> compute_gamma(float relative_perm, float conductivity, float pulsation) {
    return std::complex<float>(compute_alpha(relative_perm, conductivity, pulsation), compute_beta(relative_perm, conductivity, pulsation));
}

float compute_alpha(float relative_perm, float conductivity, float pulsation) {
    float alpha = pulsation * sqrt((4.f * 3.14159265f * pow(10, -7) * 8.854f * pow(10, -12) * relative_perm) / 2.0);
    alpha *= sqrt(sqrt(1.0 + pow(conductivity / (pulsation * 8.854f * pow(10, -12) * relative_perm), 2)) - 1.0);
    return alpha; // normalement le compilateur optimisera ce code pour éviter de créer une variable inutile
}

float compute_beta(float relative_perm, float conductivity, float pulsation) {
    float beta = pulsation * sqrt((4.f * 3.14159265f * pow(10, -7) * 8.854f * pow(10, -12) * relative_perm) / 2.0);
    beta *= sqrt(sqrt(1.0 + pow(conductivity / (pulsation * 8.854f * pow(10, -12) * relative_perm), 2)) + 1.0);
    return beta; // idem
}

