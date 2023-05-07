#include "ex_8_1.h"
#include <complex>
#include "wall.h"
#include "ray.h"
#include "coefficients.h"

#include "../libs/nlohmann/json.hpp"
#include "zone_tracing.h"
#include "rx_field.h"

using json = nlohmann::json;

void compute_everything_8() {
    float pulsation = 2.f * 3.141592f * 868.3f * pow(10, 6);
    std::complex<float> Z0 = compute_impedance(1.f, 0, 1); // 377 ohms ✅
    std::complex<float> Z1 = compute_impedance(4.8f, 0.018, pulsation); // 171.57 + i*6.65 ✅

    Wall w(0, 4.8f, 0.018f, pulsation, FancyVector{}, 0.15, 0);

    // ex 8
    coefficients c = compute_reflection_coefficients(0.9648f, Z0, w);

    float total_transmission = compute_total_transmission(0.9648f, c, w);
    std::complex<float> total_transmission_c = compute_total_transmission_coefficient(0.9648f, c, w);

    float alpha = compute_alpha(4.8f, 0.018f, pulsation); // 1.55 ✅
    float beta = compute_beta(4.8f, 0.018f, pulsation); // 39.9 ✅

    // actual exercice

    vec2 rx(470, 650);
    vec2 tx(320, 100);

    std::ifstream file_plan("plan_ex8.json");
    json walls_data = json::parse(file_plan);
    std::ifstream file_material("materials.json");
    json materials_data = json::parse(file_material);


    // loading walls
    Wall* walls = new Wall[walls_data.size()];
    for (int i = 0; i < walls_data.size(); i++) {
        vec2 a((float)walls_data[i]["point_ax"] * 10.f, (float)walls_data[i]["point_ay"] * 10.f);
        vec2 b((float)walls_data[i]["point_bx"] * 10.f, (float)walls_data[i]["point_by"] * 10.f);

        FancyVector temp_vector(a, b);
        int material = walls_data[i]["material_id"] - 1;
        walls[i] = Wall(i, materials_data[material]["e_r"], materials_data[material]["sigma"], pulsation,
            temp_vector, materials_data[material]["depth"], material + 1);
    }

    std::vector<Ray> rays_rx;
    Ray* ray_list = new Ray[10];
    int ray_count = 0;

    compute_all_rays_hitting_tx_fields(2, walls, walls_data.size(), tx, rx, ray_list, ray_count);

    for (int i = 0; i < ray_count; i++) rays_rx.push_back(ray_list[i]);

    std::vector<Ray> only_direct;
    only_direct.push_back(rays_rx[0]);

    float received_power_direct = 10 * log(1000.f * compute_energy_fields(only_direct, pulsation, 73.f)) / log(10.f);
    // -64.77 dBm
    std::vector<Ray> direct_and_1;
    direct_and_1.push_back(rays_rx[0]);
    direct_and_1.push_back(rays_rx[3]);

    float received_power_direct_and_one = 10 * log(1000.f * compute_energy_fields(direct_and_1, pulsation, 73.f)) / log(10.f);
    // -64.60 dBm
    float received_power_total = 10 * log(1000.f * compute_energy_fields(rays_rx, pulsation, 73.f)) / log(10.f);
    // -64.87 dBm
    std::cout << "Received power : " << received_power_total << std::endl;
}
