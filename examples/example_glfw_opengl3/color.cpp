#include "color.h"

Color getColorForValue(float value, float minValue, float maxValue) {
    if (value < minValue) {
        // Return a default color if the value is outside the range
        return BLACK; // Gray color
    }

    if (value > maxValue) {
        return WHITE;
    }

    // Calculate the value's position within the range [minValue, maxValue]
    float range = maxValue - minValue;
    float valuePosition = (value - minValue) / range;

    // Calculate the RGB components of the color based on the value's position
    int red, green, blue;
    if (valuePosition < 0.5f) {
        red = 0;
        green = 255 * (valuePosition / 0.5f);
        blue = 255 - green;
    }
    else {
        blue = 0;
        red = 255 * ((valuePosition - 0.5f) / 0.5f);
        green = 255 - red;
    }

    // Return the calculated color
    return Color{ red / 255.f, green / 255.f, blue / 255.f};
}
