#include <CL/cl.h>
#include <cassert>
#include <string>
#include <iostream>
#include <fstream>
#include "wall.h"
#include "zone_tracing_cpu.h"

static std::string read_file(const char* fileName) {
    std::fstream f;
    f.open(fileName, std::ios_base::in);
    assert(f.is_open());

    std::string res;
    while (!f.eof()) {
        char c;
        f.get(c);
        res += c;
    }

    f.close();

    return std::move(res);
}

cl_device_id getDevice() {
    cl_platform_id platforms[64];
    unsigned int platformCount;
    cl_int platformResult = clGetPlatformIDs(64, platforms, &platformCount);

    assert(platformResult == CL_SUCCESS);

    cl_device_id device; // checking for the best device
    for (int i = 0;i < platformCount; i++) {
        cl_device_id devices[64];
        unsigned int deviceCount;
        cl_int deviceResult = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU, 64, devices, &deviceCount);

        if (deviceResult == CL_SUCCESS) {
            for (int j = 0; j < deviceCount; j++) {
                char vendorName[256];
                size_t vendorNameLength;
                cl_int deviceInfoResult = clGetDeviceInfo(devices[j], CL_DEVICE_VENDOR, 256, vendorName, &vendorNameLength);
                char work[256];
                size_t workLength;

                cl_int workInfoResult = clGetDeviceInfo(devices[j], CL_DEVICE_MAX_WORK_GROUP_SIZE, 256, work, &workLength);

                if (deviceInfoResult == CL_SUCCESS && std::string(vendorName).substr(0, vendorNameLength) == "NVIDIA Corporation") {
                    device = devices[j];
                    break;
                }
            }
        }

    }
    return device;

}

void compute_zone(int zone_count, const char* kernel_name, int* numbers, float* coordinates, cl_device_id device) {
    // program initialization 
    cl_int contextResult;
    cl_context context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &contextResult);

    cl_int commandQueueResult;
    cl_command_queue queue = clCreateCommandQueue(context, device, 0, &commandQueueResult);

    std::string s = read_file("zones.cl");
    const char* programSource = s.c_str();
    size_t length = 0;
    cl_int programResult;
    cl_program program = clCreateProgramWithSource(context, 1, &programSource, &length, &programResult);
    assert(programResult == CL_SUCCESS);

    cl_int programBuildResult = clBuildProgram(program, 1, &device, "", nullptr, nullptr);
    if (programBuildResult != CL_SUCCESS) {
        char log[2560];
        size_t logLength;
        cl_int programBuildInfoResult = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 2560, log, &logLength);

        assert(programBuildInfoResult == CL_SUCCESS);
        std::cout << log << std::endl;
        assert(log);
    }

    cl_int kernelResult;
    cl_kernel kernel = clCreateKernel(program, kernel_name, &kernelResult);
    assert(kernelResult == CL_SUCCESS);
    // program initialization completed

    cl_int number_result;
    cl_mem veca = clCreateBuffer(context, CL_MEM_READ_ONLY, zone_count * sizeof(int), nullptr, &number_result);
    assert(number_result == CL_SUCCESS);

    cl_int enqueue_number_result = clEnqueueWriteBuffer(queue, veca, CL_TRUE, 0, zone_count * sizeof(int), numbers, 0, nullptr, nullptr);
    assert(enqueue_number_result == CL_SUCCESS);

    cl_int coordinates_result;
    cl_mem vecb = clCreateBuffer(context, CL_MEM_WRITE_ONLY, zone_count * sizeof(float), nullptr, &coordinates_result);
    assert(coordinates_result == CL_SUCCESS);

    cl_int enqueue_coordinates_result = clEnqueueWriteBuffer(queue, vecb, CL_TRUE, 0, zone_count * sizeof(float), coordinates, 0, nullptr, nullptr);
    assert(enqueue_coordinates_result == CL_SUCCESS);

    cl_int zone_count_x_cl = zone_count;

    // arguments du kernel
    cl_int kernelArgaResult = clSetKernelArg(kernel, 0, sizeof(cl_mem), &veca);
    assert(kernelArgaResult == CL_SUCCESS);
    cl_int kernelArgbResult = clSetKernelArg(kernel, 1, sizeof(cl_mem), &vecb);
    assert(kernelArgbResult == CL_SUCCESS);
    cl_int kernelArgcResult = clSetKernelArg(kernel, 2, sizeof(cl_int), &zone_count_x_cl);
    assert(kernelArgcResult == CL_SUCCESS);

    size_t globalWorkSize = zone_count;
    size_t localWorkSize = 1;
    cl_int enqueueKernelResult = clEnqueueNDRangeKernel(queue, kernel, 1, 0, &globalWorkSize, &localWorkSize, 0, nullptr, nullptr);
    assert(enqueueKernelResult == CL_SUCCESS);

    float* coordinates_data = new float[zone_count];
    cl_int enqueueReadBufferResult = clEnqueueReadBuffer(queue, vecb, CL_TRUE, 0, zone_count * sizeof(float), coordinates_data, 0, nullptr, nullptr);
    assert(enqueueReadBufferResult == CL_SUCCESS);

    clFinish(queue);

    std::copy(coordinates_data, coordinates_data + zone_count, coordinates);

    clReleaseMemObject(veca);
    clReleaseMemObject(vecb);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
}

int compute_zones(int zone_count_x, int zone_count_y, cl_device_id device) {
    // Computes the coordinates of the zones and returns the number of zones
    // Cette fonction ne fait probablement pas gagner de temps d'exécution mais c'est pour s'entrainer

    int total_zones = zone_count_x * zone_count_y;
    int *x_numbers = new int[zone_count_x];
    int *y_numbers = new int[zone_count_y];

    float* x_coordinates = new float[zone_count_x];
    float* y_coordinates = new float[zone_count_y];

    // initializing the coordinates ids
    for (int x = 0; x < zone_count_x; x++) x_numbers[x] = x;
    for (int y = 0; y < zone_count_y; y++) y_numbers[y] = y;

    compute_zone(zone_count_x, "compute_zones_x", x_numbers, x_coordinates, device);
    compute_zone(zone_count_y, "compute_zones_y", y_numbers, y_coordinates, device);

    delete[] x_numbers, y_numbers;

    return total_zones;
}

void run_algo(int zone_count_x, int zone_count_y, int reflection_count, Wall walls[], int wall_count, vec2 tx) {
    // creates all necessary arrays
    int max_count = 1 + (int)pow(wall_count, reflection_count);
    vec2* all_tx_mirrors = new vec2[max_count]; // il y a moyen de calculer tous les tx mirrors, faire le chemin à l'envers mais c'est chiant je pense (j'ai rien dit je l'ai fait)
    int* each_reflection_count = new int[max_count];
    int* walls_to_reflect = new int[max_count * reflection_count]; // represents a 2d array [[r1, r2, ...], [r1, r2, ...], ...]

    int mirror_count = 0;

    generate_all_mirrors(reflection_count, all_tx_mirrors, each_reflection_count, walls_to_reflect,
        walls, wall_count, mirror_count, tx, max_count);

    // generates the zone coordinates

    cl_device_id device = getDevice();

    int* x_numbers = new int[zone_count_x];
    int* y_numbers = new int[zone_count_y];

    float* x_coordinates = new float[zone_count_x];
    float* y_coordinates = new float[zone_count_y];

    // initializing the coordinates ids
    for (int x = 0; x < zone_count_x; x++) x_numbers[x] = x;
    for (int y = 0; y < zone_count_y; y++) y_numbers[y] = y;

    // we now have two arrays, each containing all of the x and y positions of the rx
    compute_zone(zone_count_x, "compute_zones_x", x_numbers, x_coordinates, device);
    compute_zone(zone_count_y, "compute_zones_y", y_numbers, y_coordinates, device);

    delete[] x_numbers, y_numbers;

    // arrays necessary for the kernel

    float* tx_mirror_coordinates_x = new float[mirror_count]();
    float* tx_mirror_coordinates_y = new float[mirror_count]();
    for (int i = 0; i < mirror_count; i++) {
        tx_mirror_coordinates_x[i] = all_tx_mirrors[i].x;
        tx_mirror_coordinates_y[i] = all_tx_mirrors[i].y;
    }

    cl_float2* walls_coordinates_a = new cl_float2[wall_count];
    cl_float2* normalized_walls_u = new cl_float2[wall_count];
    cl_float2* normalized_walls_n = new cl_float2[wall_count];
    cl_float2* walls_u = new cl_float2[wall_count];

    for (int i = 0; i < wall_count; i++) {
        walls_coordinates_a[i] = { walls[i].fancy_vector.a.x, walls[i].fancy_vector.a.y };
        normalized_walls_u[i] = { walls[i].fancy_vector.normalized_u.x, walls[i].fancy_vector.normalized_u.y };
        normalized_walls_n[i] = { walls[i].fancy_vector.normalized_n.x, walls[i].fancy_vector.normalized_n.y };
        walls_u[i] = { walls[i].fancy_vector.u.x, walls[i].fancy_vector.u.y };
    }

    cl_float2 tx_f2 = { tx.x, tx.y };
    cl_int* ray_count = new cl_int[mirror_count]; // output of the code

    // build program and kernel
    cl_int contextResult;
    cl_context context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &contextResult);

    cl_int commandQueueResult;
    cl_command_queue queue = clCreateCommandQueue(context, device, 0, &commandQueueResult);

    std::string s = read_file("solve_zone.cl");
    const char* programSource = s.c_str();
    size_t length = 0;
    cl_int programResult;
    cl_program program = clCreateProgramWithSource(context, 1, &programSource, &length, &programResult);
    assert(programResult == CL_SUCCESS);

    cl_int programBuildResult = clBuildProgram(program, 1, &device, "", nullptr, nullptr);
    if (programBuildResult != CL_SUCCESS) {
        char log[20000];
        size_t logLength;
        cl_int programBuildInfoResult = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 20000, log, &logLength);

        assert(programBuildInfoResult == CL_SUCCESS);
        std::cout << log << std::endl;
        assert(log);
    }

    cl_int kernelResult;
    cl_kernel kernel = clCreateKernel(program, "compute_zone_number", &kernelResult);
    assert(kernelResult == CL_SUCCESS);

    // pass arguments to kernel

    cl_int buffer_result;
    cl_int kernel_result;
    cl_int enqueue_result;

    cl_mem vec_x_coordinates = clCreateBuffer(context, CL_MEM_READ_ONLY, zone_count_x * sizeof(float), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_x_coordinates, CL_TRUE, 0, zone_count_x * sizeof(float), x_coordinates, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 0, sizeof(cl_mem), &vec_x_coordinates);
    assert(kernel_result == CL_SUCCESS);

    cl_mem vec_y_coordinates = clCreateBuffer(context, CL_MEM_READ_ONLY, zone_count_y * sizeof(float), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_y_coordinates, CL_TRUE, 0, zone_count_y * sizeof(float), y_coordinates, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 1, sizeof(cl_mem), &vec_y_coordinates);
    assert(kernel_result == CL_SUCCESS);

    cl_mem vec_tx_mirror_coordinates_x = clCreateBuffer(context, CL_MEM_READ_ONLY, mirror_count * sizeof(float), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_tx_mirror_coordinates_x, CL_TRUE, 0, mirror_count * sizeof(float), tx_mirror_coordinates_x, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 2, sizeof(cl_mem), &vec_tx_mirror_coordinates_x);
    assert(kernel_result == CL_SUCCESS);

    cl_mem vec_tx_mirror_coordinates_y = clCreateBuffer(context, CL_MEM_READ_ONLY, mirror_count * sizeof(float), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_tx_mirror_coordinates_y, CL_TRUE, 0, mirror_count * sizeof(float), tx_mirror_coordinates_y, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 3, sizeof(cl_mem), &vec_tx_mirror_coordinates_y);
    assert(kernel_result == CL_SUCCESS);

    cl_int mirror_count_cl = mirror_count;
    kernel_result = clSetKernelArg(kernel, 4, sizeof(cl_int), &mirror_count_cl);
    assert(kernel_result == CL_SUCCESS);

    cl_int wall_count_cl = wall_count;
    kernel_result = clSetKernelArg(kernel, 5, sizeof(cl_int), &wall_count_cl);
    assert(kernel_result == CL_SUCCESS);

    cl_mem vec_walls_coordinates_a = clCreateBuffer(context, CL_MEM_READ_ONLY, wall_count * sizeof(cl_float2), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_walls_coordinates_a, CL_TRUE, 0, wall_count * sizeof(cl_float2), walls_coordinates_a, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 6, sizeof(cl_mem), &vec_walls_coordinates_a);
    assert(kernel_result == CL_SUCCESS);

    cl_mem vec_normalized_walls_u = clCreateBuffer(context, CL_MEM_READ_ONLY, wall_count * sizeof(cl_float2), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_normalized_walls_u, CL_TRUE, 0, wall_count * sizeof(cl_float2), normalized_walls_u, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 7, sizeof(cl_mem), &vec_normalized_walls_u);
    assert(kernel_result == CL_SUCCESS);

    cl_mem vec_normalized_walls_n = clCreateBuffer(context, CL_MEM_READ_ONLY, wall_count * sizeof(cl_float2), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_normalized_walls_n, CL_TRUE, 0, wall_count * sizeof(cl_float2), normalized_walls_n, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 8, sizeof(cl_mem), &vec_normalized_walls_n);
    assert(kernel_result == CL_SUCCESS);

    cl_mem vec_walls_u = clCreateBuffer(context, CL_MEM_READ_ONLY, wall_count * sizeof(cl_float2), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_walls_u, CL_TRUE, 0, wall_count * sizeof(cl_float2), walls_u, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 9, sizeof(cl_mem), &vec_walls_u);
    assert(kernel_result == CL_SUCCESS);

    cl_mem vec_each_reflection_count = clCreateBuffer(context, CL_MEM_READ_ONLY, mirror_count * sizeof(int), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_each_reflection_count, CL_TRUE, 0, mirror_count * sizeof(int), each_reflection_count, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 10, sizeof(cl_mem), &vec_each_reflection_count);
    assert(kernel_result == CL_SUCCESS);

    cl_mem vec_walls_to_reflect = clCreateBuffer(context, CL_MEM_READ_ONLY, mirror_count * reflection_count * sizeof(int), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_walls_to_reflect, CL_TRUE, 0, mirror_count * reflection_count * sizeof(int), walls_to_reflect, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 11, sizeof(cl_mem), &vec_walls_to_reflect);
    assert(kernel_result == CL_SUCCESS);

    kernel_result = clSetKernelArg(kernel, 12, sizeof(cl_float2), &tx_f2);
    assert(kernel_result == CL_SUCCESS);

    cl_int zone_count_x_cl = zone_count_x;
    kernel_result = clSetKernelArg(kernel, 13, sizeof(int), &zone_count_x_cl);
    assert(kernel_result == CL_SUCCESS);

    cl_mem vec_ray_count = clCreateBuffer(context, CL_MEM_READ_WRITE, mirror_count * sizeof(int), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_ray_count, CL_TRUE, 0, mirror_count * sizeof(int), ray_count, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 14, sizeof(cl_mem), &vec_ray_count);
    assert(kernel_result == CL_SUCCESS);

    cl_int reflection_count_cl = reflection_count;
    kernel_result = clSetKernelArg(kernel, 15, sizeof(int), &reflection_count_cl);
    assert(kernel_result == CL_SUCCESS);

    // program execution
    size_t globalWorkSize = zone_count_x * zone_count_y;
    size_t localWorkSize = 32 * 5; // empiriquement le plus opti
    cl_int enqueueKernelResult = clEnqueueNDRangeKernel(queue, kernel, 1, 0, &globalWorkSize, &localWorkSize, 0, nullptr, nullptr);
    assert(enqueueKernelResult == CL_SUCCESS);

    cl_int enqueueReadBufferResult = clEnqueueReadBuffer(queue, vec_ray_count, CL_TRUE, 0, mirror_count * sizeof(int), ray_count, 0, nullptr, nullptr);
    assert(enqueueReadBufferResult == CL_SUCCESS);

    // memory release
    delete[] x_coordinates, y_coordinates, all_tx_mirrors, each_reflection_count, walls_to_reflect;
    delete[] tx_mirror_coordinates_x, tx_mirror_coordinates_y;
    delete[] walls_coordinates_a, normalized_walls_u, normalized_walls_n, walls_u;

    clReleaseMemObject(vec_x_coordinates);
    clReleaseMemObject(vec_y_coordinates);
    clReleaseMemObject(vec_tx_mirror_coordinates_x);
    clReleaseMemObject(vec_tx_mirror_coordinates_y);
    clReleaseMemObject(vec_walls_coordinates_a);
    clReleaseMemObject(vec_normalized_walls_u);
    clReleaseMemObject(vec_normalized_walls_n);
    clReleaseMemObject(vec_each_reflection_count);
    clReleaseMemObject(vec_walls_to_reflect);
    clReleaseMemObject(vec_walls_u);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseDevice(device);
}

void run_algo_antenna(int zone_count_x, int zone_count_y, int reflection_count, Wall walls[], int wall_count, vec2 tx, float pulsation, float* energy,
    int* ray_count, int antenna_id, float antenna_angle) {
    // creates all necessary arrays
    int max_count = 1 + (int)pow(wall_count, reflection_count);
    vec2* all_tx_mirrors = new vec2[max_count]; // il y a moyen de calculer tous les tx mirrors, faire le chemin à l'envers mais c'est chiant je pense (j'ai rien dit je l'ai fait)
    int* each_reflection_count = new int[max_count];
    int* walls_to_reflect = new int[max_count * reflection_count]; // represents a 2d array [[r1, r2, ...], [r1, r2, ...], ...]

    int mirror_count = 0;
    float beta = pulsation / (299792458.f);

    generate_all_mirrors(reflection_count, all_tx_mirrors, each_reflection_count, walls_to_reflect,
        walls, wall_count, mirror_count, tx, max_count);

    // generates the zone coordinates

    cl_device_id device = getDevice();

    int* x_numbers = new int[zone_count_x];
    int* y_numbers = new int[zone_count_y];

    float* x_coordinates = new float[zone_count_x];
    float* y_coordinates = new float[zone_count_y];

    // initializing the coordinates ids
    for (int x = 0; x < zone_count_x; x++) x_numbers[x] = x;
    for (int y = 0; y < zone_count_y; y++) y_numbers[y] = y;

    // we now have two arrays, each containing all of the x and y positions of the rx
    compute_zone(zone_count_x, "compute_zones_x", x_numbers, x_coordinates, device);
    compute_zone(zone_count_y, "compute_zones_y", y_numbers, y_coordinates, device);

    delete[] x_numbers, y_numbers;

    // arrays necessary for the kernel

    float* tx_mirror_coordinates_x = new float[mirror_count];
    float* tx_mirror_coordinates_y = new float[mirror_count];

    for (int i = 0; i < mirror_count; i++) {
        tx_mirror_coordinates_x[i] = all_tx_mirrors[i].x;
        tx_mirror_coordinates_y[i] = all_tx_mirrors[i].y;
    }

    cl_float2* walls_coordinates_a = new cl_float2[wall_count];
    cl_float2* normalized_walls_u = new cl_float2[wall_count];
    cl_float2* normalized_walls_n = new cl_float2[wall_count];
    cl_float2* walls_u = new cl_float2[wall_count];
    float* walls_relative_perm = new float[wall_count];
    float* walls_depth = new float[wall_count];
    cl_float2* walls_impedance = new cl_float2[wall_count];
    cl_float2* walls_gamma = new cl_float2[wall_count];

    for (int i = 0; i < wall_count; i++) {
        walls_coordinates_a[i] = { walls[i].fancy_vector.a.x, walls[i].fancy_vector.a.y };
        normalized_walls_u[i] = { walls[i].fancy_vector.normalized_u.x, walls[i].fancy_vector.normalized_u.y };
        normalized_walls_n[i] = { walls[i].fancy_vector.normalized_n.x, walls[i].fancy_vector.normalized_n.y };
        walls_u[i] = { walls[i].fancy_vector.u.x, walls[i].fancy_vector.u.y };
        walls_relative_perm[i] = walls[i].relative_perm;
        walls_depth[i] = walls[i].depth;
        walls_impedance[i] = { walls[i].impedance.real(), walls[i].impedance.imag() };
        walls_gamma[i] = { walls[i].gamma.real(), walls[i].gamma.imag() };
    }

    cl_float2 tx_f2 = { tx.x, tx.y };

    // build program and kernel
    cl_int contextResult;
    cl_context context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &contextResult);

    cl_int commandQueueResult;
    cl_command_queue queue = clCreateCommandQueue(context, device, 0, &commandQueueResult);

    std::string s = read_file("zone_antenna.cl");
    const char* programSource = s.c_str();
    size_t length = 0;
    cl_int programResult;
    cl_program program = clCreateProgramWithSource(context, 1, &programSource, &length, &programResult);
    assert(programResult == CL_SUCCESS);

    cl_int programBuildResult = clBuildProgram(program, 1, &device, "", nullptr, nullptr);
    if (programBuildResult != CL_SUCCESS) {
        char log[20000];
        size_t logLength;
        cl_int programBuildInfoResult = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 20000, log, &logLength);

        assert(programBuildInfoResult == CL_SUCCESS);
        std::cout << log << std::endl;
        assert(log);
    }

    cl_int kernelResult;
    cl_kernel kernel = clCreateKernel(program, "compute_zone", &kernelResult);
    assert(kernelResult == CL_SUCCESS);

    // pass arguments to kernel

    cl_int buffer_result;
    cl_int kernel_result;
    cl_int enqueue_result;

    cl_mem vec_x_coordinates = clCreateBuffer(context, CL_MEM_READ_ONLY, zone_count_x * sizeof(float), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_x_coordinates, CL_TRUE, 0, zone_count_x * sizeof(float), x_coordinates, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 0, sizeof(cl_mem), &vec_x_coordinates);
    assert(kernel_result == CL_SUCCESS);

    cl_mem vec_y_coordinates = clCreateBuffer(context, CL_MEM_READ_ONLY, zone_count_y * sizeof(float), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_y_coordinates, CL_TRUE, 0, zone_count_y * sizeof(float), y_coordinates, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 1, sizeof(cl_mem), &vec_y_coordinates);
    assert(kernel_result == CL_SUCCESS);

    cl_mem vec_tx_mirror_coordinates_x = clCreateBuffer(context, CL_MEM_READ_ONLY, mirror_count * sizeof(float), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_tx_mirror_coordinates_x, CL_TRUE, 0, mirror_count * sizeof(float), tx_mirror_coordinates_x, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 2, sizeof(cl_mem), &vec_tx_mirror_coordinates_x);
    assert(kernel_result == CL_SUCCESS);

    cl_mem vec_tx_mirror_coordinates_y = clCreateBuffer(context, CL_MEM_READ_ONLY, mirror_count * sizeof(float), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_tx_mirror_coordinates_y, CL_TRUE, 0, mirror_count * sizeof(float), tx_mirror_coordinates_y, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 3, sizeof(cl_mem), &vec_tx_mirror_coordinates_y);
    assert(kernel_result == CL_SUCCESS);

    cl_int mirror_count_cl = mirror_count;
    kernel_result = clSetKernelArg(kernel, 4, sizeof(cl_int), &mirror_count_cl);
    assert(kernel_result == CL_SUCCESS);

    cl_int wall_count_cl = wall_count;
    kernel_result = clSetKernelArg(kernel, 5, sizeof(cl_int), &wall_count_cl);
    assert(kernel_result == CL_SUCCESS);

    cl_mem vec_walls_coordinates_a = clCreateBuffer(context, CL_MEM_READ_ONLY, wall_count * sizeof(cl_float2), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_walls_coordinates_a, CL_TRUE, 0, wall_count * sizeof(cl_float2), walls_coordinates_a, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 6, sizeof(cl_mem), &vec_walls_coordinates_a);
    assert(kernel_result == CL_SUCCESS);

    cl_mem vec_normalized_walls_u = clCreateBuffer(context, CL_MEM_READ_ONLY, wall_count * sizeof(cl_float2), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_normalized_walls_u, CL_TRUE, 0, wall_count * sizeof(cl_float2), normalized_walls_u, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 7, sizeof(cl_mem), &vec_normalized_walls_u);
    assert(kernel_result == CL_SUCCESS);

    cl_mem vec_normalized_walls_n = clCreateBuffer(context, CL_MEM_READ_ONLY, wall_count * sizeof(cl_float2), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_normalized_walls_n, CL_TRUE, 0, wall_count * sizeof(cl_float2), normalized_walls_n, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 8, sizeof(cl_mem), &vec_normalized_walls_n);
    assert(kernel_result == CL_SUCCESS);

    cl_mem vec_walls_u = clCreateBuffer(context, CL_MEM_READ_ONLY, wall_count * sizeof(cl_float2), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_walls_u, CL_TRUE, 0, wall_count * sizeof(cl_float2), walls_u, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 9, sizeof(cl_mem), &vec_walls_u);
    assert(kernel_result == CL_SUCCESS);

    cl_mem vec_walls_rel_perm = clCreateBuffer(context, CL_MEM_READ_ONLY, wall_count * sizeof(float), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_walls_rel_perm, CL_TRUE, 0, wall_count * sizeof(float), walls_relative_perm, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 10, sizeof(cl_mem), &vec_walls_rel_perm);
    assert(kernel_result == CL_SUCCESS);

    cl_mem vec_walls_depth = clCreateBuffer(context, CL_MEM_READ_ONLY, wall_count * sizeof(float), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_walls_depth, CL_TRUE, 0, wall_count * sizeof(float), walls_depth, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 11, sizeof(cl_mem), &vec_walls_depth);
    assert(kernel_result == CL_SUCCESS);

    cl_mem vec_walls_impedance = clCreateBuffer(context, CL_MEM_READ_ONLY, wall_count * sizeof(cl_float2), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_walls_impedance, CL_TRUE, 0, wall_count * sizeof(cl_float2), walls_impedance, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 12, sizeof(cl_mem), &vec_walls_impedance);
    assert(kernel_result == CL_SUCCESS);

    cl_float2 air_impedance = { compute_impedance(1.f, 0.f, pulsation).real(), compute_impedance(1.f, 0.f, pulsation).imag() };
    kernel_result = clSetKernelArg(kernel, 13, sizeof(cl_float2), &air_impedance);
    assert(kernel_result == CL_SUCCESS);

    cl_mem vec_walls_gamma = clCreateBuffer(context, CL_MEM_READ_ONLY, wall_count * sizeof(cl_float2), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_walls_gamma, CL_TRUE, 0, wall_count * sizeof(cl_float2), walls_gamma, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 14, sizeof(cl_mem), &vec_walls_gamma);
    assert(kernel_result == CL_SUCCESS);

    cl_mem vec_each_reflection_count = clCreateBuffer(context, CL_MEM_READ_ONLY, mirror_count * sizeof(int), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_each_reflection_count, CL_TRUE, 0, mirror_count * sizeof(int), each_reflection_count, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 15, sizeof(cl_mem), &vec_each_reflection_count);
    assert(kernel_result == CL_SUCCESS);

    cl_mem vec_walls_to_reflect = clCreateBuffer(context, CL_MEM_READ_ONLY, mirror_count * reflection_count * sizeof(int), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_walls_to_reflect, CL_TRUE, 0, mirror_count * reflection_count * sizeof(int), walls_to_reflect, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 16, sizeof(cl_mem), &vec_walls_to_reflect);
    assert(kernel_result == CL_SUCCESS);

    kernel_result = clSetKernelArg(kernel, 17, sizeof(cl_float2), &tx_f2);
    assert(kernel_result == CL_SUCCESS);

    cl_int zone_count_x_cl = zone_count_x;
    kernel_result = clSetKernelArg(kernel, 18, sizeof(int), &zone_count_x_cl);
    assert(kernel_result == CL_SUCCESS);

    cl_mem vec_ray_count = clCreateBuffer(context, CL_MEM_READ_WRITE, zone_count_x * zone_count_y * sizeof(int), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_ray_count, CL_TRUE, 0, zone_count_x * zone_count_y * sizeof(int), ray_count, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 19, sizeof(cl_mem), &vec_ray_count);
    assert(kernel_result == CL_SUCCESS);

    cl_int reflection_count_cl = reflection_count;
    kernel_result = clSetKernelArg(kernel, 20, sizeof(int), &reflection_count_cl);
    assert(kernel_result == CL_SUCCESS);

    cl_mem vec_energy = clCreateBuffer(context, CL_MEM_READ_WRITE, zone_count_x * zone_count_y * sizeof(float), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_energy, CL_TRUE, 0, zone_count_x * zone_count_y * sizeof(float), energy, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 21, sizeof(cl_mem), &vec_energy);
    assert(kernel_result == CL_SUCCESS);

    kernel_result = clSetKernelArg(kernel, 22, sizeof(float), &beta);
    assert(kernel_result == CL_SUCCESS);

    kernel_result = clSetKernelArg(kernel, 23, sizeof(float), &pulsation);
    assert(kernel_result == CL_SUCCESS);

    kernel_result = clSetKernelArg(kernel, 24, sizeof(int), &antenna_id);
    assert(kernel_result == CL_SUCCESS);

    kernel_result = clSetKernelArg(kernel, 25, sizeof(float), &antenna_angle);
    assert(kernel_result == CL_SUCCESS);

    // program execution
    size_t globalWorkSize = zone_count_x * zone_count_y;
    size_t localWorkSize = 28; // 32;// *5; // empiriquement le plus opti pour 200 * 140 zones
    cl_int enqueueKernelResult = clEnqueueNDRangeKernel(queue, kernel, 1, 0, &globalWorkSize, &localWorkSize, 0, nullptr, nullptr);
    assert(enqueueKernelResult == CL_SUCCESS);

    cl_int enqueueReadBufferResult = clEnqueueReadBuffer(queue, vec_ray_count, CL_TRUE, 0, zone_count_x * zone_count_y * sizeof(cl_int), ray_count, 0, nullptr, nullptr);
    assert(enqueueReadBufferResult == CL_SUCCESS);

    enqueueReadBufferResult = clEnqueueReadBuffer(queue, vec_energy, CL_TRUE, 0, zone_count_x * zone_count_y * sizeof(cl_float), energy, 0, nullptr, nullptr);
    assert(enqueueReadBufferResult == CL_SUCCESS);

    // everything should be in ray_count
    for (int i = 0; i < zone_count_x * zone_count_y; i++) {
        int a = ray_count[i];
        float e = energy[i];
    }

    // memory release
    delete[] x_coordinates, y_coordinates, all_tx_mirrors, each_reflection_count, walls_to_reflect;
    delete[] tx_mirror_coordinates_x, tx_mirror_coordinates_y;
    delete[] walls_coordinates_a, normalized_walls_u, normalized_walls_n, walls_u;
    delete[] walls_relative_perm, walls_depth, walls_impedance, walls_gamma;

    clReleaseMemObject(vec_x_coordinates);
    clReleaseMemObject(vec_y_coordinates);
    clReleaseMemObject(vec_tx_mirror_coordinates_x);
    clReleaseMemObject(vec_tx_mirror_coordinates_y);
    clReleaseMemObject(vec_walls_coordinates_a);
    clReleaseMemObject(vec_normalized_walls_u);
    clReleaseMemObject(vec_normalized_walls_n);
    clReleaseMemObject(vec_each_reflection_count);
    clReleaseMemObject(vec_walls_to_reflect);
    clReleaseMemObject(vec_walls_u);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseDevice(device);
}

void run_final_algo(int zone_count_x, int zone_count_y, int reflection_count, Wall walls[], int wall_count, vec2 tx, float pulsation, float* energy,
        int* ray_count) {
    // creates all necessary arrays
    int max_count = 1 + (int)pow(wall_count, reflection_count);
    vec2* all_tx_mirrors = new vec2[max_count]; // il y a moyen de calculer tous les tx mirrors, faire le chemin à l'envers mais c'est chiant je pense (j'ai rien dit je l'ai fait)
    int* each_reflection_count = new int[max_count];
    int* walls_to_reflect = new int[max_count * reflection_count]; // represents a 2d array [[r1, r2, ...], [r1, r2, ...], ...]

    int mirror_count = 0;
    float beta = pulsation / (299792458.f);

    generate_all_mirrors(reflection_count, all_tx_mirrors, each_reflection_count, walls_to_reflect,
        walls, wall_count, mirror_count, tx, max_count);

    // generates the zone coordinates

    cl_device_id device = getDevice();

    int* x_numbers = new int[zone_count_x];
    int* y_numbers = new int[zone_count_y];

    float* x_coordinates = new float[zone_count_x];
    float* y_coordinates = new float[zone_count_y];

    // initializing the coordinates ids
    for (int x = 0; x < zone_count_x; x++) x_numbers[x] = x;
    for (int y = 0; y < zone_count_y; y++) y_numbers[y] = y;

    // we now have two arrays, each containing all of the x and y positions of the rx
    compute_zone(zone_count_x, "compute_zones_x", x_numbers, x_coordinates, device);
    compute_zone(zone_count_y, "compute_zones_y", y_numbers, y_coordinates, device);

    delete[] x_numbers, y_numbers;

    // arrays necessary for the kernel

    float* tx_mirror_coordinates_x = new float[mirror_count];
    float* tx_mirror_coordinates_y = new float[mirror_count];
    
    for (int i = 0; i < mirror_count; i++) {
        tx_mirror_coordinates_x[i] = all_tx_mirrors[i].x;
        tx_mirror_coordinates_y[i] = all_tx_mirrors[i].y;
    }

    cl_float2* walls_coordinates_a = new cl_float2[wall_count];
    cl_float2* normalized_walls_u = new cl_float2[wall_count];
    cl_float2* normalized_walls_n = new cl_float2[wall_count];
    cl_float2* walls_u = new cl_float2[wall_count];
    float* walls_relative_perm = new float[wall_count];
    float* walls_depth = new float[wall_count];
    cl_float2* walls_impedance = new cl_float2[wall_count];
    cl_float2* walls_gamma = new cl_float2[wall_count];

    for (int i = 0; i < wall_count; i++) {
        walls_coordinates_a[i] = { walls[i].fancy_vector.a.x, walls[i].fancy_vector.a.y };
        normalized_walls_u[i] = { walls[i].fancy_vector.normalized_u.x, walls[i].fancy_vector.normalized_u.y };
        normalized_walls_n[i] = { walls[i].fancy_vector.normalized_n.x, walls[i].fancy_vector.normalized_n.y };
        walls_u[i] = { walls[i].fancy_vector.u.x, walls[i].fancy_vector.u.y };
        walls_relative_perm[i] = walls[i].relative_perm;
        walls_depth[i] = walls[i].depth;
        walls_impedance[i] = { walls[i].impedance.real(), walls[i].impedance.imag() };
        walls_gamma[i] = { walls[i].gamma.real(), walls[i].gamma.imag()};
    }

    cl_float2 tx_f2 = { tx.x, tx.y };

    // build program and kernel
    cl_int contextResult;
    cl_context context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &contextResult);

    cl_int commandQueueResult;
    cl_command_queue queue = clCreateCommandQueue(context, device, 0, &commandQueueResult);

    std::string s = read_file("solve_zone.cl");
    const char* programSource = s.c_str();
    size_t length = 0;
    cl_int programResult;
    cl_program program = clCreateProgramWithSource(context, 1, &programSource, &length, &programResult);
    assert(programResult == CL_SUCCESS);

    cl_int programBuildResult = clBuildProgram(program, 1, &device, "", nullptr, nullptr);
    if (programBuildResult != CL_SUCCESS) {
        char log[20000];
        size_t logLength;
        cl_int programBuildInfoResult = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 20000, log, &logLength);

        assert(programBuildInfoResult == CL_SUCCESS);
        std::cout << log << std::endl;
        assert(log);
    }

    cl_int kernelResult;
    cl_kernel kernel = clCreateKernel(program, "compute_zone", &kernelResult);
    assert(kernelResult == CL_SUCCESS);

    // pass arguments to kernel

    cl_int buffer_result;
    cl_int kernel_result;
    cl_int enqueue_result;

    cl_mem vec_x_coordinates = clCreateBuffer(context, CL_MEM_READ_ONLY, zone_count_x * sizeof(float), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_x_coordinates, CL_TRUE, 0, zone_count_x * sizeof(float), x_coordinates, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 0, sizeof(cl_mem), &vec_x_coordinates);
    assert(kernel_result == CL_SUCCESS);

    cl_mem vec_y_coordinates = clCreateBuffer(context, CL_MEM_READ_ONLY, zone_count_y * sizeof(float), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_y_coordinates, CL_TRUE, 0, zone_count_y * sizeof(float), y_coordinates, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 1, sizeof(cl_mem), &vec_y_coordinates);
    assert(kernel_result == CL_SUCCESS);

    cl_mem vec_tx_mirror_coordinates_x = clCreateBuffer(context, CL_MEM_READ_ONLY, mirror_count * sizeof(float), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_tx_mirror_coordinates_x, CL_TRUE, 0, mirror_count * sizeof(float), tx_mirror_coordinates_x, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 2, sizeof(cl_mem), &vec_tx_mirror_coordinates_x);
    assert(kernel_result == CL_SUCCESS);

    cl_mem vec_tx_mirror_coordinates_y = clCreateBuffer(context, CL_MEM_READ_ONLY, mirror_count * sizeof(float), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_tx_mirror_coordinates_y, CL_TRUE, 0, mirror_count * sizeof(float), tx_mirror_coordinates_y, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 3, sizeof(cl_mem), &vec_tx_mirror_coordinates_y);
    assert(kernel_result == CL_SUCCESS);

    cl_int mirror_count_cl = mirror_count;
    kernel_result = clSetKernelArg(kernel, 4, sizeof(cl_int), &mirror_count_cl);
    assert(kernel_result == CL_SUCCESS);

    cl_int wall_count_cl = wall_count;
    kernel_result = clSetKernelArg(kernel, 5, sizeof(cl_int), &wall_count_cl);
    assert(kernel_result == CL_SUCCESS);

    cl_mem vec_walls_coordinates_a = clCreateBuffer(context, CL_MEM_READ_ONLY, wall_count * sizeof(cl_float2), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_walls_coordinates_a, CL_TRUE, 0, wall_count * sizeof(cl_float2), walls_coordinates_a, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 6, sizeof(cl_mem), &vec_walls_coordinates_a);
    assert(kernel_result == CL_SUCCESS);

    cl_mem vec_normalized_walls_u = clCreateBuffer(context, CL_MEM_READ_ONLY, wall_count * sizeof(cl_float2), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_normalized_walls_u, CL_TRUE, 0, wall_count * sizeof(cl_float2), normalized_walls_u, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 7, sizeof(cl_mem), &vec_normalized_walls_u);
    assert(kernel_result == CL_SUCCESS);

    cl_mem vec_normalized_walls_n = clCreateBuffer(context, CL_MEM_READ_ONLY, wall_count * sizeof(cl_float2), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_normalized_walls_n, CL_TRUE, 0, wall_count * sizeof(cl_float2), normalized_walls_n, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 8, sizeof(cl_mem), &vec_normalized_walls_n);
    assert(kernel_result == CL_SUCCESS);

    cl_mem vec_walls_u = clCreateBuffer(context, CL_MEM_READ_ONLY, wall_count * sizeof(cl_float2), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_walls_u, CL_TRUE, 0, wall_count * sizeof(cl_float2), walls_u, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 9, sizeof(cl_mem), &vec_walls_u);
    assert(kernel_result == CL_SUCCESS);

    cl_mem vec_walls_rel_perm = clCreateBuffer(context, CL_MEM_READ_ONLY, wall_count * sizeof(float), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_walls_rel_perm, CL_TRUE, 0, wall_count * sizeof(float), walls_relative_perm, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 10, sizeof(cl_mem), &vec_walls_rel_perm);
    assert(kernel_result == CL_SUCCESS);

    cl_mem vec_walls_depth = clCreateBuffer(context, CL_MEM_READ_ONLY, wall_count * sizeof(float), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_walls_depth, CL_TRUE, 0, wall_count * sizeof(float), walls_depth, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 11, sizeof(cl_mem), &vec_walls_depth);
    assert(kernel_result == CL_SUCCESS);

    cl_mem vec_walls_impedance = clCreateBuffer(context, CL_MEM_READ_ONLY, wall_count * sizeof(cl_float2), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_walls_impedance, CL_TRUE, 0, wall_count * sizeof(cl_float2), walls_impedance, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 12, sizeof(cl_mem), &vec_walls_impedance);
    assert(kernel_result == CL_SUCCESS);

    cl_float2 air_impedance = { compute_impedance(1.f, 0.f, pulsation).real(), compute_impedance(1.f, 0.f, pulsation).imag() };
    kernel_result = clSetKernelArg(kernel, 13, sizeof(cl_float2), &air_impedance);
    assert(kernel_result == CL_SUCCESS);

    cl_mem vec_walls_gamma = clCreateBuffer(context, CL_MEM_READ_ONLY, wall_count * sizeof(cl_float2), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_walls_gamma, CL_TRUE, 0, wall_count * sizeof(cl_float2), walls_gamma, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 14, sizeof(cl_mem), &vec_walls_gamma);
    assert(kernel_result == CL_SUCCESS);

    cl_mem vec_each_reflection_count = clCreateBuffer(context, CL_MEM_READ_ONLY, mirror_count * sizeof(int), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_each_reflection_count, CL_TRUE, 0, mirror_count * sizeof(int), each_reflection_count, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 15, sizeof(cl_mem), &vec_each_reflection_count);
    assert(kernel_result == CL_SUCCESS);

    cl_mem vec_walls_to_reflect = clCreateBuffer(context, CL_MEM_READ_ONLY, mirror_count * reflection_count * sizeof(int), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_walls_to_reflect, CL_TRUE, 0, mirror_count * reflection_count * sizeof(int), walls_to_reflect, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 16, sizeof(cl_mem), &vec_walls_to_reflect);
    assert(kernel_result == CL_SUCCESS);

    kernel_result = clSetKernelArg(kernel, 17, sizeof(cl_float2), &tx_f2);
    assert(kernel_result == CL_SUCCESS);

    cl_int zone_count_x_cl = zone_count_x;
    kernel_result = clSetKernelArg(kernel, 18, sizeof(int), &zone_count_x_cl);
    assert(kernel_result == CL_SUCCESS);

    cl_mem vec_ray_count = clCreateBuffer(context, CL_MEM_READ_WRITE, zone_count_x * zone_count_y * sizeof(int), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_ray_count, CL_TRUE, 0, zone_count_x * zone_count_y * sizeof(int), ray_count, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 19, sizeof(cl_mem), &vec_ray_count);
    assert(kernel_result == CL_SUCCESS);

    cl_int reflection_count_cl = reflection_count;
    kernel_result = clSetKernelArg(kernel, 20, sizeof(int), &reflection_count_cl);
    assert(kernel_result == CL_SUCCESS);

    cl_mem vec_energy = clCreateBuffer(context, CL_MEM_READ_WRITE, zone_count_x * zone_count_y * sizeof(float), nullptr, &buffer_result);
    assert(buffer_result == CL_SUCCESS);
    enqueue_result = clEnqueueWriteBuffer(queue, vec_energy, CL_TRUE, 0, zone_count_x * zone_count_y * sizeof(float), energy, 0, nullptr, nullptr);
    assert(enqueue_result == CL_SUCCESS);
    kernel_result = clSetKernelArg(kernel, 21, sizeof(cl_mem), &vec_energy);
    assert(kernel_result == CL_SUCCESS);

    kernel_result = clSetKernelArg(kernel, 22, sizeof(float), &beta);
    assert(kernel_result == CL_SUCCESS);

    kernel_result = clSetKernelArg(kernel, 23, sizeof(float), &pulsation);
    assert(kernel_result == CL_SUCCESS);

    // program execution
    size_t globalWorkSize = zone_count_x * zone_count_y;
    size_t localWorkSize = 32 * 5; // empiriquement le plus opti
    cl_int enqueueKernelResult = clEnqueueNDRangeKernel(queue, kernel, 1, 0, &globalWorkSize, &localWorkSize, 0, nullptr, nullptr);
    assert(enqueueKernelResult == CL_SUCCESS);

    cl_int enqueueReadBufferResult = clEnqueueReadBuffer(queue, vec_ray_count, CL_TRUE, 0, zone_count_x * zone_count_y * sizeof(cl_int), ray_count, 0, nullptr, nullptr);
    assert(enqueueReadBufferResult == CL_SUCCESS);

    enqueueReadBufferResult = clEnqueueReadBuffer(queue, vec_energy, CL_TRUE, 0, zone_count_x * zone_count_y * sizeof(cl_float), energy, 0, nullptr, nullptr);
    assert(enqueueReadBufferResult == CL_SUCCESS);

    // everything should be in ray_count
    for (int i = 0; i < zone_count_x * zone_count_y; i++) {
        int a = ray_count[i];
        float e = energy[i];
    }

    // memory release
    delete[] x_coordinates, y_coordinates, all_tx_mirrors, each_reflection_count, walls_to_reflect;
    delete[] tx_mirror_coordinates_x, tx_mirror_coordinates_y;
    delete[] walls_coordinates_a, normalized_walls_u, normalized_walls_n, walls_u;
    delete[] walls_relative_perm, walls_depth, walls_impedance, walls_gamma;

    clReleaseMemObject(vec_x_coordinates);
    clReleaseMemObject(vec_y_coordinates);
    clReleaseMemObject(vec_tx_mirror_coordinates_x);
    clReleaseMemObject(vec_tx_mirror_coordinates_y);
    clReleaseMemObject(vec_walls_coordinates_a);
    clReleaseMemObject(vec_normalized_walls_u);
    clReleaseMemObject(vec_normalized_walls_n);
    clReleaseMemObject(vec_each_reflection_count);
    clReleaseMemObject(vec_walls_to_reflect);
    clReleaseMemObject(vec_walls_u);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseDevice(device);
}

void test_opencl() {
    cl_device_id device = getDevice();

    int zone_count_x = 200;
    int zone_count_y = 140;

    int* x_numbers = new int[zone_count_x];
    int* y_numbers = new int[zone_count_y];

    float* x_coordinates = new float[zone_count_x];
    float* y_coordinates = new float[zone_count_y];

    // initializing the coordinates ids
    for (int x = 0; x < zone_count_x; x++) x_numbers[x] = x;
    for (int y = 0; y < zone_count_y; y++) y_numbers[y] = y;

    // we now have two arrays, each containing all of the x and y positions of the rx
    compute_zone(zone_count_x, "compute_zones_x", x_numbers, x_coordinates, device);
    compute_zone(zone_count_y, "compute_zones_y", y_numbers, y_coordinates, device);

    delete[] x_numbers, y_numbers;
    // now for the fun part

    delete[] x_coordinates, y_coordinates;
    clReleaseDevice(device);
}

/*
void test_opencl() {
    cl_device_id device = getDevice();

    cl_int contextResult;
    cl_context context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &contextResult);

    cl_int commandQueueResult;
    cl_command_queue queue = clCreateCommandQueue(context, device, 0, &commandQueueResult);

    std::string s = read_file("vecsum.cl");
    const char* programSource = s.c_str();
    size_t length = 0;
    cl_int programResult;
    cl_program program = clCreateProgramWithSource(context, 1, &programSource, &length, &programResult);
    assert(programResult == CL_SUCCESS);

    cl_int programBuildResult = clBuildProgram(program, 1, &device, "", nullptr, nullptr);
    if (programBuildResult != CL_SUCCESS) {
        char log[256];
        size_t logLength;
        cl_int programBuildInfoResult = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 256, log, &logLength);
        assert(programBuildInfoResult == CL_SUCCESS);
        std::cout << log << std::endl;
        assert(log);
    }

    cl_int kernelResult;
    cl_kernel kernel = clCreateKernel(program, "vector_sum", &kernelResult);
    assert(kernelResult == CL_SUCCESS);

    float vecaData[256];
    float vecbData[256];

    for (int i = 0; i < 256; ++i) {
        vecaData[i] = (float)(i * i);
        vecbData[i] = (float)i;
    }

    cl_int vecaResult;
    cl_mem veca = clCreateBuffer(context, CL_MEM_READ_ONLY, 256 * sizeof(float), nullptr, &vecaResult);
    assert(vecaResult == CL_SUCCESS);

    cl_int enqueueVecaResult = clEnqueueWriteBuffer(queue, veca, CL_TRUE, 0, 256 * sizeof(float), vecaData, 0, nullptr, nullptr);
    assert(enqueueVecaResult == CL_SUCCESS);

    cl_int vecbResult;
    cl_mem vecb = clCreateBuffer(context, CL_MEM_READ_ONLY, 256 * sizeof(float), nullptr, &vecbResult);
    assert(vecbResult == CL_SUCCESS);

    cl_int enqueueVecbResult = clEnqueueWriteBuffer(queue, vecb, CL_TRUE, 0, 256 * sizeof(float), vecbData, 0, nullptr, nullptr);
    assert(enqueueVecbResult == CL_SUCCESS);

    cl_int veccResult;
    cl_mem vecc = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 256 * sizeof(float), nullptr, &veccResult);
    assert(veccResult == CL_SUCCESS);

    cl_int kernelArgaResult = clSetKernelArg(kernel, 0, sizeof(cl_mem), &veca);
    assert(kernelArgaResult == CL_SUCCESS);
    cl_int kernelArgbResult = clSetKernelArg(kernel, 1, sizeof(cl_mem), &vecb);
    assert(kernelArgaResult == CL_SUCCESS);
    cl_int kernelArgcResult = clSetKernelArg(kernel, 2, sizeof(cl_mem), &vecc);
    assert(kernelArgaResult == CL_SUCCESS);

    size_t globalWorkSize = 256;
    size_t localWorkSize = 64;
    cl_int enqueueKernelResult = clEnqueueNDRangeKernel(queue, kernel, 1, 0, &globalWorkSize, &localWorkSize, 0, nullptr, nullptr);
    assert(enqueueKernelResult == CL_SUCCESS);

    float veccData[256];
    cl_int enqueueReadBufferResult = clEnqueueReadBuffer(queue, vecc, CL_TRUE, 0, 256 * sizeof(float), veccData, 0, nullptr, nullptr);
    assert(enqueueReadBufferResult == CL_SUCCESS);

    clFinish(queue); // on est sûr qu'il a fini, clFlush si on veut utiliser le cpu en attendant

    std::cout << "Result: ";
    for (int i = 0; i < 256; ++i) {

        std::cout << veccData[i] << std::endl;
    }

    clReleaseMemObject(veca);
    clReleaseMemObject(vecb);
    clReleaseMemObject(vecc);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    clReleaseDevice(device);
}
*/
