#include "wall.h"
#include "glm/glm.hpp"
using namespace glm;

#include "../libs/GL/glew.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <vector>
#include <complex>

Wall::Wall(float _relative_perm, float _conductivity, float _pulsation, FancyVector _fancy_vector, float _depth) {
    relative_perm = _relative_perm;
    conductivity = _conductivity;
    fancy_vector = _fancy_vector;
    depth = _depth;
    set_impedance(_pulsation);
    set_gamma(_pulsation);
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
    float alpha = pulsation * sqrt((4.f * 3.14159265f * pow(10, -7) * 8.854f * pow(10, -12) * relative_perm) / 2);
    alpha *= sqrt(sqrt(1 + pow(conductivity / (pulsation * 8.854f * pow(10, -12) * relative_perm), 2)) - 1);
    return alpha; // normalement le compilateur optimisera ce code pour éviter de créer une variable inutile
}

float compute_beta(float relative_perm, float conductivity, float pulsation) {
    float beta = pulsation * sqrt((4.f * 3.14159265f * pow(10, -7) * 8.854f * pow(10, -12) * relative_perm) / 2);
    beta *= sqrt(sqrt(1 + pow(conductivity / (pulsation * 8.854f * pow(10, -12) * relative_perm), 2)) + 1);
    return beta; // idem
}

