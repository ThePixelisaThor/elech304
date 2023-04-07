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

float compute_reflection(vec2 tx, vec2 rx, FancyVector wall) {
    float t;

    float lengthLimit = glm::length(wall.u);
    vec2 s(tx.x - wall.a.x, tx.y - wall.a.y);
    vec2 k(rx.x - wall.a.x, rx.y - wall.a.y);

    if (!is_reflection_possible(s, k, wall.n)) return 0;

    vec2 tx_mirror(tx.x - 2 * glm::normalize(wall.n).x * dot(s, glm::normalize(wall.n)), tx.y - 2 * glm::normalize(wall.n).y * dot(s, glm::normalize(wall.n)));
    vec2 d(rx.x - tx_mirror.x, rx.y - tx_mirror.y);

    t = (d.y * (tx_mirror.x - wall.a.x) - d.x * (tx_mirror.y - wall.a.y)) / (glm::normalize(wall.u).x * d.y - glm::normalize(wall.u).y * d.x);

    //if (t < 0 || t > lengthLimit) return -1;

    return t;
}

void trace_rays(vec2 tx, vec2 rx, vector<FancyVector> walls, unsigned int VBO[], unsigned int VAO[], unsigned int CBO[]) {
    vector<float> positions;

    for (int i = 0; i < walls.size(); i++)
    {
        float pos = compute_reflection(tx, rx, walls[i]);
        positions.push_back(pos);
        vec2 reflex(walls[i].a.x + glm::normalize(walls[i].u).x * pos, walls[i].a.y + glm::normalize(walls[i].u).y * pos);
        create_line(tx, reflex, VBO[i * 2], VAO[i * 2], CBO[i * 2], GREEN);
        create_line(reflex, rx, VBO[i * 2 + 1], VAO[i * 2 + 1], CBO[i * 2 + 1], GREEN);
    }

    const int length = positions.size();
    cout<<length;

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    draw_arrays(VBO, CBO, length);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
