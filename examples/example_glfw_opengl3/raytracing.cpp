#include <stdio.h>
#include <math.h>

#include "raytracing.h"
#include "render_utils.h"
#include "fancy_vector.h"
#include "color.h"
#include "ray.h"

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

    vec2 tx_mirror = getMirrorWithWall(tx, wall);
    vec2 d(rx.x - tx_mirror.x, rx.y - tx_mirror.y);

    t = (d.y * (tx_mirror.x - wall.a.x) - d.x * (tx_mirror.y - wall.a.y)) / (glm::normalize(wall.u).x * d.y - glm::normalize(wall.u).y * d.x);

    if (t < 0 || t > lengthLimit) found = false;

    return t;
}

vec2 getMirrorWithWall(vec2 point, FancyVector wall) {
    vec2 s(point.x - wall.a.x, point.y - wall.a.y);
    return vec2(point.x - 2 * glm::normalize(wall.n).x * dot(s, glm::normalize(wall.n)), point.y - 2 * glm::normalize(wall.n).y * dot(s, glm::normalize(wall.n)));
}

void generate_new_rays(vec2 tx, vec2 rx, vec2 hit_point, vec2 direction_before, vector<Wall> walls, Wall wall, vector<Ray>& all_rays, vector<Ray>& buffer, vector<int>& sequence, float start_energy, float total_distance, int reflexion_left) {
    // when a ray hits a wall, two rays are created
    vec2 normalized_direction = glm::normalize(direction_before);

    float incident_cos = abs(glm::dot(glm::normalize(wall.fancy_vector.n), normalized_direction)); // pour que l'angle soit entre -90 et 90

    // reflected ray
    complex<float> air_impedance = compute_impedance(1.f, 0, 1);
    coefficients cs = compute_reflection_coefficients(incident_cos, air_impedance, wall);
    vec2 reflection_vector = direction_before - 2.f * glm::dot(direction_before,
                                glm::normalize(vec2(wall.fancy_vector.u.y, -wall.fancy_vector.u.x))) * glm::normalize(vec2(wall.fancy_vector.u.y, -wall.fancy_vector.u.x));
    float reflection_coef = sqrt(pow(cs.reflection.real(), 2) + pow(cs.reflection.imag(), 2));
    if (reflection_coef >= 1.f || reflection_coef < 0.f) {
        //breakpoint
        cout << "problem" << endl;
    }
    if (reflexion_left != 0)
    compute_ray(tx, rx, reflection_vector, hit_point, all_rays, buffer, walls, sequence, start_energy * reflection_coef, total_distance, reflexion_left - 1);

    // transmitted ray
    float transmission = compute_total_transmission(incident_cos, cs, wall);
    compute_ray(tx, rx, direction_before, hit_point, all_rays, buffer, walls, sequence, start_energy * transmission, total_distance, reflexion_left);
}

void compute_ray(vec2 tx, vec2 rx, vec2 direction, vec2 ray_origin, vector<Ray> &all_rays, vector<Ray> &buffer, vector<Wall> walls, vector<int>& sequence, float start_energy, float total_distance, int reflexion_left) {
    if (reflexion_left == -1) return;
    // is hitting RX
    vec2 direction_to_rx = normalize(rx - ray_origin);
    vec2 direction_normalized = normalize(direction);

    if (direction_normalized.x < direction_to_rx.x + 0.000001 && direction_normalized.x > direction_to_rx.x - 0.000001 &&
        direction_normalized.y < direction_to_rx.y + 0.000001 && direction_normalized.y > direction_to_rx.y - 0.000001) {
        float distance_to_rx = length(rx - ray_origin);
        // a wall could be between the ray reflex and RX
        bool found_wall_intersect_before_rx = true;
        Wall best_wall = walls[0];

        vec2 hit_point = getIntersection(direction, ray_origin, walls, best_wall, sequence, found_wall_intersect_before_rx, distance_to_rx);

        if (found_wall_intersect_before_rx) {
            vector<Ray> buffer2(buffer);
            buffer2.push_back(Ray(ray_origin, hit_point, start_energy, total_distance + length(ray_origin - hit_point)));
            return generate_new_rays(tx, rx, hit_point, direction, walls, best_wall, all_rays, buffer2, sequence, start_energy, total_distance, reflexion_left);
        }

        buffer.push_back(Ray(ray_origin, rx, start_energy, total_distance + distance_to_rx));
        for (Ray r : buffer) { // adds the rays only if they actually hit RX
            all_rays.push_back(r);
        }
        
        return;
    }

    bool found = true;
    Wall best_wall = walls[0];
    vec2 hit_point = getIntersection(direction, ray_origin, walls, best_wall, sequence, found, 100000);

    if (found) {
        // the ray hit a wall
        vector<Ray> buffer2(buffer);
        buffer2.push_back(Ray(ray_origin, hit_point, start_energy, total_distance + length(hit_point - ray_origin)));
        // copies the vector
        return generate_new_rays(tx, rx, hit_point, direction, walls, best_wall, all_rays, buffer2, sequence, start_energy, total_distance, reflexion_left);
    }
}

void generate_rays_direction(vec2 tx, vec2 rx, vec2 fake_rx, vector<Wall> walls, int previous, vector<int>& sequence, vector<Ray> &all_rays, int max_reflexion, int counter) {
    vector<float> positions;
    counter++;
    for (int i = previous + 1; i < walls.size(); i++)
    {
        bool found = true;
        float pos = compute_reflection(tx, fake_rx, walls[i].fancy_vector, found);
        if (!found) continue;

        positions.push_back(pos);
        vec2 reflex(walls[i].fancy_vector.a.x + glm::normalize(walls[i].fancy_vector.u).x * pos, walls[i].fancy_vector.a.y + glm::normalize(walls[i].fancy_vector.u).y * pos);

        // a direction has been found
        vec2 direction = reflex - tx;
        vector<Ray> buffer;
        sequence.push_back(previous);
        compute_ray(tx, rx, direction, tx, all_rays, buffer, walls, sequence, 1.0f, direction.length(), max_reflexion - counter + 2);
    }

    if (counter == max_reflexion) return;

    for (int i = previous + 1; i < walls.size(); i++)
    {
        generate_rays_direction(tx, rx, getMirrorWithWall(fake_rx, walls[i].fancy_vector), walls, i, sequence, all_rays, max_reflexion, counter);
    }

}

vec2 getIntersection(vec2 direction, vec2 origin, vector<Wall> walls, Wall &wall_, vector<int>& sequence, bool& found, float min_distance) {
    float t = 0;
    float s = 0;
    // calculating intersection
    vec2 best_intersection(-1000, -1000);
    Wall best_wall = walls[0];
    double distance = 0;

    for (int w = 0; w < walls.size(); w++) {

        if (std::binary_search(sequence.begin(), sequence.end(), w)) {
            wall_ = walls[w];
            FancyVector wall = wall_.fancy_vector;
            if (wall.u.y == 0) {
                t = (wall.a.y - origin.y) / direction.y;
                s = (origin.x + direction.x * t - wall.a.x) / wall.u.x;
            }
            else {
                t = wall.u.y / (direction.x * wall.u.y - wall.u.x * direction.y) * (wall.a.x - origin.x + wall.u.x * (origin.y - wall.a.y) / wall.u.y);
                s = (origin.y + direction.y * t - wall.a.y) / wall.u.y;
            }

            if (t < 0.01 || t > 100000 || s < 0.01 || s > 1) continue;

            vec2 intersection = vec2(origin.x + t * direction.x, origin.y + t * direction.y);
            distance = length(intersection - origin);

            if (distance < min_distance) {
                min_distance = distance;
                best_intersection = intersection;
                best_wall = wall_;
            }
        }
       
    }

    if (best_intersection.x == -1000 && best_intersection.y == -1000) found = false;
    wall_ = best_wall;
    return best_intersection;
}
