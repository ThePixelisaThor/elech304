#include "color.h"

Color getGradientColor(float intensity, float min_intensity, float max_intensity) {
    Color c;

    float intensity_deg = 300.f - (intensity - min_intensity) / (max_intensity - min_intensity) * 300.f;

    // red
    if (intensity_deg < 0.f) {
        c = WHITE;
    }
    else if (intensity_deg >= 0.f && intensity_deg < 60.f) {
        c.r = 1.0f;
        c.g = intensity_deg / 60.f;
        c.b = 0.f;
    }
    else if (intensity_deg >= 60.f && intensity_deg < 120.f) {
        c.r = (120.f - intensity_deg) / 60.f;
        c.g = 1.0f;
        c.b = 0.f;
    }
    else if (intensity_deg >= 120.f && intensity_deg < 180.f) {
        c.r = 0.f;
        c.g = 1.f;
        c.b = (intensity_deg - 120.f) / 60.f;
    }
    else if (intensity_deg >= 180.f && intensity_deg < 240.f) {
        c.r = 0.f;
        c.g = (240.f - intensity_deg) / 60.f;
        c.b = 1.f;
    }
    else if (intensity_deg >= 240.f && intensity_deg <= 300.f) {
        c.r = (intensity - 240.f) / 60.f;
        c.g = 0.f;
        c.b = 1.f;
    }
    else {
        c = BLACK;
    }

    return c;
}
