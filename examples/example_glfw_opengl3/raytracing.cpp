#include <math.h>

#include "raytracing.h"
#include "render_utils.h"
#include "fancy_vector.h"

bool is_reflection_possible(vec2 s, vec2 k, vec2 n) {
    return (sign(dot(s, n)) == sign(dot(k, n)));
}

float compute_reflection(vec2 tx, vec2 rx, FancyVector wall) {
    float t;

    float lengthLimit = glm::length(wall.u);
    vec2 s(tx.x - wall.a.x, tx.y - wall.a.y);
    vec2 k(rx.x - wall.a.x, rx.y - wall.a.y);

    if (!is_reflection_possible(s, k, wall.n)) return -1;

    vec2 tx_mirror(tx.x - 2 * wall.n.x * dot(s, wall.n), tx.y - 2 * wall.n.y * dot(s, wall.n));
    vec2 d(rx.x - tx_mirror.x, rx.y - tx_mirror.y);

    t = (d.y * (tx_mirror.x - wall.a.x) - d.x * (tx_mirror.y - wall.a.y)) / (wall.u.x * d.y - wall.u.y * d.x);

    if (t < 0 || t > lengthLimit) return -1;

    return t;
}

void trace_rays(vec2 tx, vec2 rx, std::vector<FancyVector> walls) {
    std::vector<float> positions;

    for (int i = 0; i < walls.size(); i++)
    {
        float pos = compute_reflection(tx, rx, walls[i]);
        if (pos >= 0) {
            positions.push_back(pos);
        }
    }

    const int count = positions.size();

    //TODO
}
