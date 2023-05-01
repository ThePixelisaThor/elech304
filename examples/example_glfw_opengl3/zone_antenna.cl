// some function for complex numbers, because they are not natively supported by opencl :'(
// we can choose between floats and doubles, we'll go for floats are a huge precision isn't required to compute coefficients
// the idea is to use float2 as a complex

typedef float2 cl_float_complex;
typedef cl_float_complex cl_complex;

inline float cl_complex_real_part(cl_complex n){
	return n.x;
}

inline float cl_complex_imaginary_part(cl_complex n){
	return n.y;
}

inline float cl_complex_modulus(cl_complex n){
	return (sqrt( (n.x*n.x)+(n.y*n.y) ));
}

inline float cl_complex_modulus_squared(cl_complex n){
	return ((n.x*n.x)+(n.y*n.y));
}

inline cl_complex cl_complex_add(cl_complex a, cl_complex b){
	return (cl_complex)( a.x + b.x, a.y + b.y);
}

inline cl_complex cl_complex_multiply(cl_complex a, cl_complex b){
	return (cl_complex)(a.x*b.x - a.y*b.y,  a.x*b.y + a.y*b.x);
}

inline cl_complex cl_complex_divide(cl_complex a, cl_complex b){
	const float dividend = (b.x*b.x  + b.y*b.y);
	return (cl_complex)((a.x*b.x + a.y*b.y)/dividend , (a.y*b.x - a.x*b.y)/dividend);
}

inline cl_complex cl_complex_exp(cl_complex n){
	const float e = exp(n.x);
	return (cl_complex)(e*cos(n.y) , e*sin(n.y));
}

inline float get_antenna_power(int antenna_id, float incident_angle_cos, float antenna_angle) {
    if (antenna_id == 1) return 0.1; // 0.1 W for TX1
    if (antenna_id == 2) return 3.162; // TX2
    if (antenna_id == 3) {
        return 3.162 * pow(10, (21.5836 - 12 * pown((acos(incident_angle_cos) - antenna_angle) / 0.5236, 2)) / 10.0); // power of TX3 varies depending on the angle
    }
    return 1.;
}

__kernel void compute_zone(__global float* x_coordinates, __global float* y_coordinates,
                __global float* tx_mirror_coordinates_x, __global float* tx_mirror_coordinates_y, int mirror_count, int wall_count,
                __global float2* walls_coordinates_a,
                __global float2* normalized_walls_u, __global float2* normalized_walls_n, __global float2* walls_u,
                __global float* walls_relative_perm, __global float* walls_depth,
                __global float2* walls_impedance, float2 air_impedance, __global float2* walls_gamma,
                __global int* each_reflection_count, __global int* walls_to_reflect,
                float2 tx,
                int zone_count_x, __global int* ray_count, int reflection_count, __global float* energy, float beta,
                float pulsation, int antenna_id, float antenna_angle){
    // the goal here is to compute the number of ray hitting rx AND the power received, no antennas for now
    // energy is already squared

    // compute_id goes from 0 to zone_count_x * zone_count_y - 1
    // x_coordinate is modified first
    int compute_id = get_global_id(0);

    float total_distance = 0;
    float rx_x = x_coordinates[compute_id % zone_count_x];
    float rx_y = y_coordinates[compute_id / zone_count_x];

    float2 rx = (float2) (rx_x, rx_y);

    ray_count[compute_id] = 0; // starts at 0
    energy[compute_id] = 0.f;

    float ray_energy;

    float2 current_direction;
    float2 current_origin;
    int reflexion_left;
    float2 current_direction_to_tx_normalized;
    float2 current_direction_normalized;

    float distance_to_tx;
    int best_wall_id;
    float2 hit_point;
    float2 reflection_vector;

    // intersection (everything should reset for each intersection)
    int last_wall_id;
    float t = 0; 
    float s = 0;
    float2 best_intersection;
    float2 intersection;
    double distance;
    int wall_id = -1;
    float it_wall_x = 0;
    float it_wall_y = 0;
    float min_distance;

    // coefficients
    float incident_angle_cos;
    float incident_angle_sin;
    float transmitted_angle_sin;
    float transmitted_angle_cos;
    float transmitted_depth;
    cl_complex one = (cl_complex) (1, 0); // 1 + 0j
    cl_complex two = (cl_complex) (2, 0); // 2 + 0j
    cl_complex j = (cl_complex) (0, 1); // 0 + 1j
    cl_complex reflection_coef;
    cl_complex squared_reflection_coef;
    cl_complex total_transmission_coef;
    cl_complex squared_total_transmission_coef;

    // energy, thoses constants will be eventually moved
    float equivalent_resistance = 50.f; // du récepteur !!
    float power = 1.f;
    float lambda = 3.f * pown(10.f, 8) * 2.f * 3.141592f / pulsation; // 2 pi v / omega
    float equivalent_height = lambda / 3.141592f;

    for (int m = 0; m < mirror_count; m++) {
        last_wall_id = -1;
        ray_energy = 1.f;
        current_direction = (float2) (tx_mirror_coordinates_x[m] - rx.x, tx_mirror_coordinates_y[m] - rx.y); // initial direction of the ray, going from rx to tx
        current_origin = rx; // ray starts at rx
        current_direction_normalized = normalize(current_direction);
        current_direction_to_tx_normalized = normalize(tx - current_origin); // to the real tx !
        reflexion_left = each_reflection_count[m]; // not constant, varies between 0 (direct ray) to max_reflexion
        total_distance = 0;

        if (length(current_origin - tx) <= 5) continue; // zone d'exclusion

        while(true) { // no recursion allowed
            // is it hitting TX, accounting for rounding errors

            if (current_direction_normalized.x < current_direction_to_tx_normalized.x + 0.000001 && current_direction_normalized.x > current_direction_to_tx_normalized.x - 0.000001 &&
            current_direction_normalized.y < current_direction_to_tx_normalized.y + 0.000001 && current_direction_normalized.y > current_direction_to_tx_normalized.y - 0.000001 &&
            reflexion_left == 0) {
                distance_to_tx = length(tx - current_origin);
                // a wall could be between the ray reflex and TX, let's find an intersection :
                t = 0;
                s = 0;
                best_intersection = (float2) (-1000, -1000);
                distance = 0;
                min_distance = distance_to_tx;
                best_wall_id = -1;

                for (int w = 0; w < wall_count; w++) { // w is the wall id
                    if (w == last_wall_id) continue;
                    if (walls_u[w].y == 0) {
                        t = (walls_coordinates_a[w].y - current_origin.y) / current_direction.y;
                        s = (current_origin.x + current_direction.x * t - walls_coordinates_a[w].x) / walls_u[w].x;
                    }
                    else {
                        t = walls_u[w].y / (current_direction.x * walls_u[w].y - walls_u[w].x * current_direction.y) * (walls_coordinates_a[w].x - current_origin.x + walls_u[w].x * (current_origin.y - walls_coordinates_a[w].y) / walls_u[w].y);
                        s = (current_origin.y + current_direction.y * t - walls_coordinates_a[w].y) / walls_u[w].y;
                    }
                    if (t < 0.001 || t > 1000000 || s < 0.0001 || s > 1) continue;

                    // updates intersection without creating a new vector
                    intersection.x = current_origin.x + t * current_direction.x;
                    intersection.y = current_origin.y + t * current_direction.y;

                    distance = length(intersection - current_origin);
                    if (distance < min_distance) {
                        min_distance = distance;
                        best_intersection = intersection;
                        best_wall_id = w;
                    }
                }

                hit_point = best_intersection;

                // end of the intersection
                last_wall_id = best_wall_id;

                if (best_wall_id > -1) {
                    incident_angle_cos = fabs(dot(normalized_walls_n[best_wall_id], current_direction_normalized)); // angle should be between -90 and 90

                    // reflection coefficient
                    incident_angle_sin = sqrt(1 - incident_angle_cos * incident_angle_cos);
                    transmitted_angle_sin = sqrt(1.0 / walls_relative_perm[best_wall_id]) * incident_angle_sin; // snell descartes
                    transmitted_angle_cos = sqrt(1.0 - transmitted_angle_sin * transmitted_angle_sin);
                    transmitted_depth = walls_depth[best_wall_id] / transmitted_angle_cos;

                    reflection_coef = cl_complex_divide(
                                        cl_complex_add((cl_complex)
                                                walls_impedance[best_wall_id] * incident_angle_cos,
                                                - transmitted_angle_cos * air_impedance),
                                        cl_complex_add((cl_complex)
                                                incident_angle_cos * walls_impedance[best_wall_id],
                                                (cl_complex) transmitted_angle_cos * air_impedance));
                    squared_reflection_coef = cl_complex_multiply(reflection_coef, reflection_coef);
                    // total transmission

                    total_transmission_coef = cl_complex_multiply(
                                                cl_complex_add(
                                                        one, (cl_complex) - (float2) squared_reflection_coef),
                                                cl_complex_exp(
                                                        - transmitted_depth * walls_gamma[best_wall_id]));

                    total_transmission_coef = cl_complex_divide(total_transmission_coef,
                                                    cl_complex_add(one,
                                                    - cl_complex_multiply(
                                                        squared_reflection_coef,
                                                        cl_complex_multiply(
                                                            cl_complex_exp(
                                                            -2 * walls_gamma[best_wall_id] * transmitted_depth),
                                                            cl_complex_exp(
                                                                (cl_complex) (0, 2 * beta * transmitted_depth * transmitted_angle_sin * incident_angle_sin)
                                                        ))
                                                    )));

                    ray_energy *= cl_complex_modulus_squared(total_transmission_coef);

                    current_origin = hit_point;
                    total_distance += min_distance / 10.f;
                    continue;

                } else {
                    // the ray hits tx before anything else
                    total_distance += min_distance / 10.f;
                    ray_count[compute_id]++;

                    // update energy
                    if (antenna_id == 3)
                        incident_angle_cos = fabs(dot((float2) (1, 0), current_direction_normalized));

                    if (ray_energy > 0.f && ray_energy <= 1.f) // parfois ray_energy vaut NaN jsp pourquoi
                        energy[compute_id] += get_antenna_power(antenna_id, incident_angle_cos, antenna_angle) * ray_energy / pown(total_distance, 2);
                    break;
                }
            }

            // if the direction doesn't correspond to hitting tx, let's find the intersection :
            t = 0;
            s = 0;
            best_intersection = (float2) (-1000, -1000);
            distance = 0;
            min_distance = 100000;
            best_wall_id = -1;

            for (int w = 0; w < wall_count; w++) { // w is the wall id
                if (w == last_wall_id) continue;
                if (walls_u[w].y == 0) {
                    t = (walls_coordinates_a[w].y - current_origin.y) / current_direction.y;
                    s = (current_origin.x + current_direction.x * t - walls_coordinates_a[w].x) / walls_u[w].x;
                }
                else {
                    t = walls_u[w].y / (current_direction.x * walls_u[w].y - walls_u[w].x * current_direction.y) * (walls_coordinates_a[w].x - current_origin.x + walls_u[w].x * (current_origin.y - walls_coordinates_a[w].y) / walls_u[w].y);
                    s = (current_origin.y + current_direction.y * t - walls_coordinates_a[w].y) / walls_u[w].y;
                }

                if (t < 0.001 || t > 1000000 || s < 0.0001 || s > 1) continue; // 0.001 est le seuil de sensibilité, on peut probablement le ramener à 0.1 qui correspond à 1 cm je crois

                // updates intersection without creating a new vector
                intersection.x = current_origin.x + t * current_direction.x;
                intersection.y = current_origin.y + t * current_direction.y;

                distance = length(intersection - current_origin);

                if (distance < min_distance) {
                    min_distance = distance;
                    best_intersection = intersection;
                    best_wall_id = w;
                }
            }

            hit_point = best_intersection;

            // end of the intersection

            last_wall_id = best_wall_id;

            if (best_wall_id > -1) {
                
                reflection_vector = current_direction - 2.f * dot(current_direction, normalized_walls_n[best_wall_id]) * normalized_walls_n[best_wall_id];

                incident_angle_cos = fabs(dot(normalized_walls_n[best_wall_id], current_direction_normalized)); // angle should be between -90 and 90

                // reflection coefficient (has to be calculated for both reflection and transmission)
                incident_angle_sin = sqrt(1 - incident_angle_cos * incident_angle_cos);
                transmitted_angle_sin = sqrt(1.0 / walls_relative_perm[best_wall_id]) * incident_angle_sin; // snell descartes
                transmitted_angle_cos = sqrt(1.0 - transmitted_angle_sin * transmitted_angle_sin);
                transmitted_depth = walls_depth[best_wall_id] / transmitted_angle_cos;

                reflection_coef = cl_complex_divide(
                                    cl_complex_add(
                                            walls_impedance[best_wall_id] * incident_angle_cos,
                                            - transmitted_angle_cos * air_impedance),
                                    cl_complex_add(
                                            incident_angle_cos * walls_impedance[best_wall_id],
                                            transmitted_angle_cos * air_impedance));
                squared_reflection_coef = cl_complex_multiply(reflection_coef, reflection_coef);
                    

                // ça peut être une réflexion ou une transmission
                current_origin = hit_point;
                total_distance += min_distance / 10.f;

                if (reflexion_left != 0 && walls_to_reflect[m * reflection_count + reflexion_left - 1] == best_wall_id) {
                    // reflection on the right wall
                    current_direction = reflection_vector;
                    reflexion_left--;
                    current_direction_to_tx_normalized = normalize(tx - current_origin);
                    current_direction_normalized = normalize(current_direction);
                    ray_energy *= cl_complex_modulus_squared(reflection_coef);

                } else {
                    // transmission on a random wall

                    total_transmission_coef = cl_complex_multiply(
                                                cl_complex_add(
                                                        one, - squared_reflection_coef),
                                                cl_complex_exp(
                                                        - transmitted_depth * walls_gamma[best_wall_id]));

                    total_transmission_coef = cl_complex_divide(total_transmission_coef,
                                                    cl_complex_add(one,
                                                    - cl_complex_multiply(
                                                        squared_reflection_coef,
                                                        cl_complex_multiply(
                                                            cl_complex_exp(
                                                            -2 * walls_gamma[best_wall_id] * transmitted_depth),
                                                            cl_complex_exp(
                                                                (cl_complex) (0, 2 * beta * transmitted_depth * transmitted_angle_sin * incident_angle_sin)
                                                        ))
                                                    )));

                    ray_energy *= cl_complex_modulus_squared(total_transmission_coef);

                }

                continue;
            }

        break;
        }
    }

    // we want the energy in log
    if (energy[compute_id] != 0.f) {
        energy[compute_id] = 10.0 * log10(1000.f * 60.f * pow(equivalent_height, 2) / (8.f * equivalent_resistance) * energy[compute_id]); // factor 1000 to get it in dBm
        } else {
        energy[compute_id] = -100.f;
    }
}
