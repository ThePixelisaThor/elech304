#include "glm/glm.hpp"
#include <vector>
#include "fancy_vector.h"
#include "raytracing.h"

using namespace glm;

vec2 getIntersection(vec2 direction, vec2 center, std::vector<FancyVector> walls, FancyVector& wall, bool& found) {
    float t = 0;
    float s = 0;
    // calculating intersection
    vec2 best_intersection(-1000, -1000);
    FancyVector best_wall;
    double best_distance = 1000000;
    double distance = 0;

    for (int w = 0; w < walls.size(); w++) {
        wall = walls[w];
        if (wall.u.y == 0) {
            t = (wall.a.y - center.y) / direction.y;
            s = (center.x + direction.x * t - wall.a.x) / wall.u.x;
        }
        else {
            t = wall.u.y / (direction.x * wall.u.y - wall.u.x * direction.y) * (wall.a.x - center.x + wall.u.x * (center.y - wall.a.y) / wall.u.y);
            s = (center.y + direction.y * t - wall.a.y) / wall.u.y;
        }

        if (t <= 0.1 || t > 100000 || s < 0 || s > 1) continue;

        vec2 intersection = vec2(center.x + t * direction.x, center.y + t * direction.y);
        distance = length(intersection - center);

        if (distance < best_distance) {
            best_distance = distance;
            best_intersection = intersection;
            best_wall = wall;
        }
    }

    if (best_intersection.x == -1000 && best_intersection.y == -1000) found = false;

    wall = best_wall;
    return best_intersection;
}
