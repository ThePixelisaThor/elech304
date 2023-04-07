#include <stdio.h>
#include <math.h>

#include "raytracing.h"
#include "render_utils.h"
#include "fancy_vector.h"
#include "color.h"

using namespace std;

bool is_reflection_possible(vec2 s, vec2 k, vec2 n) {
    return (sign(dot(s, n)) == sign(dot(k, n)));
}

float compute_reflection(vec2 tx, vec2 rx, FancyVector wall, bool &found) {
    float t;

    float lengthLimit = glm::length(wall.u);
    vec2 s(tx.x - wall.a.x, tx.y - wall.a.y);
    vec2 k(rx.x - wall.a.x, rx.y - wall.a.y);

    if (!is_reflection_possible(s, k, wall.n)) {
        found = false;
        return 0;
    }

    vec2 tx_mirror(tx.x - 2 * glm::normalize(wall.n).x * dot(s, glm::normalize(wall.n)), tx.y - 2 * glm::normalize(wall.n).y * dot(s, glm::normalize(wall.n)));
    vec2 d(rx.x - tx_mirror.x, rx.y - tx_mirror.y);

    t = (d.y * (tx_mirror.x - wall.a.x) - d.x * (tx_mirror.y - wall.a.y)) / (glm::normalize(wall.u).x * d.y - glm::normalize(wall.u).y * d.x);

    if (t < 0.01 || t > lengthLimit - 0.01) found = false;

    return t;
}

void trace_rays(vec2 tx, vec2 rx, vector<FancyVector> walls, unsigned int *VBO, unsigned int *VAO, unsigned int *CBO) {
    vector<float> positions;
    int j = 0;
    for (int i = 0; i < walls.size(); i++)
    {
        bool found = true;
        float pos = compute_reflection(tx, rx, walls[i], found);
        if (!found) continue;

        positions.push_back(pos);
        vec2 reflex(walls[i].a.x + glm::normalize(walls[i].u).x * pos, walls[i].a.y + glm::normalize(walls[i].u).y * pos);
        create_line(tx, reflex, VBO[j], VAO[j], CBO[j], GREEN);
        create_line(reflex, rx, VBO[j + 1], VAO[j + 1], CBO[j + 1], GREEN);
        j += 2;
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    draw_arrays(VBO, CBO, j);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
