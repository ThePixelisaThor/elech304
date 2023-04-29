#include "zone_tracing_cpu.h"
#include "ray.h"

using namespace glm;

std::complex<float> air_impedance_cst = compute_impedance(1.f, 0, 1);

vec2 getMirrorWithWall_cpu(vec2 point, FancyVector wall) {
    // returns the point position, as reflected by the wall

    vec2 s(point.x - wall.a.x, point.y - wall.a.y);
    return vec2(point.x - 2 * glm::normalize(wall.n).x * dot(s, glm::normalize(wall.n)), point.y - 2 * glm::normalize(wall.n).y * dot(s, glm::normalize(wall.n)));
}

void generate_mirrors(vec2 fake_tx, int& current_index, Wall* walls, int& wall_count, int current_reflection_count,
    int& reflection_count, int previous_walls_index, vec2* all_mirrors, int* each_reflection_count,
    int* walls_to_reflect, int previous) {
    /* fake_tx : current mirror in the tree of tx
     * current_index : current index of the direction
     * walls : array containing all walls
     * wall_count : number of walls
     * current_reflection_count : recursion depth
     * reflection_count : max reflection allowed
     * previous_walls_index : index of the previous wall
     * all_mirrors : array containing all the positions of fake_tx (+ tx) to aim for
     * each_reflection_count : array containing for each direction the number of reflections it should do (0 for tx, 1 for tx', 2 for tx'', etc)
     * walls_to_reflect : array of int arrays of max size reflection_count, containing for each direction the walls it should reflect
     * previous : id of the wall it just mirrored from
     * 
     * PS : j'ai cherché des moyens pour que cette fonction ne soit pas récursive et j'ai pas trouvé
     */
    current_reflection_count++;
    vec2 new_fake_tx;

    // c'est un peu long mais c'est pour éviter de check la condition pour chaque mur
    if (current_reflection_count == reflection_count) {
        for (int w = 0; w < wall_count; w++) {
            if (w == previous)
                continue;
            // creates new reflection
            new_fake_tx = getMirrorWithWall_cpu(fake_tx, walls[w].fancy_vector);
            all_mirrors[current_index] = new_fake_tx;
            each_reflection_count[current_index] = current_reflection_count;

            for (int i = 0; i < current_reflection_count - 1; i++)
                walls_to_reflect[current_index*reflection_count + i] = walls_to_reflect[previous_walls_index * reflection_count + i];
            walls_to_reflect[current_index * reflection_count + current_reflection_count - 1] = w;

            vec2 bu = all_mirrors[current_index];

            current_index++;
        }
    }
    else {
        for (int w = 0; w < wall_count; w++) {
            if (w == previous) continue;

            // creates new reflection
            new_fake_tx = getMirrorWithWall_cpu(fake_tx, walls[w].fancy_vector);
            all_mirrors[current_index] = new_fake_tx;
            each_reflection_count[current_index] = current_reflection_count;

            for (int i = 0; i < current_reflection_count - 1; i++)
                walls_to_reflect[current_index * reflection_count + i] = walls_to_reflect[previous_walls_index * reflection_count + i];
            walls_to_reflect[current_index * reflection_count + current_reflection_count - 1] = w;

            vec2 bu = all_mirrors[current_index];

            current_index++;

            // goes one recursion level further
            generate_mirrors(new_fake_tx, current_index, walls, wall_count, current_reflection_count,
                reflection_count, current_index - 1, all_mirrors, each_reflection_count,
                walls_to_reflect, w);
        }
    }
}

void generate_all_mirrors(int& reflection_count, vec2* all_mirrors, int* each_reflection_count, int* walls_to_reflect,
    Wall* walls, int& wall_count, int& mirror_count, vec2& tx, int& total_count) {
    /* reflection_count : max reflection allowed
     * all_mirrors : array containing all the positions of fake_rx (+ rx) to aim for
     * each_reflection_count : array containing for each direction the number of reflections it should do (0 for rx, 1 for rx', 2 for rx'', etc)
     * walls_to_reflect : array of int arrays of max size reflection_count, containing for each direction the walls it should reflect
     * walls : array of all the walls
     * wall_count : number of walls
     * tx : vec2 representing the position of rx
     * total_count : max size of all_directions/each_reflection_count/walls_to_reflect
     *
     * mirror_count : returns the number of mirror found (should be < 1 + wall_count^reflection_count)
     */

    mirror_count = 0;

    // by default, all of the walls to reflect from are -1
    for (int i = 0;i < reflection_count * total_count; i++)
        walls_to_reflect[i] = -1;

    // adds rx
    all_mirrors[0] = tx;
    each_reflection_count[0] = 0;
    mirror_count++;

    generate_mirrors(tx, mirror_count, walls, wall_count, 0, reflection_count, 0, all_mirrors, each_reflection_count,
        walls_to_reflect, -1);
}

vec2 getIntersection_cpu(vec2 direction, vec2 origin, Wall* walls, int &wall_count, Wall& best_wall, bool& found, float min_distance) {
    /* direction : direction of the ray
     * origin : origin of the ray
     * walls : array containing all walls
     * wall_count : number of walls
     * 
     * I have yet to optimize this function sorry
     */

    float t = 0;
    float s = 0;

    vec2 best_intersection(-1000, -1000);
    Wall it_wall;
    FancyVector wall;
    double distance = 0;
    vec2 intersection;

    for (int w = 0; w < wall_count; w++) {
        it_wall = walls[w];
        wall = it_wall.fancy_vector;
        if (wall.u.y == 0) {
            t = (wall.a.y - origin.y) / direction.y;
            s = (origin.x + direction.x * t - wall.a.x) / wall.u.x;
        }
        else {
            t = wall.u.y / (direction.x * wall.u.y - wall.u.x * direction.y) * (wall.a.x - origin.x + wall.u.x * (origin.y - wall.a.y) / wall.u.y);
            s = (origin.y + direction.y * t - wall.a.y) / wall.u.y;
        }

        if (t < 0.0001 || t > 1000000 || s < 0.0001 || s > 1) continue;

        // updates intersection without creating a new vector
        intersection.x = origin.x + t * direction.x;
        intersection.y = origin.y + t * direction.y;

        distance = length(intersection - origin);

        if (distance < min_distance) {
            min_distance = distance;
            best_intersection = intersection;
            best_wall = it_wall;
        }
    }

    if (best_intersection.x == -1000 && best_intersection.y == -1000) found = false;

    return best_intersection;
}

void compute_ray_cpu(vec2 &tx, vec2 &rx, vec2 direction, Ray &ray, bool &hit, Wall *walls, int wall_count,
                int *walls_to_reflect, float energy, float total_distance, int max_reflexion) {
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
    float reflection_coef, transmission_coef;

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
                incident_cos = abs(glm::dot(glm::normalize(best_wall.fancy_vector.n), current_direction_normalized)); // pour que l'angle soit entre -90 et 90
                cs = compute_reflection_coefficients(incident_cos, air_impedance_cst, best_wall);
                transmission_coef = compute_total_transmission(incident_cos, cs, best_wall);

                if (transmission_coef >= 1.f || transmission_coef < 0.f) {
                    //breakpoint
                    std::cout << "problem" << std::endl;
                }

                // on sait que c'est d'office une transmission car il ne reste plus de réflexions
                energy *= transmission_coef;
                total_distance += length(hit_point - current_origin) / 10.f;
                current_origin = hit_point; // pas inverser les 2 lignes

                continue;

            } else {
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
            incident_cos = abs(glm::dot(glm::normalize(best_wall.fancy_vector.n), current_direction_normalized)); // pour que l'angle soit entre -90 et 90
            cs = compute_reflection_coefficients(incident_cos, air_impedance_cst, best_wall);
            reflection_vector = current_direction - 2.f * glm::dot(current_direction,
                glm::normalize(vec2(best_wall.fancy_vector.u.y, -best_wall.fancy_vector.u.x))) * glm::normalize(vec2(best_wall.fancy_vector.u.y, -best_wall.fancy_vector.u.x));
            reflection_coef = sqrt(pow(cs.reflection.real(), 2) + pow(cs.reflection.imag(), 2)); // c'est un peu con psq après on mets quand même au carré
            transmission_coef = compute_total_transmission(incident_cos, cs, best_wall);

            if (reflection_coef >= 1.f || reflection_coef < 0.f || transmission_coef >= 1.f || transmission_coef < 0.f) {
                //breakpoint
                std::cout << "problem" << std::endl;
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
            } else {
                energy *= transmission_coef;
            }

            continue;
        }

        hit = false;
        break;
    }
}

void compute_all_rays_hitting_tx_cpu(int reflection_count, Wall *walls, int wall_count, vec2 tx, vec2 rx, Ray *rays_hitting_tx, int &ray_count) {
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
        compute_ray_cpu(tx, rx, all_tx_mirrors[m] - rx, rays_hitting_tx[ray_count], hit, walls, wall_count, &walls_to_reflect[m * reflection_count],
            1.0f, 0.f, each_reflection_count[m]);

        if (hit)
            ray_count++;
    }

    delete[] all_tx_mirrors;
    delete[] each_reflection_count;
    delete[] walls_to_reflect;
    
    return;
}

float squared_module_cpu(std::complex<float> vector) {
    return pow(vector.imag(), 2) + pow(vector.real(), 2);
}

float compute_energy_cpu(Ray *rays_hitting, int &ray_count)
{
    // not optimized yet
    float average_power = 0.f;
    float equivalent_resistance = 50.f;
    float directionality = 1.f;
    float power = 1.f;
    // float pulsation = 2.f * 3.141592f * 868.3f * pow(10, 6); // 868.3 MHz
    float pulsation = 2.f * 3.141592f * 26.f * pow(10, 9); // 26 GHz
    float lambda = 3.f * pow(10, 6) * 2.f * 3.141592f / pulsation;
    float beta = compute_beta(1.f, 0.f, pulsation); // alpha in the air

    float equivalent_height = lambda / 3.141592f; // antenne dipôle avec theta = 90°

    std::complex<float> j{ 0, 1 }; // 0 + 1j
    // en soi l'exponentielle complexe est inutile vu qu'on calcule la puissance moyenne
    for (int i = 0; i < ray_count; i++) {

    std::complex<float> phaseur(sqrt(60.f * directionality * power) / (rays_hitting[i].total_distance_travelled));

        average_power += squared_module_cpu(equivalent_height * phaseur * rays_hitting[i].energy);
    }

    return average_power / (8.f * equivalent_resistance);
}
