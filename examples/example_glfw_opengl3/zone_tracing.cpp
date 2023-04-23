#include <stdio.h>
#include <math.h>

#include "zone_tracing.h"
#include "raytracing.h"
#include "render_utils.h"
#include "fancy_vector.h"
#include "color.h"
#include "ray.h"

using namespace std;

void generate_new_rays_zone(vec2 tx, vec2 rx, vec2 hit_point, vec2 direction_before, vector<Wall> walls, Wall wall, vector<Ray>& rays_hitting, float start_energy, float total_distance, int reflexion_left) {
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
        compute_ray_zone(tx, rx, reflection_vector, hit_point, rays_hitting, walls, start_energy * reflection_coef, total_distance, reflexion_left - 1);

    // transmitted ray
    float transmission = compute_total_transmission(incident_cos, cs, wall);
    compute_ray_zone(tx, rx, direction_before, hit_point, rays_hitting, walls, start_energy * transmission, total_distance, reflexion_left);
}

void compute_ray_zone(vec2 tx, vec2 rx, vec2 direction, vec2 ray_origin, vector<Ray>& rays_hitting, vector<Wall> walls, float start_energy, float total_distance, int reflexion_left) {
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

        vec2 hit_point = getIntersection(direction, ray_origin, walls, best_wall, found_wall_intersect_before_rx, distance_to_rx);

        if (found_wall_intersect_before_rx) {
            return generate_new_rays_zone(tx, rx, hit_point, direction, walls, best_wall, rays_hitting, start_energy, total_distance, reflexion_left);
        }

        rays_hitting.push_back(Ray(ray_origin, rx, start_energy, total_distance + distance_to_rx));

        return;
    }

    bool found = true;
    Wall best_wall = walls[0];
    vec2 hit_point = getIntersection(direction, ray_origin, walls, best_wall, found, 100000);

    if (found) {
        // copies the vector
        return generate_new_rays_zone(tx, rx, hit_point, direction, walls, best_wall, rays_hitting, start_energy, total_distance, reflexion_left);
    }
}

float squared_module(complex<float> vector) {
    return pow(vector.imag(), 2) + pow(vector.real(), 2);
}

float module(complex<float> vector) {
    return (float)sqrt(pow(vector.imag(), 2) + pow(vector.real(), 2));
}

float compute_energy(std::vector<Ray>& rays_hitting)
{
    float average_power = 0.f;
    float equivalent_resistance = 50.f;
    float directionality = 1.f;
    float power = 1.f;
    float pulsation = 2.f * 3.141592f * 868.3f * pow(10, 6);
    float lambda = 3.f * pow(10, 6) * 2.f * 3.141592f / pulsation;
    float beta = compute_beta(1.f, 0.f, pulsation); // alpha in the air

    float equivalent_height = lambda / 3.141592f; // antenne dipôle avec theta = 90°

    complex<float> j{ 0, 1 }; // 0 + 1j
    // en soi l'exponentielle complexe est inutile vu qu'on calcule la puissance moyenne
    for (Ray r : rays_hitting) {
        complex<float> phaseur(sqrt(60.f * directionality * power) * exp(-j * beta * r.total_distance_travelled) /(r.total_distance_travelled));
        
        average_power += squared_module(equivalent_height * phaseur);
    }

    return average_power / (8.f * equivalent_resistance);
}

void generate_rays_direction_(vec2 tx, vec2 rx, vec2 fake_rx, vector<Wall> walls, int previous, vector<Ray>& rays_hitting, int max_reflexion, int counter) {
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
        compute_ray_zone(tx, rx, direction, tx, rays_hitting, walls, 1.0f, direction.length(), max_reflexion - counter + 2);
    }

    if (counter == max_reflexion) {
        return;
    }

    for (int i = previous + 1; i < walls.size(); i++) // recursion
    {
        generate_rays_direction_(tx, rx, getMirrorWithWall(fake_rx, walls[i].fancy_vector), walls, i, rays_hitting, max_reflexion, counter);
    }

}

void generate_ray_hitting_rx(vec2 tx, vec2 rx, std::vector<Wall> &walls, std::vector<Ray>& ray_hitting, int max_reflexion)
{
    vector<Ray> _buffer;

    compute_ray_zone(tx, rx, rx - tx, tx, ray_hitting, walls, 1.0f, 0, 2); // direct

    generate_rays_direction_(tx, rx, rx, walls, -1, ray_hitting, max_reflexion, 0);
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

        if (t < 0.01 || t > 100000 || s < 0.01 || s > 1) continue;

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
