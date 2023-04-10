#pragma once
#include <complex>

struct coefficients {
    std::complex<float> reflection;
    std::complex<float> transmission; // of the ray hitting the wall, fraction that goes through but not what hits the other side
    std::complex<float> transmission_reverse; // of the ray hitting the end of the wall from inside
};
