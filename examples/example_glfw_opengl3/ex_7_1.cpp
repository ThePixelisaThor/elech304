#include "ex_7_1.h"
#include <complex>
#include "wall.h"
#include "ray.h"
#include "coefficients.h"
#include "constants.h"

void compute_everything() {
    float pulsation = 2.f * PI * 2.45f * pow(10, 9);
    std::complex<float> Z0 = compute_impedance(1.f, 0, 1); // 377 ohms ✅
    std::complex<float> Z1 = compute_impedance(6.f, 0.01, pulsation); // 153.8 + 0.94j ✅

    Wall w(0, 6.f, 0.01f, pulsation, FancyVector {}, 0.3, 0, 0);

    coefficients c = compute_reflection_coefficients(1.f, Z0, w);
    // R should be 0.42 - j 0.0025 (opposite sign) ✅
    // T should be 0.58 + j 0.0025 ✅
    // T reverse should be 1.42 - j 0.0025 ✅

    float total_transmission = compute_total_transmission(1.f, c, w); // 0.54 ✅

    float alpha = compute_alpha(6.f, 0.01f, pulsation); // 0.77 ✅
    float beta = compute_beta(6.f, 0.01f, pulsation); // 125.7 ✅

}
