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

using json = nlohmann::json;
using namespace glm;

const GLint WIDTH = 1920;
const GLint HEIGHT = 1080;

GLuint FBO;
GLuint RBO;
GLuint texture_id;
GLuint shader;

glm::mat4 MVP = glm::mat4(1.0f);
GLuint MatrixID = 0;

const char* fragment_shader_code = R"*(
#version 330
in vec3 fragmentColor;
out vec3 color;
void main()
{
	// color = vec3(1.f, 1.f, 1.f);
    color = fragmentColor;
}
)*";

void create_shaders()
{
    create_shader(shader, fragment_shader_code);
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) // link to windows => no terminal
{
    if (!glfwInit()) {
        std::cout << "GLFW initialisation failed!\n";
        glfwTerminate();
        return 1;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    GLFWwindow* mainWindow = glfwCreateWindow(WIDTH, HEIGHT, "Projet ELEC-H304", NULL, NULL);
    if (!mainWindow)
    {
        std::cout << "GLFW creation failed!\n";
        glfwTerminate();
        return 1;
    }

    int bufferWidth, bufferHeight;
    glfwGetFramebufferSize(mainWindow, &bufferWidth, &bufferHeight);
    glfwMakeContextCurrent(mainWindow);
    glewExperimental = GL_TRUE;

    if (glewInit() != GLEW_OK) {
        std::cout << "glew initialisation failed!\n";
        glfwDestroyWindow(mainWindow);
        glfwTerminate();
        return 1;
    }

    glViewport(0, 0, bufferWidth, bufferHeight);


    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplGlfw_InitForOpenGL(mainWindow, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    create_shaders();

    // loading json
    std::ifstream f("plan.json");
    json data = json::parse(f);

    std::vector<FancyVector> walls; // vector is a list with access by id

    // parse wall vectors
    for (int i = 0; i < data.size(); i++) {
        vec2 a(data[i]["point_ax"] * 10.f, data[i]["point_ay"] * 10.f);
        vec2 b(data[i]["point_bx"] * 10.f, data[i]["point_by"] * 10.f);

        FancyVector temp_vector(a, b);
        walls.push_back(temp_vector);
    }

    unsigned int VBO_circle[721], VAO_circle[721], CBO_circle[721]; // only for zones
    glGenVertexArrays(721, VAO_circle); // we can generate multiple VAOs or buffers at the same time
    glGenBuffers(721, VBO_circle);
    glGenBuffers(721, CBO_circle);

    unsigned int VBO_zones[342], VAO_zones[342], CBO_zones[342]; // only for zones
    glGenVertexArrays(342, VAO_zones); // we can generate multiple VAOs or buffers at the same time
    glGenBuffers(342, VBO_zones);
    glGenBuffers(342, CBO_zones);
    // generate local zones

    for (int j = 0; j <= 140; j++) {
        create_line(vec2(0.0, -j * 0.5 * 10.f), vec2(100.0 * 10.f, -j * 0.5 * 10.f), VBO_zones[j], VAO_zones[j], CBO_zones[j], YELLOW);
    }
    for (int i = 0; i <= 200; i++) {
        create_line(vec2(i * 0.5 * 10.f, 0.0), vec2(i * 0.5 * 10.f, -70.0 * 10.f), VBO_zones[141 + i], VAO_zones[141 + i], CBO_zones[141 + i], YELLOW);
    }

    float pos_x = 56.f;
    float pos_y = -56.f;

    float buffer_pos_x = 0;
    float buffer_pos_y = 0;

    int material = 0;
    bool render_again = true; // temporary fix

    // lines
    const int line_count = 50;
    unsigned int VBO_lines[line_count], VAO_lines[line_count], CBO_lines[line_count];
    glGenVertexArrays(line_count, VAO_lines);
    glGenBuffers(line_count, VBO_lines);
    glGenBuffers(line_count, CBO_lines);

    unsigned int VBO_lines_r[line_count], VAO_lines_r[line_count], CBO_lines_r[line_count];
    glGenVertexArrays(line_count, VAO_lines_r);
    glGenBuffers(line_count, VBO_lines_r);
    glGenBuffers(line_count, CBO_lines_r);

    unsigned int VBO_lines_rr[line_count], VAO_lines_rr[line_count], CBO_lines_rr[line_count];
    glGenVertexArrays(line_count, VAO_lines_rr);
    glGenBuffers(line_count, VBO_lines_rr);
    glGenBuffers(line_count, CBO_lines_rr);

    // init walls
    FancyVector wall;
    unsigned int VBO_walls[100], VAO_walls[100], CBO_walls[100]; // only for walls
    glGenVertexArrays(100, VAO_walls); // we can generate multiple VAOs or buffers at the same time
    glGenBuffers(100, VBO_walls);
    glGenBuffers(100, CBO_walls);
    for (int i = 0; i < walls.size(); i++) {
        material = data[i]["material_id"];
        create_line(walls[i].a, walls[i].b, VBO_walls[i], VAO_walls[i], CBO_walls[i], material == 1 ? BLUE : material == 2 ? RED : WHITE);
    }

    // Get a handle for our "MVP" uniform
    MatrixID = glGetUniformLocation(shader, "MVP");

    float zoom_factor = 2.f;
    float cam_x = 0.4f;
    float cam_y = 0.f;
    float cam_z = 1.4f;

    bool draw_zone = false;

    int ray_count = 0;
    float circle_color[] = { 0.f, 0.f, 1.f };
    float buffer_circle_color[] = { 0.f, 0.f, 1.f };

    while (!glfwWindowShouldClose(mainWindow)) // main loop
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        glClearColor(0.2f, 0.2f, 0.2f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shader);
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

        ImGui::NewFrame();

        // GUI code goes here

        ImGui::Begin("Settings");
        ImGui::DragFloat("Value pos_x", &pos_x);
        ImGui::DragFloat("Value pos_y", &pos_y);
        ImGui::DragFloat("Cam x", &cam_x, 0.01f);
        ImGui::DragFloat("Cam y", &cam_y, 0.01f);
        ImGui::DragFloat("Cam z", &cam_z, 0.01f);
        ImGui::Checkbox("Draw zones", &draw_zone);
        ImGui::End();

        ImGui::Begin("Circle color");
        ImGui::ColorPicker3("##MyColor##5", (float*)&circle_color, ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha);
        ImGui::End(); 

        ImGui::Begin("Metrics");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::Text("Ray count : %d", ray_count);

        ImGui::End();

        ImGui::Render(); // render GUI
        

        // OpenGL code goes here

        if (buffer_pos_x != pos_x || buffer_pos_y != pos_y || buffer_circle_color != circle_color) {
            render_again = true;
            buffer_pos_x = pos_x; buffer_pos_y = pos_y;
            buffer_circle_color[0] = circle_color[0]; 
            buffer_circle_color[1] = circle_color[1]; 
            buffer_circle_color[2] = circle_color[2]; 
            ray_count = 0;
        }

        glm::mat4 Projection = glm::perspective(glm::radians(45.0f), (float) bufferWidth / (float) bufferHeight, 0.01f, 1000.0f);

        // Camera matrix
        glm::mat4 View = glm::lookAt(
            glm::vec3(cam_x, cam_y, cam_z), // Camera is at
            glm::vec3(cam_x, cam_y, cam_z - 1.), // and looks at the origin
            glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
        );
        // Model matrix : an identity matrix (model will be at the origin)
        glm::mat4 Model = glm::mat4(1.0f);
        // Our ModelViewProjection : multiplication of our 3 matrices
        MVP = Projection * View * Model; // Remember, matrix multiplication is the other way around

        if (draw_zone) {
            //drawing local zones
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            for (int i = 0; i < 342; i++) {
                // draw line

                glEnableVertexAttribArray(0);
                glBindBuffer(GL_ARRAY_BUFFER, VBO_zones[i]);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

                glEnableVertexAttribArray(1);
                glBindBuffer(GL_ARRAY_BUFFER, CBO_zones[i]);
                glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

                glDrawArrays(GL_TRIANGLES, 0, 3);
            }
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        // rays
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        
        vec2 center(pos_x * 10.f, pos_y * 10.f);
        for (int i = 0; i < line_count; i++) {
            
            float dx = cos(i * 2.f / line_count * 3.1415f); // on pourra opti ça
            float dy = sin(i * 2.f / line_count * 3.1415f);
            vec2 direction(dx, dy);

            float t = 0;
            float s = 0;
            bool found = true;

            if (render_again) {
                // calculating intersection
                vec2 best_intersection = getIntersection(direction, center, walls, wall, found);

                if (!found){ // obligé psq on continue de render les lignes, donc il faut override
                    create_line(center, center, VBO_lines[i], VAO_lines[i], CBO_lines[i], GREEN);
                    create_line(center, center, VBO_lines_r[i], VAO_lines_r[i], CBO_lines_r[i], GREEN);
                    create_line(center, center, VBO_lines_rr[i], VAO_lines_rr[i], CBO_lines_rr[i], GREEN);
                    continue;
                }

                create_line(center, best_intersection, VBO_lines[i], VAO_lines[i], CBO_lines[i], GREEN);
                ray_count++;
                // reflections
                
                vec2 reflection_vector = direction - 2.f * dot(direction, normalize(vec2(wall.u.y, -wall.u.x))) * normalize(vec2(wall.u.y, -wall.u.x));

                vec2 best_intersection_r = getIntersection(reflection_vector, best_intersection, walls, wall, found);

                if (!found) {
                    create_line(center, center, VBO_lines_r[i], VAO_lines_r[i], CBO_lines_r[i], GREEN);
                    create_line(center, center, VBO_lines_rr[i], VAO_lines_rr[i], CBO_lines_rr[i], GREEN);
                    continue;
                }

                create_line(best_intersection, best_intersection_r, VBO_lines_r[i], VAO_lines_r[i], CBO_lines_r[i], YELLOW);
                ray_count++;
                vec2 reflection_vector_2 = reflection_vector - 2.f * dot(reflection_vector, normalize(vec2(wall.u.y, -wall.u.x))) * normalize(vec2(wall.u.y, -wall.u.x));

                vec2 best_intersection_r2 = getIntersection(reflection_vector_2, best_intersection_r, walls, wall, found);

                if (!found) {
                    create_line(center, center, VBO_lines_rr[i], VAO_lines_rr[i], CBO_lines_rr[i], GREEN);
                    continue;
                }

                create_line(best_intersection_r, best_intersection_r2, VBO_lines_rr[i], VAO_lines_rr[i], CBO_lines_rr[i], RED);
                ray_count++;
            }
        }

        for (int i = 0; i < line_count; i++) {

            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, VBO_lines[i]);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

            glEnableVertexAttribArray(1);
            glBindBuffer(GL_ARRAY_BUFFER, CBO_lines[i]);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

            glDrawArrays(GL_TRIANGLES, 0, 3);

            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, VBO_lines_r[i]);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

            glEnableVertexAttribArray(1);
            glBindBuffer(GL_ARRAY_BUFFER, CBO_lines_r[i]);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

            glDrawArrays(GL_TRIANGLES, 0, 3);

            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, VBO_lines_rr[i]);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

            glEnableVertexAttribArray(1);
            glBindBuffer(GL_ARRAY_BUFFER, CBO_lines_rr[i]);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);


        // drawing walls
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        for (int i = 0; i < walls.size(); i++) {
            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, VBO_walls[i]);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

            glEnableVertexAttribArray(1);
            glBindBuffer(GL_ARRAY_BUFFER, CBO_walls[i]);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        // draw circle
        float radius = 15.f;
        //draw_circle(15.f, pos_x, pos_y, VBO_circle, CBO_circle, VAO_circle, BLUE);

        if (render_again) { // pas la méthode la plus opti pour dessiner un cercle mais soit (plus de triangles au centre qu'aux extrémités
            float x, y;
            // vec2 center(pos_x * 10.f, pos_y * 10.f);
            for (double i = 0; i <= 360;) {
                x = radius * cos(i * 3.141592f / 180.f) + pos_x * 10.f;
                y = radius * sin(i * 3.141592f / 180.f) + pos_y * 10.f;
                vec2 a(x, y);
                i = i + 0.5;
                x = radius * cos(i * 3.141592f / 180.f) + pos_x * 10.f;
                y = radius * sin(i * 3.141592f / 180.f) + pos_y * 10.f;
                vec2 b(x, y);

                int j = (int)2 * (i-0.5);
                create_triangle(a, center, b, VBO_circle[j], VAO_circle[j], CBO_circle[j], Color{ circle_color[0], circle_color[1], circle_color[2] });
            }
        }

        for (int i = 0; i <= 720; i++) {
            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, VBO_circle[i]);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

            glEnableVertexAttribArray(1);
            glBindBuffer(GL_ARRAY_BUFFER, CBO_circle[i]);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        glUseProgram(0);
        render_again = false;

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(mainWindow);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glDeleteFramebuffers(1, &FBO);
    glDeleteTextures(1, &texture_id);
    glDeleteRenderbuffers(1, &RBO);

    glfwDestroyWindow(mainWindow);
    glfwTerminate();

    return 0;
}
