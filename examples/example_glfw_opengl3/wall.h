#pragma once

#include "render_utils.h"
#include <complex>

class Wall {
public:
    float relative_perm;
    float conductivity;
    std::complex<float> gamma;
    float alpha;
    float beta;
    std::complex<float> impedance;
    FancyVector fancy_vector;
    float depth;

    Wall(float _relative_perm, float _conductivity, float _pulsation, FancyVector _fancy_vector, float _depth);

    void set_impedance(float pulsation);

    void set_gamma(float pulsation);
};

std::complex<float> compute_impedance(float relative_perm, float conductivity, float pulsation);

std::complex<float> compute_gamma(float relative_perm, float conductivity, float pulsation);

float compute_alpha(float relative_perm, float conductivity, float pulsation);

float compute_beta(float relative_perm, float conductivity, float pulsation);
