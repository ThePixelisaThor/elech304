#include "rx_field.h"
#include "zone_tracing_cpu.h"
#include "ray.h"

using namespace glm;

void compute_ray_fields(vec2& tx, vec2& rx, vec2 direction, Ray& ray, bool& hit, Wall* walls, int wall_count,
    int* walls_to_reflect, std::complex<float> energy, float total_distance, int max_reflexion) {
    /* tx : vec2 representing the position of tx
     * rx : vec2 representing the position of rx
     * direction : the initial direction
     * walls : array containing all walls
     * wall_count : number of walls
     * walls_to_reflect : contains the sequence of wall to hit
     * max_reflexion : max reflexion allowed (0 for tx, 1 for tx', etc)
     *
     * ray : modified if the ray actually hit rx
     * hit : true if the ray hit rx
     * energy : final energy
     * total_distance : total distance travelled in meters
     */

    vec2 current_origin(rx); // starts from rx !
    vec2 current_direction(direction); // current direction changes for each reflexion
    int reflexion_left = max_reflexion; // changes after each reflexion
    hit = false; // by default

    vec2 current_direction_to_tx = normalize(tx - current_origin); // changes after each reflexion
    vec2 current_direction_normalized = normalize(current_direction); // changes after each reflexion

    // variables in the loop, so that we don't initialize and delete them from the stack each iteration
    float distance_to_tx;
    bool found;
    Wall best_wall;
    vec2 hit_point;
    float incident_cos;
    coefficients cs;
    vec2 reflection_vector;
    std::complex<float> reflection_coef, transmission_coef;

    std::complex<float> air_impedance_cst = compute_impedance(1.f, 0, 1);

    while (true) { // no recursion !!!
        // is it hitting TX, accounting for rounding errors
        if (current_direction_normalized.x < current_direction_to_tx.x + 0.000001 && current_direction_normalized.x > current_direction_to_tx.x - 0.000001 &&
            current_direction_normalized.y < current_direction_to_tx.y + 0.000001 && current_direction_normalized.y > current_direction_to_tx.y - 0.000001 &&
            reflexion_left == 0) {
            distance_to_tx = length(tx - current_origin);

            // a wall could be between the ray reflex and TX
            found = true;
            hit_point = getIntersection_cpu(current_direction, current_origin, walls, wall_count, best_wall, found, distance_to_tx);

            if (found) {
                // faut update energy avec les coefficients
                incident_cos = abs(glm::dot(best_wall.fancy_vector.normalized_n, current_direction_normalized)); // pour que l'angle soit entre -90 et 90
                cs = compute_reflection_coefficients(incident_cos, air_impedance_cst, best_wall);
                transmission_coef = compute_total_transmission_coefficient(incident_cos, cs, best_wall);

                // on sait que c'est d'office une transmission car il ne reste plus de réflexions
                energy *= transmission_coef;
                total_distance += length(hit_point - current_origin) / 10.f;
                current_origin = hit_point; // pas inverser les 2 lignes

                continue;

            }
            else {
                // the ray hits tx before anything else
                ray = Ray(current_origin, tx, energy, total_distance + (distance_to_tx / 10.f));
                hit = true;
                break;
            }
        }

        // if the direction doesn't correspond to hitting tx
        found = true;
        hit_point = getIntersection_cpu(current_direction, current_origin, walls, wall_count, best_wall, found, 100000);

        if (found) {
            // faut update energy avec les coefficients
            incident_cos = abs(glm::dot(best_wall.fancy_vector.normalized_n, current_direction_normalized)); // pour que l'angle soit entre -90 et 90
            cs = compute_reflection_coefficients(incident_cos, air_impedance_cst, best_wall);
            reflection_vector = current_direction - 2.f * glm::dot(current_direction,
                glm::normalize(vec2(best_wall.fancy_vector.u.y, -best_wall.fancy_vector.u.x))) * glm::normalize(vec2(best_wall.fancy_vector.u.y, -best_wall.fancy_vector.u.x));
            reflection_coef = compute_total_reflection_coefficient(incident_cos, cs, best_wall); // pas juste
            transmission_coef = compute_total_transmission_coefficient(incident_cos, cs, best_wall);

            if (incident_cos > 0.820 && incident_cos < 0.830) {

                std::cout << "";
            }

            // ça peut être une réflexion ou une transmission

            total_distance += length(hit_point - current_origin) / 10.f;

            current_origin = hit_point;

            if (reflexion_left != 0 && walls_to_reflect[reflexion_left - 1] == best_wall.id) {
                energy *= reflection_coef;
                current_direction = reflection_vector;
                reflexion_left--;
                current_direction_to_tx = normalize(tx - current_origin);
                current_direction_normalized = normalize(current_direction);
            }
            else {
                energy *= transmission_coef;
            }

            continue;
        }

        hit = false;
        break;
    }
}

void compute_all_rays_hitting_tx_fields(int reflection_count, Wall* walls, int wall_count, vec2 tx, vec2 rx, Ray* rays_hitting_tx, int& ray_count) {
    /* reflection_count : max reflection allowed
     * walls : array of all the walls
     * wall_count : number of walls
     * rays_hitting_tx : la direction est déjà bonne pour l'antenne du coup
     *
     * Cette fonction considère un seul rx, pour plusieurs rx il ne faut pas recalculer tous les mirrors !
     * Les rayons vont de rx à tx !
     */
    int max_count = 1 + (int)pow(wall_count, reflection_count); // should equal mirror_count
    vec2* all_tx_mirrors = new vec2[max_count]; // il y a moyen de calculer tous les tx mirrors, faire le chemin à l'envers mais c'est chiant je pense
    int* each_reflection_count = new int[max_count];
    int* walls_to_reflect = new int[max_count * reflection_count]; // represents a 2d array [[r1, r2, ...], [r1, r2, ...], ...]

    int mirror_count = 0;

    generate_all_mirrors(reflection_count, all_tx_mirrors, each_reflection_count, walls_to_reflect,
        walls, wall_count, mirror_count, tx, max_count);

    bool hit;
    ray_count = 0;

    for (int m = 0; m < mirror_count; m++) {
        compute_ray_fields(tx, rx, all_tx_mirrors[m] - rx, rays_hitting_tx[ray_count], hit, walls, wall_count, &walls_to_reflect[m * reflection_count],
            1.0f, 0.f, each_reflection_count[m]);

        if (hit)
            ray_count++;
    }

    delete[] all_tx_mirrors;
    delete[] each_reflection_count;
    delete[] walls_to_reflect;

    return;
}

float compute_energy_fields(std::vector<Ray>& rays_hitting, float pulsation, float equivalent_resistance)
{
    float directionality = 1.5f; // lambda demi verticale
    float power = 1.0933f * pow(10, -3);

    float lambda = 299792458.f * 2.f * 3.141592f / pulsation;
    float beta = pulsation / (299792458.f); // beta in the air

    float equivalent_height = lambda / 3.141592f; // antenne dipôle avec theta = 90°

    std::complex<float> power_buffer = 0.f;
    std::complex<float> j{ 0, 1 }; // 0 + 1j

    for (Ray r : rays_hitting) {
        power_buffer += r.complex_energy * equivalent_height * exp(-j * beta * r.total_distance_travelled) / (r.total_distance_travelled);
        std::complex<float> eze = r.complex_energy * equivalent_height * exp(-j * beta * r.total_distance_travelled) / (r.total_distance_travelled);
        std::complex<float> buffer = -j * beta * r.total_distance_travelled;
        std::complex<float> test = exp(-j * beta * r.total_distance_travelled);
        std::cout << "";
    }

    return 60.f * directionality * power * squared_module_cpu(power_buffer) / (8.f * equivalent_resistance);
}
