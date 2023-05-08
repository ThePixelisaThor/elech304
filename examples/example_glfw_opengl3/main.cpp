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
#include "constants.h"
#include "main_utilities.h"

using json = nlohmann::json;
using namespace glm;

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

    GLFWwindow* mainWindow = NULL; ImGuiIO io; int bufferWidth; int bufferHeight;
    load_all(WIDTH, HEIGHT, mainWindow, bufferWidth, bufferHeight, io);

    glfwSetScrollCallback(mainWindow, scroll_callback);

    create_shader(shader, shader_texture);

    // loading json
    std::ifstream file_plan("plan.json");
    json walls_data = json::parse(file_plan);
    std::ifstream file_material("materials.json");
    json materials_data = json::parse(file_material);

    std::vector<FancyVector> walls;

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

    bool render_again = true; // temporary fix that became permanent
    bool d3 = false;
    bool show_circles = false;
    // lines
    
    unsigned int VBO_lines[line_count], VAO_lines[line_count], CBO_lines[line_count];
    create_arrays(VBO_lines, VAO_lines, CBO_lines, line_count);

    unsigned int VBO_lines_r[line_count], VAO_lines_r[line_count], CBO_lines_r[line_count];
    create_arrays(VBO_lines_r, VAO_lines_r, CBO_lines_r, line_count);

    unsigned int VBO_lines_rr[line_count], VAO_lines_rr[line_count], CBO_lines_rr[line_count];
    create_arrays(VBO_lines_rr, VAO_lines_rr, CBO_lines_rr, line_count);

    unsigned int* VBO_img = new unsigned int[10000];
    unsigned int* VAO_img = new unsigned int[10000];
    unsigned int* CBO_img = new unsigned int[10000];

    create_arrays(VBO_img, VAO_img, CBO_img, 10000);

    unsigned int* ray_cbo_buffer = new unsigned int[500];
    unsigned int* ray_vbo_buffer = new unsigned int[500];
    unsigned int* ray_vao_buffer = new unsigned int[500];

    create_arrays(ray_vbo_buffer, ray_vao_buffer, ray_cbo_buffer, 500);

    // init walls
    FancyVector wall;
    unsigned int VBO_walls[100], VAO_walls[100], CBO_walls[100]; // only for walls
    create_arrays(VBO_walls, VAO_walls, CBO_walls, 100);

    for (int i = 0; i < walls.size(); i++) {
        int material = walls_data[i]["material_id"];
        create_line(walls[i].a, walls[i].b, VBO_walls[i], VAO_walls[i], CBO_walls[i],
            Color {materials_data[material - 1]["color_r"], materials_data[material - 1]["color_g"] ,materials_data[material - 1]["color_b"] });
    }

    std::vector<Wall> walls_obj;
    for (int i = 0; i < walls.size(); i++) {
        int material = walls_data[i]["material_id"] - 1;
        walls_obj.push_back(Wall(i, materials_data[material]["e_r"], materials_data[material]["sigma"], 26.f * pow(10, 9) * 2.f * PI,
                        walls[i], materials_data[material]["depth"], material + 1, materials_data[material]["height"])); // j'ai pas du tout galéré psq j'ai oublié le + 1
    }

    // 3D walls
    unsigned int* VBO_rect = new unsigned int[600];
    unsigned int* VAO_rect = new unsigned int[600];
    unsigned int* EBO_rect = new unsigned int[600];

    glGenVertexArrays(600, VAO_rect);
    glGenBuffers(600, VBO_rect);
    glGenBuffers(600, EBO_rect);

    glUseProgram(shader_texture);
    for (int i = 0; i < walls.size(); i++) {
        vec2 normalized = 10.f * glm::normalize(walls_obj[i].fancy_vector.n);
        create_textured_cube(
            vec3(walls_obj[i].fancy_vector.a + normalized, walls_obj[i].height),
            vec3(walls_obj[i].fancy_vector.a - normalized, walls_obj[i].height),
            vec3(walls_obj[i].fancy_vector.b - normalized, walls_obj[i].height),
            vec3(walls_obj[i].fancy_vector.b + normalized, walls_obj[i].height),
            vec3(walls_obj[i].fancy_vector.a + normalized, 0.f),
            vec3(walls_obj[i].fancy_vector.a - normalized, 0.f),
            vec3(walls_obj[i].fancy_vector.b - normalized, 0.f),
            vec3(walls_obj[i].fancy_vector.b + normalized, 0.f),
            &VBO_rect[6*i], &VAO_rect[6*i], &EBO_rect[6*i], WHITE);
    }

    unsigned int brick_texture, stone_texture, wood_texture;
    load_texture(brick_texture, false, "bricks.jpg");
    load_texture(stone_texture, false, "stone.jpg");
    load_texture(wood_texture, false, "wood.jpg");

    glUniform1i(glGetUniformLocation(shader_texture, "ourTexture"), 0);

    glUseProgram(shader);

    // generate local zones
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

    float* energy_table_tx3 = new float[zone_count_x * zone_count_y];
    float* energy_table_tx1 = new float[zone_count_x * zone_count_y];
    float* energy_table_tx1_bis = new float[zone_count_x * zone_count_y];
    float* energy_table_tx2 = new float[zone_count_x * zone_count_y];

    int* ray_count_table = new int[zone_count_x * zone_count_y];

    double tic = glfwGetTime();

    run_algo_antenna(zone_count_x, zone_count_y, reflection_count, wall_array, walls_obj.size(), vec2(-100.f, 5.f), 26.f * pow(10, 9) * 2.f * PI, energy_table_tx3, ray_count_table, 3, -0.1f); // TX3
    // run_algo_antenna(zone_count_x, zone_count_y, reflection_count, wall_array, walls_obj.size(), vec2(450.f, -250.f), 26.f * pow(10, 9) * 2.f * PI, energy_table_tx1, ray_count_table, 1, 0.f); // TX1
    // run_algo_antenna(zone_count_x, zone_count_y, reflection_count, wall_array, walls_obj.size(), vec2(925.f, -276.795f), 26.f * pow(10, 9) * 2.f * PI, energy_table_tx1_bis, ray_count_table, 1, 0.f); // TX1 bis
    // run_algo_antenna(zone_count_x, zone_count_y, reflection_count, wall_array, walls_obj.size(), vec2(700.f, -500.f), 26.f * pow(10, 9) * 2.f * PI, energy_table_tx2, ray_count_table, 2, 0.1f); // TX2

    double toc = glfwGetTime();
    double delta = toc - tic;


    for (int y = 0; y < zone_count_y; y++) {
        for (int x = 0; x < zone_count_x; x++) {
             Color c_p = getGradientColor(energy_table_tx3[y * zone_count_x + x], min_energy, max_energy);
            /*
            Color c_p = getGradientColor(max(energy_table_tx3[y * zone_count_x + x], max(energy_table_tx1[y * zone_count_x + x],
                max(energy_table_tx1_bis[y * zone_count_x + x], energy_table_tx2[y * zone_count_x + x]))), min_energy, max_energy);
                */

            /*
            Color c_p = getGradientColor(max(energy_table_tx3[y * zone_count_x + x], max(energy_table_tx1[y * zone_count_x + x],
                energy_table_tx1_bis[y * zone_count_x + x])), min_energy, max_energy); */

            // Color c_p = getGradientColor(energy_table_tx1_bis[y * zone_count_x + x], min_energy, max_energy);
            Color c = getGradientColor(ray_count_table[y * zone_count_x + x], 0.f, 100.f);

            create_rectangle(vec2(x * 1000.f / zone_count_x, -y * 700.f / zone_count_y), vec2((x + 1) * 1000.f / zone_count_x, -y * 700.f / zone_count_y),
                vec2(x * 1000.f / zone_count_x, -(y + 1) * 700.f / zone_count_y), vec2((x + 1) * 1000.f / zone_count_x, -(y + 1) * 700.f / zone_count_y),
                VBO_zones_rect[x * zone_count_y + y], VAO_zones_rect[x * zone_count_y + y], CBO_zones_rect[x * zone_count_y + y], c);

            create_rectangle(vec2(x * 1000.f / zone_count_x, -y * 700.f / zone_count_y), vec2((x + 1) * 1000.f / zone_count_x, -y * 700.f / zone_count_y),
                vec2(x * 1000.f / zone_count_x, -(y + 1) * 700.f / zone_count_y), vec2((x + 1) * 1000.f / zone_count_x, -(y + 1) * 700.f / zone_count_y),
                VBO_zones_rect_p[x * zone_count_y + y], VAO_zones_rect_p[x * zone_count_y + y], CBO_zones_rect_p[x * zone_count_y + y], c_p);
        }
    }

    delete[] energy_table_tx3, ray_count_table, VAO_zones_rect, VAO_zones_rect_p;

    // Get a handle for our "MVP" uniform
    MatrixID = glGetUniformLocation(shader, "MVP");
    MatrixID2 = glGetUniformLocation(shader_texture, "MVP");

    bool draw_zone = false;
    bool ray_casting = false;
    bool ray_tracing_buffer = false;
    bool image_method = true;

    int ray_count = 0;
    int ray_rx_count = 0;
    int total_ray_count = 0;
    float circle_color[] = { 0.f, 0.f, 1.f };
    float buffer_circle_color[] = { 0.f, 0.f, 1.f };

    std::vector<Ray> all_rays;
    std::vector<int> seq;

    int show_vect = 0;
    bool power = false;
    float log_power = 0.f;

    glEnable(GL_DEPTH_TEST);

    GLFWcursorposfun previous_callback;

    while (!glfwWindowShouldClose(mainWindow)) // main loop
    {
        if (mouse_movement_buffer != mouse_movement) {
            glfwSetInputMode(mainWindow, GLFW_CURSOR, (mouse_movement_buffer) ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

            if (mouse_movement_buffer)
                previous_callback = glfwSetCursorPosCallback(mainWindow, mouse_callback);
            else
                glfwSetCursorPosCallback(mainWindow, previous_callback);

            mouse_movement = mouse_movement_buffer;
        }

        glfwPollEvents();
        processInput(mainWindow);

        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        glClearColor(0.1f, 0.1f, 0.1f, 0.1f);
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
        ImGui::Checkbox("Raycasting", &ray_casting);
        ImGui::Checkbox("Power", &power);
        ImGui::Checkbox("3D", &d3);
        ImGui::Checkbox("Show circles", &show_circles);
        ImGui::End();

        ImGui::Begin("Circle color");
        ImGui::ColorPicker3("##MyColor##5", (float*)&circle_color, ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha);
        ImGui::End(); 

        ImGui::Begin("Metrics");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::Text("Ray count : %d", ray_count);
        ImGui::Text("Ray count hitting rx : %d", ray_rx_count);
        ImGui::SeparatorText("PERFORMANCE");
        ImGui::Text("Time to run the algorithm (s) : %.3f", delta);
        ImGui::Text("Number of reflections : %d", reflection_count);
        ImGui::Text("Number of zones : %d", zone_count_x * zone_count_y);
        ImGui::Text("Zone of %.1fcm x %.1fcm", 10000.f / zone_count_x, 7000.f / zone_count_y);
        ImGui::End();

        ImGui::Begin("Legend");
        ImGui::Text("350          300         250         200          150         100         50 [Mb/s]");
        {
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImVec2 gradient_size(500, 35);
            // white "gradient"
            ImVec2 p0 = ImGui::GetCursorScreenPos();
            ImVec2 p1 = ImVec2(p0.x + 20.f, p0.y + gradient_size.y);

            // red green gradient
            ImVec2 p2 = ImVec2(p0.x + 20.f, p0.y);
            ImVec2 p3 = ImVec2(p0.x + gradient_size.x / 2.f + 20.f, p0.y + gradient_size.y);

            // green blue gradient
            ImVec2 p4 = ImVec2(p0.x + gradient_size.x / 2.f + 20.f, p0.y);
            ImVec2 p5 = ImVec2(p0.x + gradient_size.x + 20.f - 80.f, p0.y + gradient_size.y);

            // black "gradient"
            ImVec2 p6 = ImVec2(p0.x + gradient_size.x + 20.f - 80.f, p0.y);
            ImVec2 p7 = ImVec2(p0.x + gradient_size.x + 40.f, p0.y + gradient_size.y);

            ImU32 col_a = ImGui::GetColorU32(IM_COL32(255, 0, 0, 255));
            ImU32 col_b = ImGui::GetColorU32(IM_COL32(0, 255, 0, 255));
            ImU32 col_c = ImGui::GetColorU32(IM_COL32(0, 0, 255, 255));
            ImU32 col_d = ImGui::GetColorU32(IM_COL32(255, 255, 255, 255));
            ImU32 col_e = ImGui::GetColorU32(IM_COL32(50, 50, 50, 255));

            draw_list->AddRectFilledMultiColor(p0, p1, col_d, col_d, col_d, col_d);
            draw_list->AddRectFilledMultiColor(p2, p3, col_a, col_b, col_b, col_a);
            draw_list->AddRectFilledMultiColor(p4, p5, col_b, col_c, col_c, col_b);
            draw_list->AddRectFilledMultiColor(p6, p7, col_e, col_e, col_e, col_e);
            ImGui::InvisibleButton("##gradient", gradient_size);
        }
        ImGui::Text("   -60       -63         -66         -70          -73         -76         -80 [dBm]");
        ImGui::End();

        ImGui::Render(); // render GUI
        

        // OpenGL code goes here

        if (buffer_pos_tx_x != pos_x_tx || buffer_pos_tx_y != pos_y_tx ||
            buffer_pos_rx_x != pos_x_rx || buffer_pos_rx_y != pos_y_rx ||
            (buffer_circle_color[0] != circle_color[0]) || ray_tracing_buffer != ray_casting) {
            render_again = true;
            buffer_pos_tx_x = pos_x_tx; buffer_pos_tx_y = pos_y_tx;
            buffer_pos_rx_x = pos_x_rx; buffer_pos_rx_y = pos_y_rx;
            buffer_circle_color[0] = circle_color[0]; 
            buffer_circle_color[1] = circle_color[1]; 
            buffer_circle_color[2] = circle_color[2]; 
            ray_count = 0;
            ray_tracing_buffer = ray_casting;
        }

        glm::mat4 Projection = glm::perspective(glm::radians(camera.Zoom), (float)WIDTH / (float)HEIGHT, 0.1f, 1000.0f);
        glm::mat4 View = camera.GetViewMatrix();
        glm::mat4 Model = glm::mat4(1.0f);
        MVP = Projection * View * Model;

        if (d3) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            for (int i = 0; i < walls.size() * 6; i++)
                draw_textured_rectangle((walls_obj[i / 6].material_id == 1) ? stone_texture : ((walls_obj[i / 6].material_id == 2) ? brick_texture : wood_texture),
                    shader, shader_texture, VAO_rect[i], VBO_rect[i], EBO_rect[i]);
                //draw_textured_rectangle(brick_texture, shader, shader_texture, VAO_rect[i], VBO_rect[i], EBO_rect[i]);
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
        if (image_method && render_again) {
            compute_image_vectors(all_rays, center_tx, center_rx, walls_obj, ray_rx_count, total_ray_count, ray_cbo_buffer, ray_vbo_buffer, ray_vao_buffer);
        }

        // show vectors
        for (int i = max(0, show_vect - shown_rays_at_once); i < min(total_ray_count, show_vect); i++) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            draw_array(ray_vbo_buffer[i], ray_cbo_buffer[i]);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        if (ray_casting) {
            compute_draw_ray_cast(render_again, center_tx, VBO_lines, VAO_lines, CBO_lines, VBO_lines_r, VAO_lines_r, CBO_lines_r,
                VBO_lines_rr, VAO_lines_rr, CBO_lines_rr, walls, ray_count);
        }


        // drawing line walls
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        draw_arrays(VBO_walls, CBO_walls, walls.size());
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        
        // creating circle
        
        if (render_again) {
            compute_circle(center_tx, center_rx, VBO_circle_tx, VAO_circle_tx, CBO_circle_tx, VBO_circle_rx, VAO_circle_rx, CBO_circle_rx, circle_color);
        }

        // drawing circle
        if (show_circles) {
            draw_arrays(VBO_circle_tx, CBO_circle_tx, 721);
            draw_arrays(VBO_circle_rx, CBO_circle_rx, 721);
        }
        
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
