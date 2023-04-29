__kernel void compute_zones_x(__global int* x_id, __global float* x_coordinate, int zone_count_x){
    int compute_id = get_global_id(0);

    x_coordinate[x_id[compute_id]] = (x_id[compute_id] + 0.5f) * 1000.f / zone_count_x;
}

__kernel void compute_zones_y(__global int* y_id, __global float* y_coordinate, int zone_count_y){
    int compute_id = get_global_id(0);

    y_coordinate[y_id[compute_id]] = -(y_id[compute_id] + 0.5f) * 700.f / zone_count_y;
} 
