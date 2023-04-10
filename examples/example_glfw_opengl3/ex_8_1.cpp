#include "ex_8_1.h"
#include <complex>
#include "wall.h"
#include "ray.h"
#include "coefficients.h"

void compute_everything_8() {
    float pulsation = 2.f * 3.141592f * 868.3f * pow(10, 6);
    std::complex<float> Z0 = compute_impedance(1.f, 0, 1); // 377 ohms ✅
    std::complex<float> Z1 = compute_impedance(4.8f, 0.018, pulsation); // 171.57 + i*6.65 ✅

    Wall w(4.8f, 0.018f, pulsation, FancyVector{}, 0.15);

    coefficients c = compute_reflection_coefficients(0.9648f, Z0, w);

    float total_transmission = compute_total_transmission(1.f, c, w);
    // la valeur de transmission_coef_after est très proche de celle du corrigé (0.67 + 0.25i) vs (0.69 + 0.23i)
    // donc je suppose que ce sont les arrondis ?

    float alpha = compute_alpha(4.8f, 0.018f, pulsation); // 1.55 ✅
    float beta = compute_beta(4.8f, 0.018f, pulsation); // 39.9 ✅

}
