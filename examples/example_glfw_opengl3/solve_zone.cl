__kernel void compute_zone_number(__global float* x_coordinates, __global float* y_coordinates,
                __global float* tx_mirror_coordinates_x, __global float* tx_mirror_coordinates_y, int mirror_count, int wall_count,
                __global float2* walls_coordinates_a,
                __global float2* normalized_walls_u, __global float2* normalized_walls_u_opposite, __global float2* walls_u,
                __global int* each_reflection_count, __global int* walls_to_reflect,
                float2 tx,
                int zone_count_x, __global int* ray_count, int reflection_count){
    // the goal here is to compute the number of ray hitting rx (no coefficients for now)
    // normalized_walls_u : glm::normalize(vec2(best_wall.fancy_vector.u.y, -best_wall.fancy_vector.u.x))
    // normalized_walls_u_opposite : glm::normalize(vec2(best_wall.fancy_vector.u.y, -best_wall.fancy_vector.u.x))

    // compute_id goes from 0 to zone_count_x * zone_count_y - 1
    // x_coordinate is modified first
    int compute_id = get_global_id(0);

    float rx_x = x_coordinates[compute_id % zone_count_x];
    float rx_y = y_coordinates[compute_id / zone_count_x];
    float2 rx = (float2) (rx_x, rx_y);

    ray_count[compute_id] = 0; // starts at 0
    
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
    float t = 0; 
    float s = 0;
    float2 best_intersection;
    float2 intersection;
    double distance;
    int wall_id = -1;
    float it_wall_x = 0;
    float it_wall_y = 0;
    float min_distance;

    for (int m = 0; m < mirror_count; m++) {

        current_direction = (float2) (tx_mirror_coordinates_x[m] - rx.x, tx_mirror_coordinates_y[m] - rx.y); // initial direction of the ray, going from rx to tx
        current_origin = rx; // ray starts at rx
        current_direction_normalized = normalize(current_direction);
        current_direction_to_tx_normalized = normalize(tx - current_origin); // to the real tx !
        reflexion_left = each_reflection_count[m]; // not constant, varies between 0 (direct ray) to max_reflexion

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
                    if (walls_u[w].y == 0) {
                        t = (walls_coordinates_a[w].y - current_origin.y) / current_direction.y;
                        s = (current_origin.x + current_direction.x * t - walls_coordinates_a[w].x) / walls_u[w].x;
                    }
                    else {
                        t = walls_u[w].y / (current_direction.x * walls_u[w].y - walls_u[w].x * current_direction.y) * (walls_coordinates_a[w].x - current_origin.x + walls_u[w].x * (current_origin.y - walls_coordinates_a[w].y) / walls_u[w].y);
                        s = (current_origin.y + current_direction.y * t - walls_coordinates_a[w].y) / walls_u[w].y;
                    }
                    if (t < 0.0001 || t > 1000000 || s < 0.0001 || s > 1) continue;

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

                if (best_wall_id > -1) {
                    // one oublie l'énergie et la distance pour l'instant
                    current_origin = hit_point; // pas inverser les 2 lignes

                    continue;

                } else {
                    // the ray hits tx before anything else
                    ray_count[compute_id]++;
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
                if (walls_u[w].y == 0) {
                    t = (walls_coordinates_a[w].y - current_origin.y) / current_direction.y;
                    s = (current_origin.x + current_direction.x * t - walls_coordinates_a[w].x) / walls_u[w].x;
                }
                else {
                    t = walls_u[w].y / (current_direction.x * walls_u[w].y - walls_u[w].x * current_direction.y) * (walls_coordinates_a[w].x - current_origin.x + walls_u[w].x * (current_origin.y - walls_coordinates_a[w].y) / walls_u[w].y);
                    s = (current_origin.y + current_direction.y * t - walls_coordinates_a[w].y) / walls_u[w].y;
                }

                if (t < 0.0001 || t > 1000000 || s < 0.0001 || s > 1) continue;

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

            if (best_wall_id > -1) {

                reflection_vector = current_direction - 2.f * dot(current_direction, normalized_walls_u_opposite[best_wall_id]) * normalized_walls_u_opposite[best_wall_id];

                // ça peut être une réflexion ou une transmission
                current_origin = hit_point;
                
                if (reflexion_left != 0 && walls_to_reflect[m * reflection_count + reflexion_left - 1] == best_wall_id) {
                    current_direction = reflection_vector;
                    reflexion_left--;
                    current_direction_to_tx_normalized = normalize(tx - current_origin);
                    current_direction_normalized = normalize(current_direction);
                } else {
                    // energy *= transmission_coef;
                }

                continue;
            }

        break;
        }
    }
    // printf("Compute id : %d has finished, finding %d rays \n", compute_id, ray_count[compute_id]);
}
