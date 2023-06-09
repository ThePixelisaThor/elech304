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

float compute_reflection(vec2 tx, vec2 rx, FancyVector wall, bool& found) {
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
    vec2 result(point.x - 2 * glm::normalize(wall.n).x * dot(s, glm::normalize(wall.n)), point.y - 2 * glm::normalize(wall.n).y * dot(s, glm::normalize(wall.n)));

    return result;
}

void generate_new_rays(vec2 tx, vec2 rx, vec2 hit_point, vec2 direction_before, vector<Wall> walls, Wall wall, vector<Ray>& all_rays, vector<Ray>& buffer, vector<int> &walls_to_reflect, float start_energy, float total_distance, int reflexion_left, int max_reflexion) {
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

    float transmission = compute_total_transmission(incident_cos, cs, wall);

    if (reflexion_left != 0 && walls_to_reflect[reflexion_left - 1] == wall.id) {
            compute_ray(tx, rx, reflection_vector, hit_point, all_rays, buffer, walls, walls_to_reflect, start_energy * reflection_coef, total_distance, reflexion_left - 1, max_reflexion);
            return;
    }
    compute_ray(tx, rx, direction_before, hit_point, all_rays, buffer, walls, walls_to_reflect, start_energy * transmission, total_distance, reflexion_left, max_reflexion);
}

void compute_ray(vec2 tx, vec2 rx, vec2 direction, vec2 ray_origin, vector<Ray>& all_rays, vector<Ray>& buffer, vector<Wall> walls, vector<int>& walls_to_reflect, float start_energy, float total_distance, int reflexion_left, int max_reflexion) {
    if (reflexion_left == -1) return;
    // is hitting RX
    vec2 direction_to_rx = normalize(rx - ray_origin);
    vec2 direction_normalized = normalize(direction);

    if (direction_normalized.x < direction_to_rx.x + 0.000001 && direction_normalized.x > direction_to_rx.x - 0.000001 &&
        direction_normalized.y < direction_to_rx.y + 0.000001 && direction_normalized.y > direction_to_rx.y - 0.000001 &&
        reflexion_left == 0) {
        float distance_to_rx = length(rx - ray_origin);
        // a wall could be between the ray reflex and RX
        bool found_wall_intersect_before_rx = true;
        Wall best_wall = walls[0];

        vec2 hit_point = getIntersection(direction, ray_origin, walls, best_wall, found_wall_intersect_before_rx, distance_to_rx);

        if (found_wall_intersect_before_rx) {
            vector<Ray> buffer2(buffer);
            buffer2.push_back(Ray(ray_origin, hit_point, start_energy, total_distance + length(ray_origin - hit_point)));
            return generate_new_rays(tx, rx, hit_point, direction, walls, best_wall, all_rays, buffer2, walls_to_reflect, start_energy, total_distance, reflexion_left, max_reflexion);
        }

        buffer.push_back(Ray(ray_origin, rx, start_energy, total_distance + distance_to_rx));
        for (Ray r : buffer) { // adds the rays only if they actually hit RX
            all_rays.push_back(r);
        }

        return;
    }

    bool found = true;
    Wall best_wall = walls[0];
    vec2 hit_point = getIntersection(direction, ray_origin, walls, best_wall, found, 100000);

    if (found) {
        // the ray hit a wall
        vector<Ray> buffer2(buffer);
        buffer2.push_back(Ray(ray_origin, hit_point, start_energy, total_distance + length(hit_point - ray_origin)));
        // copies the vector
        return generate_new_rays(tx, rx, hit_point, direction, walls, best_wall, all_rays, buffer2, walls_to_reflect, start_energy, total_distance, reflexion_left, max_reflexion);
    }
}

void generate_rays_direction(vec2 tx, vec2 rx, vec2 fake_rx, vector<Wall> walls, int previous, vector<Ray>& all_rays, vector<int> walls_to_reflect, vector<vec2>& fake_rx_done, int max_reflexion, int counter) {
    counter++;

    for (int i = 0; i < walls.size(); i++)
    {
        /*
        if (walls_to_reflect.find(i) != walls_to_reflect.end())
            continue; */
        if (i == previous)
            continue;

        vector<int> wall_buffer(walls_to_reflect);
        wall_buffer.push_back(i);

        vec2 mirror = getMirrorWithWall(fake_rx, walls[i].fancy_vector);

        if (mirror != rx) {
            vector<Ray> buffer;
            compute_ray(tx, rx, mirror - tx, tx, all_rays, buffer, walls, wall_buffer, 1.0f, 0.f, counter, max_reflexion);
        }

        /*
        if (find(fake_rx_done.begin(), fake_rx_done.end(), mirror) == fake_rx_done.end()) {// also contains rx, which happens with walls 7 and 8 for instance

            fake_rx_done.push_back(mirror);

            vector<Ray> buffer;
            compute_ray(tx, rx, mirror - tx, tx, all_rays, buffer, walls, wall_buffer, 1.0f, 0.f, counter, max_reflexion);
        } */

        if (counter == max_reflexion) continue;
        generate_rays_direction(tx, rx, mirror, walls, i, all_rays, wall_buffer, fake_rx_done, max_reflexion, counter);
    }
}

vec2 getIntersection(vec2 direction, vec2 origin, vector<Wall> walls, Wall& wall_, bool& found, float min_distance) {
    float t = 0;
    float s = 0;
    // calculating intersection
    vec2 best_intersection(-1000, -1000);
    Wall best_wall = walls[0];
    double distance = 0;

    for (int w = 0; w < walls.size(); w++) {
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

        if (t < 0.0001 || t > 1000000 || s < 0.0001 || s > 1) continue;

        vec2 intersection = vec2(origin.x + t * direction.x, origin.y + t * direction.y);
        distance = length(intersection - origin);

        if (distance < min_distance) {
            min_distance = distance;
            best_intersection = intersection;
            best_wall = wall_;
        }
    }

    if (best_intersection.x == -1000 && best_intersection.y == -1000) found = false;
    wall_ = best_wall;
    return best_intersection;
}
