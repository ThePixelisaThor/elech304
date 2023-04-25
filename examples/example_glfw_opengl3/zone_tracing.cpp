#include <stdio.h>
#include <math.h>

#include "zone_tracing.h"
#include "raytracing.h"
#include "render_utils.h"
#include "fancy_vector.h"
#include "color.h"
#include "ray.h"
#include <set>

using namespace std;

void generate_new_rays_zone(vec2 tx, vec2 rx, vec2 hit_point, vec2 direction_before, vector<Wall> walls, Wall wall, vector<Ray>& rays_hitting, set<int>& walls_to_reflect, float start_energy, float total_distance, int reflexion_left) {
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

    if (reflexion_left != 0 && walls_to_reflect.find(wall.id) != walls_to_reflect.end())
        compute_ray_zone(tx, rx, reflection_vector, hit_point, rays_hitting, walls, walls_to_reflect, start_energy * reflection_coef, total_distance, reflexion_left - 1);

    // transmitted ray
    float transmission = compute_total_transmission(incident_cos, cs, wall);
    compute_ray_zone(tx, rx, direction_before, hit_point, rays_hitting, walls, walls_to_reflect, start_energy * transmission, total_distance, reflexion_left);
}

void compute_ray_zone(vec2 tx, vec2 rx, vec2 direction, vec2 ray_origin, vector<Ray>& rays_hitting, vector<Wall> walls, set<int>& walls_to_reflect, float start_energy, float total_distance, int reflexion_left) {
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
            return generate_new_rays_zone(tx, rx, hit_point, direction, walls, best_wall, rays_hitting, walls_to_reflect, start_energy, total_distance, reflexion_left);
        }

        rays_hitting.push_back(Ray(ray_origin, rx, start_energy, total_distance + distance_to_rx));

        return;
    }

    bool found = true;
    Wall best_wall = walls[0];
    vec2 hit_point = getIntersection(direction, ray_origin, walls, best_wall, found, 100000);

    if (found) {
        // copies the vector
        return generate_new_rays_zone(tx, rx, hit_point, direction, walls, best_wall, rays_hitting, walls_to_reflect, start_energy, total_distance, reflexion_left);
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
        complex<float> phaseur(sqrt(60.f * directionality * power) * exp(-j * beta * r.total_distance_travelled) / (r.total_distance_travelled));

        average_power += squared_module(equivalent_height * phaseur);
    }

    return average_power / (8.f * equivalent_resistance);
}

void generate_rays_direction_(vec2 tx, vec2 rx, vec2 fake_rx, vector<Wall> walls, int previous, vector<Ray>& rays_hitting, set<int> walls_to_reflect, int max_reflexion, int counter) {
    counter++;

    for (int i = previous + 1; i < walls.size(); i++)
    {
        set<int> wall_buffer(walls_to_reflect);
        wall_buffer.insert(i);

        vec2 mirror = getMirrorWithWall(fake_rx, walls[i].fancy_vector);

        compute_ray_zone(tx, rx, mirror - tx, tx, rays_hitting,  walls, wall_buffer, 1.0f, 0.f, max_reflexion - counter + 2);

        if (counter == max_reflexion) continue;
        generate_rays_direction_(tx, rx, mirror, walls, i, rays_hitting, wall_buffer, max_reflexion, counter);
    }
}

void generate_ray_hitting_rx(vec2 tx, vec2 rx, std::vector<Wall>& walls, std::vector<Ray>& ray_hitting, int max_reflexion)
{
    set<int> empty;

    compute_ray_zone(tx, rx, rx - tx, tx, ray_hitting, walls, empty, 1.0f, 0, 2); // direct

    generate_rays_direction_(tx, rx, rx, walls, -1, ray_hitting, empty, max_reflexion, 0);
}
