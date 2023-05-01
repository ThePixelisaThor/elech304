#pragma once

#include "render_utils.h"
#include <complex>

class Wall {
public:
    int id;
    float relative_perm;
    float conductivity;
    float pulsation;
    std::complex<float> gamma;
    float alpha;
    float beta;
    std::complex<float> impedance;
    FancyVector fancy_vector;
    float depth;
    int material_id;

    Wall(int id, float _relative_perm, float _conductivity, float _pulsation, FancyVector _fancy_vector, float _depth, int material_id);
    Wall() : Wall(0, 0.f, 0.f, 0.f, FancyVector(), 0.f, 0) {} // default constructor needed to create an array

    void set_impedance(float pulsation);

    void set_gamma(float pulsation);
};

std::complex<float> compute_impedance(float relative_perm, float conductivity, float pulsation);

std::complex<float> compute_gamma(float relative_perm, float conductivity, float pulsation);

float compute_alpha(float relative_perm, float conductivity, float pulsation);

float compute_beta(float relative_perm, float conductivity, float pulsation);
