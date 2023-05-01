#include <iostream>
#include <string>
#include <math.h>

#include "../libs/GL/glew.h"
#include <GLFW\glfw3.h>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "../libs/nlohmann/json.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <wtypes.h>
#include <fstream>
#include "fancy_vector.h"
#include "vector_math.h"
#include "render_utils.h"
#include "shader_utils.h"
#include "color.h"
#include "raytracing.h"
#include "boilerplate.h"
#include "ex_8_1.h"
#include "ray.h"
#include "ray.h"
#include "ex_7_1.h"
#include "zone_tracing.h"

#include "texture_utils.h"
#include "camera.h"
#include "zone_tracing_cpu.h"
#include <CL/cl.h>
#include "opencl_tests.h"

using json = nlohmann::json;
using namespace glm;

const GLint WIDTH = 1920;
const GLint HEIGHT = 1080;

GLuint FBO;
GLuint RBO;
GLuint shader;
GLuint shader_texture;

glm::mat4 MVP = glm::mat4(1.0f);
GLuint MatrixID = 0;
GLuint MatrixID2 = 0;

void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

Camera camera(glm::vec3(0.0f, 0.0f, 2.0f));
float lastX = WIDTH / 2.0f;
float lastY = -HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;	// Time between current frame and last frame
float lastFrame = 0.0f; // Time of last frame

bool mouse_movement = false;
bool mouse_movement_buffer = false;
bool alt_key = false;

// int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) // link to windows => no terminal
int main(){
    // compute_everything();
    // compute_everything_8();
    // test_opencl();

    GLFWwindow* mainWindow = NULL; ImGuiIO io; int bufferWidth; int bufferHeight;
    load_all(WIDTH, HEIGHT, mainWindow, bufferWidth, bufferHeight, io);

    glfwSetScrollCallback(mainWindow, scroll_callback);
    // glfwSetKeyCallback(mainWindow, key_callback);

    create_shader(shader, shader_texture);

    // loading json
    std::ifstream file_plan("plan.json");
    json walls_data = json::parse(file_plan);
    std::ifstream file_material("materials.json");
    json materials_data = json::parse(file_material);

    std::vector<FancyVector> walls; // vector is a list with access by id

    // parse wall vectors
    for (int i = 0; i < walls_data.size(); i++) {
        vec2 a((float) walls_data[i]["point_ax"] * 10.f, (float) walls_data[i]["point_ay"] * 10.f);
        vec2 b((float) walls_data[i]["point_bx"] * 10.f, (float) walls_data[i]["point_by"] * 10.f);

        FancyVector temp_vector(a, b);
        walls.push_back(temp_vector);
    }

    unsigned int VBO_circle_tx[721], VAO_circle_tx[721], CBO_circle_tx[721];
    unsigned int VBO_circle_rx[721], VAO_circle_rx[721], CBO_circle_rx[721];
    create_arrays(VBO_circle_tx, VAO_circle_tx, CBO_circle_tx, 721);
    create_arrays(VBO_circle_rx, VAO_circle_rx, CBO_circle_rx, 721);
    
    float pos_x_tx = -10.f;
    float pos_y_tx = 0.5f;

    float pos_x_rx = 0.25f;
    float pos_y_rx = -0.25f;

    float buffer_pos_tx_x = 0;
    float buffer_pos_tx_y = 0;

    float buffer_pos_rx_x = 0;
    float buffer_pos_rx_y = 0;

    bool render_again = true; // temporary fix
    bool d3 = false;
    // lines
    const int line_count = 50;
    unsigned int VBO_lines[line_count], VAO_lines[line_count], CBO_lines[line_count];
    create_arrays(VBO_lines, VAO_lines, CBO_lines, line_count);

    unsigned int VBO_lines_r[line_count], VAO_lines_r[line_count], CBO_lines_r[line_count];
    create_arrays(VBO_lines_r, VAO_lines_r, CBO_lines_r, line_count);

    unsigned int VBO_lines_rr[line_count], VAO_lines_rr[line_count], CBO_lines_rr[line_count];
    create_arrays(VBO_lines_rr, VAO_lines_rr, CBO_lines_rr, line_count);

    unsigned int VBO_img[10000], VAO_img[10000], CBO_img[10000];
    create_arrays(VBO_img, VAO_img, CBO_img, 10000);

    // init walls
    FancyVector wall;
    unsigned int VBO_walls[100], VAO_walls[100], CBO_walls[100]; // only for walls
    create_arrays(VBO_walls, VAO_walls, CBO_walls, 100);

    for (int i = 0; i < walls.size(); i++) {
        int material = walls_data[i]["material_id"];
        create_line(walls[i].a, walls[i].b, VBO_walls[i], VAO_walls[i], CBO_walls[i],
            Color {materials_data[material - 1]["color_r"], materials_data[material - 1]["color_g"] ,materials_data[material - 1]["color_b"] });
    }

    // Wall(float _relative_perm, float _conductivity, float _pulsation, FancyVector _fancy_vector, float _depth);
    std::vector<Wall> walls_obj;
    for (int i = 0; i < walls.size(); i++) {
        int material = walls_data[i]["material_id"] - 1;
        walls_obj.push_back(Wall(i, materials_data[material]["e_r"], materials_data[material]["sigma"], 26.f * pow(10, 9) * 2.f * 3.14159265f,
                        walls[i], materials_data[material]["depth"], material));
    }

    // 3D walls
    unsigned int VBO_rect[600], VAO_rect[600], EBO_rect[600];
    glGenVertexArrays(600, VAO_rect);
    glGenBuffers(600, VBO_rect);
    glGenBuffers(600, EBO_rect);

    // bricks
    glUseProgram(shader_texture);
    float height = 50.f; // 5 meters
    for (int i = 0; i < walls.size(); i++) {
        if (walls_obj[i].material_id != 2) continue;
        vec2 normalized = 10.f * glm::normalize(walls_obj[i].fancy_vector.n);
        create_textured_cube(
            vec3(walls_obj[i].fancy_vector.a + normalized, height),
            vec3(walls_obj[i].fancy_vector.a - normalized, height),
            vec3(walls_obj[i].fancy_vector.b - normalized, height),
            vec3(walls_obj[i].fancy_vector.b + normalized, height),
            vec3(walls_obj[i].fancy_vector.a + normalized, 0.f),
            vec3(walls_obj[i].fancy_vector.a - normalized, 0.f),
            vec3(walls_obj[i].fancy_vector.b - normalized, 0.f),
            vec3(walls_obj[i].fancy_vector.b + normalized, 0.f),
            &VBO_rect[6*i], &VAO_rect[6*i], &EBO_rect[6*i], WHITE);

    }

    unsigned int brick_texture;
    load_texture(brick_texture, false, "wood.jpg");

    glUniform1i(glGetUniformLocation(shader_texture, "ourTexture"), 0);

    // stone

    glUseProgram(shader_texture);
    height = 50.f; // 5 meters
    for (int i = 0; i < walls.size(); i++) {
        if (walls_obj[i].material_id != 1) continue;
        vec2 normalized = 10.f * glm::normalize(walls_obj[i].fancy_vector.n);
        create_textured_cube(
            vec3(walls_obj[i].fancy_vector.a + normalized, height),
            vec3(walls_obj[i].fancy_vector.a - normalized, height),
            vec3(walls_obj[i].fancy_vector.b - normalized, height),
            vec3(walls_obj[i].fancy_vector.b + normalized, height),
            vec3(walls_obj[i].fancy_vector.a + normalized, 0.f),
            vec3(walls_obj[i].fancy_vector.a - normalized, 0.f),
            vec3(walls_obj[i].fancy_vector.b - normalized, 0.f),
            vec3(walls_obj[i].fancy_vector.b + normalized, 0.f),
            &VBO_rect[6 * i], &VAO_rect[6 * i], &EBO_rect[6 * i], WHITE);

    }

    unsigned int stone_texture;
    load_texture(stone_texture, false, "wood.jpg");

    glUniform1i(glGetUniformLocation(shader_texture, "ourTexture"), 0);

    // wood
    glUseProgram(shader_texture);
    height = 25.f;
    for (int i = 0; i < walls.size(); i++) {
        if (walls_obj[i].material_id != 3 && walls_obj[i].material_id != 4) continue;
        vec2 normalized = 10.f * glm::normalize(walls_obj[i].fancy_vector.n);
        create_textured_cube(
            vec3(walls_obj[i].fancy_vector.a + normalized, height),
            vec3(walls_obj[i].fancy_vector.a - normalized, height),
            vec3(walls_obj[i].fancy_vector.b - normalized, height),
            vec3(walls_obj[i].fancy_vector.b + normalized, height),
            vec3(walls_obj[i].fancy_vector.a + normalized, 0.f),
            vec3(walls_obj[i].fancy_vector.a - normalized, 0.f),
            vec3(walls_obj[i].fancy_vector.b - normalized, 0.f),
            vec3(walls_obj[i].fancy_vector.b + normalized, 0.f),
            &VBO_rect[6 * i], &VAO_rect[6 * i], &EBO_rect[6 * i], WHITE);

    }
    unsigned int wood_texture;
    load_texture(wood_texture, false, "wood.jpg");

    glUniform1i(glGetUniformLocation(shader_texture, "ourTexture"), 0);

    glUseProgram(shader);

    // generate local zones

    const int zone_count_x = 200; // *4
    const int zone_count_y = 140;

    GLuint* VBO_zones_rect = new GLuint[zone_count_x * zone_count_y];
    GLuint* VAO_zones_rect = new GLuint[zone_count_x * zone_count_y];
    GLuint* CBO_zones_rect = new GLuint[zone_count_x * zone_count_y];
    create_arrays(VBO_zones_rect, VAO_zones_rect, CBO_zones_rect, zone_count_x * zone_count_y);

    GLuint* VBO_zones_rect_p = new GLuint[zone_count_x * zone_count_y];
    GLuint* VAO_zones_rect_p = new GLuint[zone_count_x * zone_count_y];
    GLuint* CBO_zones_rect_p = new GLuint[zone_count_x * zone_count_y];
    create_arrays(VBO_zones_rect_p, VAO_zones_rect_p, CBO_zones_rect_p, zone_count_x * zone_count_y);

    vec2 tx_zone(pos_x_tx * 10.f, pos_y_tx * 10.f);
    vec2 rx_zone(pos_x_rx * 10.f, pos_y_rx * 10.f);

    Wall *wall_array = new Wall[walls_obj.size()];

    for (int i = 0; i < walls_obj.size(); i++) {
        wall_array[i] = walls_obj[i]; // walls_obj will be replaced later
    }

    // run_algo(zone_count_x, zone_count_y, 4, wall_array, walls_obj.size(), tx_zone);

    float* energy_table_tx3 = new float[zone_count_x * zone_count_y];
    float* energy_table_tx1 = new float[zone_count_x * zone_count_y];
    float* energy_table_tx1_bis = new float[zone_count_x * zone_count_y];
    float* energy_table_tx2 = new float[zone_count_x * zone_count_y];

    int* ray_count_table = new int[zone_count_x * zone_count_y];
    int reflection_count = 3;

    double tic = glfwGetTime();

    // run_final_algo(zone_count_x, zone_count_y, reflection_count, wall_array, walls_obj.size(), tx_zone, 26.f * pow(10, 9) * 2.f * 3.141592f, energy_table, ray_count_table);
    run_algo_antenna(zone_count_x, zone_count_y, reflection_count, wall_array, walls_obj.size(), vec2(-100.f, 5.f), 26.f * pow(10, 9) * 2.f * 3.141592f, energy_table_tx3, ray_count_table, 3, 0.1f); // TX3
    run_algo_antenna(zone_count_x, zone_count_y, reflection_count, wall_array, walls_obj.size(), vec2(450.f, -250.f), 26.f * pow(10, 9) * 2.f * 3.141592f, energy_table_tx1, ray_count_table, 1, 0.1f); // TX1
    run_algo_antenna(zone_count_x, zone_count_y, reflection_count, wall_array, walls_obj.size(), vec2(925.f, -276.795f), 26.f * pow(10, 9) * 2.f * 3.141592f, energy_table_tx1_bis, ray_count_table, 1, 0.1f); // TX1 bis
    run_algo_antenna(zone_count_x, zone_count_y, reflection_count, wall_array, walls_obj.size(), vec2(700.f, -500.f), 26.f * pow(10, 9) * 2.f * 3.141592f, energy_table_tx2, ray_count_table, 2, 0.1f); // TX2

    double toc = glfwGetTime();
    double delta = toc - tic;

    float min_energy = -80.f, max_energy = -30.f;

    /*
    for (int i = 0; i < zone_count_x * zone_count_y; i++) {
        // à réécrire psq max prends que 2 args
        if (min_energy > min(energy_table_tx3[i], energy_table_tx1[i], energy_table_tx1_bis[i], energy_table_tx2[i]))
            min_energy = min(energy_table_tx3[i], energy_table_tx1[i], energy_table_tx1_bis[i], energy_table_tx2[i]);
        if (max_energy < max(energy_table_tx3[i], energy_table_tx1[i], energy_table_tx1_bis[i], energy_table_tx2[i]))
            max_energy = max(energy_table_tx3[i], energy_table_tx1[i], energy_table_tx1_bis[i], energy_table_tx2[i]);
    } */

    printf("Min energy %.3f, max energy %.3f \n", min_energy, max_energy);

    for (int y = 0; y < zone_count_y; y++) {
        for (int x = 0; x < zone_count_x; x++) {
            // Color c_p = getGradientColor(energy_table[y * zone_count_x + x], -21.7f, -12.6f);

            Color c_p = getGradientColor(max(energy_table_tx3[y * zone_count_x + x], max(energy_table_tx1[y * zone_count_x + x],
                max(energy_table_tx1_bis[y * zone_count_x + x], energy_table_tx2[y * zone_count_x + x]))), min_energy, max_energy);

            if (y == 0 && x == 8) {
                printf("Color %.3f - %.3f - %.3f \n", c_p.r, c_p.g, c_p.b);
                printf("TX3 %.3f, TX1 %.3f, TX1_bis %.3f, TX2 %.3f\n", energy_table_tx3[8], energy_table_tx1[8],
                    energy_table_tx1_bis[8], energy_table_tx2[8]);
            }

            // Color c_p = getGradientColor(energy_table_tx1_bis[y * zone_count_x + x], min_energy, max_energy);
            Color c = getGradientColor(ray_count_table[y * zone_count_x + x], 0.f, 50.f);

            create_rectangle(vec2(x * 1000.f / zone_count_x, -y * 700.f / zone_count_y), vec2((x + 1) * 1000.f / zone_count_x, -y * 700.f / zone_count_y),
                vec2(x * 1000.f / zone_count_x, -(y + 1) * 700.f / zone_count_y), vec2((x + 1) * 1000.f / zone_count_x, -(y + 1) * 700.f / zone_count_y),
                VBO_zones_rect[x * zone_count_y + y], VAO_zones_rect[x * zone_count_y + y], CBO_zones_rect[x * zone_count_y + y], c);

            create_rectangle(vec2(x * 1000.f / zone_count_x, -y * 700.f / zone_count_y), vec2((x + 1) * 1000.f / zone_count_x, -y * 700.f / zone_count_y),
                vec2(x * 1000.f / zone_count_x, -(y + 1) * 700.f / zone_count_y), vec2((x + 1) * 1000.f / zone_count_x, -(y + 1) * 700.f / zone_count_y),
                VBO_zones_rect_p[x * zone_count_y + y], VAO_zones_rect_p[x * zone_count_y + y], CBO_zones_rect_p[x * zone_count_y + y], c_p);
        }
    }

    delete[] energy_table_tx3, ray_count_table, VAO_zones_rect, VAO_zones_rect_p;

    /*

    double tic = glfwGetTime();

    // creates all necessary arrays
    int reflection_count = 2;
    int wall_count = walls_obj.size();
    int max_count = 1 + (int)pow(wall_count, reflection_count); // should equal mirror_count
    vec2* all_tx_mirrors = new vec2[max_count]; // il y a moyen de calculer tous les tx mirrors, faire le chemin à l'envers mais c'est chiant je pense (j'ai rien dit je l'ai fait)
    int* each_reflection_count = new int[max_count];
    int* walls_to_reflect = new int[max_count * reflection_count]; // represents a 2d array [[r1, r2, ...], [r1, r2, ...], ...]

    int mirror_count = 0;

    generate_all_mirrors(reflection_count, all_tx_mirrors, each_reflection_count, walls_to_reflect,
        wall_array, wall_count, mirror_count, tx_zone, max_count);

    bool hit;
    int ray_count_zone = 0;
    vec2 rx_zone_test;
    Ray rays_test[100]; // max 100 vectors, not safe
    int ray_count_test = 0;

    for (int x = 0; x < zone_count_x; x++) {
        for (int y = 0; y < zone_count_y; y++) {
            // les problèmes
            rx_zone_test.x = (x + 0.5f) * 1000.f / zone_count_x; // pour éviter d'instancier un vec2
            rx_zone_test.y = -(y + 0.5) * 700.f / zone_count_y;
            ray_count_test = 0;

            for (int m = 0; m < mirror_count; m++) {
                compute_ray_cpu(tx_zone, rx_zone_test, all_tx_mirrors[m] - rx_zone_test, rays_test[ray_count_test], hit, wall_array, wall_count, &walls_to_reflect[m * reflection_count],
                    1.0f, 0.f, each_reflection_count[m]);

                if (hit)
                    ray_count_test++;
            }

            // compute_all_rays_hitting_tx_cpu(2, wall_array, walls_obj.size(), tx_zone, rx_zone_test, rays_test, ray_count_test); // ça recalcule toutes les images

            float power = compute_energy_cpu(rays_test, ray_count_test);
            //float power = compute_energy(rays__); // in db
            float log_power = log(power) / log(10); // c'est en base 2

            // Color c{(12.f + log_power) / 4.f, (12.f + log_power) / 4.f, 0.f};

            // if (c.r < 0.) c.r = 0.f;
            // if (c.g < 0.) c.g = 0.f;

            Color c_p = getGradientColor(log_power, -21.7f, -12.6f);
            Color c = getGradientColor(ray_count_test, 0.f, 50.f);
            // Color c{ rays__.size() / 20.f, rays__.size() / 20.f, 0.f };

            create_rectangle(vec2(x * 1000.f / zone_count_x, -y * 700.f / zone_count_y), vec2((x + 1) * 1000.f / zone_count_x, -y * 700.f / zone_count_y),
                vec2(x * 1000.f / zone_count_x, -(y + 1) * 700.f / zone_count_y), vec2((x + 1) * 1000.f / zone_count_x, -(y + 1) * 700.f / zone_count_y),
                VBO_zones_rect[x * zone_count_y + y], VAO_zones_rect[x * zone_count_y + y], CBO_zones_rect[x * zone_count_y + y], c);

            create_rectangle(vec2(x * 1000.f / zone_count_x, -y * 700.f / zone_count_y), vec2((x + 1) * 1000.f / zone_count_x, -y * 700.f / zone_count_y),
                vec2(x * 1000.f / zone_count_x, -(y + 1) * 700.f / zone_count_y), vec2((x + 1) * 1000.f / zone_count_x, -(y + 1) * 700.f / zone_count_y),
                VBO_zones_rect_p[x * zone_count_y + y], VAO_zones_rect_p[x * zone_count_y + y], CBO_zones_rect_p[x * zone_count_y + y], c_p);

        }
    }

    // évidemment on oublie pas de free la stack
    delete[] all_tx_mirrors;
    delete[] each_reflection_count;
    delete[] walls_to_reflect;

    double toc = glfwGetTime();
    double delta = toc - tic;

    */

    // Get a handle for our "MVP" uniform
    MatrixID = glGetUniformLocation(shader, "MVP");
    MatrixID2 = glGetUniformLocation(shader_texture, "MVP");

    bool draw_zone = false;
    bool ray_tracing = false;
    bool ray_tracing_buffer = false;
    bool image_method = true;

    int ray_count = 0;
    int ray_rx_count = 0;
    float circle_color[] = { 0.f, 0.f, 1.f };
    float buffer_circle_color[] = { 0.f, 0.f, 1.f };

    std::vector<Ray> all_rays;
    std::vector<int> seq;
    std::vector<unsigned int> ray_cbo_buffer;
    std::vector<unsigned int> ray_vbo_buffer;

    int show_vect = 0;
    bool power = false;
    float log_power = 0.f;

    glEnable(GL_DEPTH_TEST);

    GLFWcursorposfun previous_callback;

    while (!glfwWindowShouldClose(mainWindow)) // main loop
    {
        if (mouse_movement_buffer != mouse_movement) {
            glfwSetInputMode(mainWindow, GLFW_CURSOR, (mouse_movement_buffer) ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
            if (mouse_movement_buffer) {
                previous_callback = glfwSetCursorPosCallback(mainWindow, mouse_callback);
            }
            else {
                glfwSetCursorPosCallback(mainWindow, previous_callback);
            }
            mouse_movement = mouse_movement_buffer;
        }

        glfwPollEvents();
        processInput(mainWindow);

        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        glClearColor(0.2f, 0.2f, 0.2f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shader);
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
        glUseProgram(shader_texture);
        glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
        glUseProgram(shader);
        ImGui::NewFrame();

        // GUI code goes here

        ImGui::Begin("Settings");
        ImGui::SeparatorText("TX");
        ImGui::DragFloat("Value pos_x_tx", &pos_x_tx);
        ImGui::DragFloat("Value pos_y_tx", &pos_y_tx);
        ImGui::SeparatorText("RX");
        ImGui::DragFloat("Value pos_x_rx", &pos_x_rx);
        ImGui::DragFloat("Value pos_y_rx", &pos_y_rx);
        ImGui::DragInt("Show vectors", &show_vect);
        ImGui::Checkbox("Draw zones", &draw_zone);
        ImGui::Checkbox("Raytracing", &ray_tracing);
        ImGui::Checkbox("Power", &power);
        ImGui::Checkbox("3D", &d3);
        ImGui::End();

        ImGui::Begin("Circle color");
        ImGui::ColorPicker3("##MyColor##5", (float*)&circle_color, ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha);
        ImGui::End(); 

        ImGui::Begin("Metrics");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::Text("Ray count : %d", ray_count);
        ImGui::Text("Ray count hitting rx : %d", ray_rx_count);
        ImGui::Text("Received power : %.3f", log_power);
        ImGui::SeparatorText("PERFORMANCE");
        ImGui::Text("Time to run the algorithm (s) : %.3f", delta);
        ImGui::Text("Number of reflections : %d", reflection_count);
        ImGui::Text("Number of zones : %d", zone_count_x * zone_count_y);
        ImGui::Text("Zone of %.1fcm x %.1fcm", 10000.f / zone_count_x, 7000.f / zone_count_y);
        ImGui::End();

        ImGui::Render(); // render GUI
        

        // OpenGL code goes here

        if (buffer_pos_tx_x != pos_x_tx || buffer_pos_tx_y != pos_y_tx ||
            buffer_pos_rx_x != pos_x_rx || buffer_pos_rx_y != pos_y_rx ||
            (buffer_circle_color[0] != circle_color[0]) || ray_tracing_buffer != ray_tracing) {
            render_again = true;
            buffer_pos_tx_x = pos_x_tx; buffer_pos_tx_y = pos_y_tx;
            buffer_pos_rx_x = pos_x_rx; buffer_pos_rx_y = pos_y_rx;
            buffer_circle_color[0] = circle_color[0]; 
            buffer_circle_color[1] = circle_color[1]; 
            buffer_circle_color[2] = circle_color[2]; 
            ray_count = 0;
            ray_tracing_buffer = ray_tracing;
        }

        // pass projection matrix to shader (note that in this case it could change every frame)
        glm::mat4 Projection = glm::perspective(glm::radians(camera.Zoom), (float)WIDTH / (float)HEIGHT, 0.1f, 1000.0f);

        // camera/view transformation
        glm::mat4 View = camera.GetViewMatrix();


        glm::mat4 Model = glm::mat4(1.0f);
        MVP = Projection * View * Model;

        if (d3) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            for (int i = 0; i < walls.size() * 6; i++)
                draw_textured_rectangle(brick_texture, shader, shader_texture, VAO_rect[i], VBO_rect[i], EBO_rect[i]);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        vec2 center_tx(pos_x_tx * 10.f, pos_y_tx * 10.f);
        vec2 center_rx(pos_x_rx * 10.f, pos_y_rx * 10.f);

        
        if (draw_zone) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            if (!power)
                draw_arrays_rect(VBO_zones_rect, CBO_zones_rect, zone_count_x * zone_count_y);
            else
                draw_arrays_rect(VBO_zones_rect_p, CBO_zones_rect_p, zone_count_x * zone_count_y);
        }

        // rays
        
       
        int maxRef = 1;
        if (image_method && render_again)
        {
            all_rays = {};
            // trajet direct
            std::vector<Ray> _buffer;

            compute_ray(center_tx, center_rx, center_rx - center_tx, center_tx, all_rays, _buffer, walls_obj, std::vector<int>(), 1.0f, 0, 0, 0);
            std::vector<vec2> only_rx;
            only_rx.push_back(center_rx);
            generate_rays_direction(center_tx, center_rx, center_rx, walls_obj, -1, all_rays, std::vector<int>(), only_rx, maxRef, 0);

            std::vector<Ray> rays_rx;

            generate_ray_hitting_rx(center_tx, center_rx, walls_obj, rays_rx, maxRef);
            ray_rx_count = rays_rx.size();

            float power = compute_energy(rays_rx); // in db
            log_power = log(power) / log(10); // c'est en base 2

            // all_rays = rays_rx;

            ray_cbo_buffer = {};
            ray_vbo_buffer = {};

            for (Ray r : all_rays) {
                unsigned int VBO[3];
                unsigned int CBO[3];
                unsigned int VAO[3];
                create_arrays(VBO, VAO, CBO, 3);
                r.create(*VBO, *VAO, *CBO);
                ray_cbo_buffer.push_back(*CBO);
                ray_vbo_buffer.push_back(*VBO);
            }
        }
        
        for (int i = max(0, show_vect - 20); i < min(ray_cbo_buffer.size(), show_vect); i++) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            draw_array(ray_vbo_buffer[i], ray_cbo_buffer[i]);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        } 
        // for (Ray r : all_rays) r.create_draw(); // c'est de la merde ça memory leak
        ray_count = ray_vbo_buffer.size();
        if (ray_tracing) {

            for (int i = 0; i < line_count; i++) {

                float dx = cos(i * 2.f / line_count * 3.1415f); // on pourra opti ça
                float dy = sin(i * 2.f / line_count * 3.1415f);
                vec2 direction(dx, dy);

                float t = 0;
                float s = 0;
                bool found = true;

                if (render_again) {
                    // calculating intersection
                    vec2 best_intersection = getIntersection(direction, center_tx, walls, wall, found);

                    if (!found) { // obligé psq on continue de render les lignes, donc il faut override
                        create_line(center_tx, center_tx, VBO_lines[i], VAO_lines[i], CBO_lines[i], GREEN);
                        create_line(center_tx, center_tx, VBO_lines_r[i], VAO_lines_r[i], CBO_lines_r[i], GREEN);
                        create_line(center_tx, center_tx, VBO_lines_rr[i], VAO_lines_rr[i], CBO_lines_rr[i], GREEN);
                        continue;
                    }

                    create_line(center_tx, best_intersection, VBO_lines[i], VAO_lines[i], CBO_lines[i], GREEN);
                    ray_count++;
                    // reflections

                    vec2 reflection_vector = direction - 2.f * dot(direction, normalize(vec2(wall.u.y, -wall.u.x))) * normalize(vec2(wall.u.y, -wall.u.x));

                    vec2 best_intersection_r = getIntersection(reflection_vector, best_intersection, walls, wall, found);

                    if (!found) {
                        create_line(center_tx, center_tx, VBO_lines_r[i], VAO_lines_r[i], CBO_lines_r[i], GREEN);
                        create_line(center_tx, center_tx, VBO_lines_rr[i], VAO_lines_rr[i], CBO_lines_rr[i], GREEN);
                        continue;
                    }

                    create_line(best_intersection, best_intersection_r, VBO_lines_r[i], VAO_lines_r[i], CBO_lines_r[i], YELLOW);
                    ray_count++;
                    vec2 reflection_vector_2 = reflection_vector - 2.f * dot(reflection_vector, normalize(vec2(wall.u.y, -wall.u.x))) * normalize(vec2(wall.u.y, -wall.u.x));

                    vec2 best_intersection_r2 = getIntersection(reflection_vector_2, best_intersection_r, walls, wall, found);

                    if (!found) {
                        create_line(center_tx, center_tx, VBO_lines_rr[i], VAO_lines_rr[i], CBO_lines_rr[i], GREEN);
                        continue;
                    }

                    create_line(best_intersection_r, best_intersection_r2, VBO_lines_rr[i], VAO_lines_rr[i], CBO_lines_rr[i], RED);
                    ray_count++;
                }
            }

            // drawing rays
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            draw_arrays(VBO_lines, CBO_lines, line_count);
            draw_arrays(VBO_lines_r, CBO_lines_r, line_count);
            draw_arrays(VBO_lines_rr, CBO_lines_rr, line_count);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
        

        // drawing walls
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        draw_arrays(VBO_walls, CBO_walls, walls.size());
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        
        // creating circle
        float radius = 7.f;

        if (render_again) { // pas la méthode la plus opti pour dessiner un cercle mais soit (plus de triangles au centre qu'aux extrémités)
            float x, y;
            for (double i = 0; i <= 360;) {
                x = radius * cos(i * 3.141592f / 180.f) + pos_x_tx * 10.f;
                y = radius * sin(i * 3.141592f / 180.f) + pos_y_tx * 10.f;
                vec2 a(x, y);
                i = i + 0.5;
                x = radius * cos(i * 3.141592f / 180.f) + pos_x_tx * 10.f;
                y = radius * sin(i * 3.141592f / 180.f) + pos_y_tx * 10.f;
                vec2 b(x, y);

                int j = (int)2 * (i-0.5);
                create_triangle(a, center_tx, b, VBO_circle_tx[j], VAO_circle_tx[j], CBO_circle_tx[j], Color{ circle_color[0], circle_color[1], circle_color[2] });
            }

            for (double i = 0; i <= 360;) {
                x = radius * cos(i * 3.141592f / 180.f) + pos_x_rx * 10.f;
                y = radius * sin(i * 3.141592f / 180.f) + pos_y_rx * 10.f;
                vec2 a(x, y);
                i = i + 0.5;
                x = radius * cos(i * 3.141592f / 180.f) + pos_x_rx * 10.f;
                y = radius * sin(i * 3.141592f / 180.f) + pos_y_rx * 10.f;
                vec2 b(x, y);

                int j = (int)2 * (i - 0.5);
                create_triangle(a, center_rx, b, VBO_circle_rx[j], VAO_circle_rx[j], CBO_circle_rx[j], Color{ circle_color[0], circle_color[1], circle_color[2] });
            }

        }

        // drawing circle
        draw_arrays(VBO_circle_tx, CBO_circle_tx, 721);
        draw_arrays(VBO_circle_rx, CBO_circle_rx, 721);
        
        
        render_again = false;

        render_end(mainWindow, io);
    }

    terminate(mainWindow);

    return 0;
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS && !alt_key) {
        alt_key = true;
        mouse_movement_buffer = !mouse_movement_buffer;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_RELEASE && alt_key) {
        alt_key = false;
    }
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    if (!mouse_movement)
        return;

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
