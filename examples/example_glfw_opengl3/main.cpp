#include <iostream>
#include <string>

#include "../libs/GL/glew.h"
#include <GLFW\glfw3.h>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "../libs/nlohmann/json.hpp"
using json = nlohmann::json;
using namespace glm;

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <wtypes.h>
#include <fstream>

const GLint WIDTH = 1920;
const GLint HEIGHT = 1080;

GLuint FBO;
GLuint RBO;
GLuint texture_id;
GLuint shader_white;
GLuint shader_red;
GLuint shader_blue;

struct FancyVector {
    vec2 a;
    vec2 b;
    vec2 vector;
};

const char* vertex_shader_code_resize_1000x = R"*(
#version 330
layout (location = 0) in vec3 pos;
void main()
{
	gl_Position = vec4(0.001*pos.x - 0.5f, 0.001*pos.y + 0.5f, 0.5*pos.z, 1.0);
}
)*";

const char* fragment_shader_code_white = R"*(
#version 330
out vec4 color;
void main()
{
	color = vec4(1.0, 1.0, 1.0, 1.0);
}
)*";

const char* fragment_shader_code_red = R"*(
#version 330
out vec4 color;
void main()
{
	color = vec4(1.0, 0.0, 0.0, 1.0);
}
)*";

const char* fragment_shader_code_blue = R"*(
#version 330
out vec4 color;
void main()
{
	color = vec4(0.0, 0.0, 1.0, 1.0);
}
)*";

int array_size = 0;

void create_line(vec2 a, vec2 b, GLuint &VBO, GLuint &VAO)
{
    GLfloat vertices[] = {
        a.x, a.y, 0.0f,
        a.x, a.y, 0.0f,
        b.x, b.y, 0.0f,
    };

    // setup line
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

void create_triangle(vec2 a, vec2 b, vec2 c, GLuint& VBO, GLuint& VAO)
{
    GLfloat vertices[] = {
        a.x, a.y, 0.0f,
        b.x, b.y, 0.0f,
        c.x, c.y, 0.0f
    };

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    array_size = sizeof(vertices);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void create_rectangle(vec2 a, vec2 b, vec2 c, vec2 d, GLuint& VBO, GLuint& VAO)
{
    GLfloat vertices[] = {
        a.x, a.y, 0.0f,
        b.x, b.y, 0.0f,
        d.x, d.y, 0.0f,
        a.x, a.y, 0.0f,
        c.x, c.y, 0.0f,
        d.x, d.y, 0.0f,
    };

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    array_size = sizeof(vertices);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

}

void create_circle(float radius, float pos_x, float pos_y, GLuint& VBO, GLuint& VAO) {
    float x, y;
    vec2 center(pos_x * 10.f, pos_y * 10.f);
    for (double i = 0; i <= 360;) {
        x = radius * cos(i) + pos_x * 10.f;
        y = radius * sin(i) + pos_y * 10.f;
        vec2 a(x, y);
        i = i + .5;
        x = radius * cos(i) + pos_x * 10.f;
        y = radius * sin(i) + pos_y * 10.f;
        vec2 b(x, y);
        i = i + .5;
        create_triangle(a, b, center, VBO, VAO);
        glUseProgram(shader_white);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 1 + array_size);
    }
}

void add_shader(GLuint program, const char* shader_code, GLenum type)
{
    GLuint current_shader = glCreateShader(type);

    const GLchar* code[1];
    code[0] = shader_code;

    GLint code_length[1];
    code_length[0] = strlen(shader_code);

    glShaderSource(current_shader, 1, code, code_length);
    glCompileShader(current_shader);

    GLint result = 0;
    GLchar log[1024] = { 0 };

    glGetShaderiv(current_shader, GL_COMPILE_STATUS, &result);
    if (!result) {
        glGetShaderInfoLog(current_shader, sizeof(log), NULL, log);
        std::cout << "Error compiling " << type << " shader: " << log << "\n";
        return;
    }

    glAttachShader(program, current_shader);
}

void create_shader(GLuint &shader, const char *shader_code)
{
    shader = glCreateProgram();
    if (!shader) {
        std::cout << "Error creating shader program!\n";
        exit(1);
    }

    add_shader(shader, vertex_shader_code_resize_1000x, GL_VERTEX_SHADER);
    add_shader(shader, shader_code, GL_FRAGMENT_SHADER);

    GLint result = 0;
    GLchar log[1024] = { 0 };

    glLinkProgram(shader);
    glGetProgramiv(shader, GL_LINK_STATUS, &result);
    if (!result) {
        glGetProgramInfoLog(shader, sizeof(log), NULL, log);
        std::cout << "Error linking program:\n" << log << '\n';
        return;
    }

    glValidateProgram(shader);
    glGetProgramiv(shader, GL_VALIDATE_STATUS, &result);
    if (!result) {
        glGetProgramInfoLog(shader, sizeof(log), NULL, log);
        std::cout << "Error validating program:\n" << log << '\n';
        return;
    }
}

void create_shaders()
{
    create_shader(shader_white, fragment_shader_code_white);
    create_shader(shader_red, fragment_shader_code_red);
    create_shader(shader_blue, fragment_shader_code_blue);
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

    // Create the window
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

    glViewport(0, 0, min(bufferWidth, bufferHeight), min(bufferWidth, bufferHeight));


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

        FancyVector temp_vector = { a, b, b - a};
        walls.push_back(temp_vector);
    }


    float pos_x = 20.f;
    float pos_y = -10.f;

    int material = 0;


    // lines
    const int line_count = 50;
    unsigned int VBO_lines[line_count], VAO_lines[line_count];

    // init walls
    FancyVector wall;
    unsigned int VBO_walls[100], VAO_walls[100]; // only for walls
    glGenVertexArrays(100, VAO_walls); // we can generate multiple VAOs or buffers at the same time
    glGenBuffers(100, VBO_walls);
    for (int i = 0; i < walls.size(); i++) {
        create_line(walls[i].a, walls[i].b, VBO_walls[i], VAO_walls[i]);
    }


    while (!glfwWindowShouldClose(mainWindow)) // main loop
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        glClearColor(0.07f, 0.07f, 0.07f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui::NewFrame();

        // GUI code goes here

        ImGui::Begin("Settings");
        ImGui::DragFloat("Value pos_x", &pos_x);
        ImGui::DragFloat("Value pos_y", &pos_y);
        ImGui::End();

        ImGui::Begin("Circle color");
        static float color = 0.f;
        ImGui::ColorPicker3("##MyColor##5", (float*)&color, ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha);
        ImGui::End(); 

        ImGui::Begin("Metrics");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();

        ImGui::Render(); // render GUI
        

        // OpenGL code goes here

        // rays
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glUseProgram(shader_white);
        vec2 center(pos_x * 10.f, pos_y * 10.f);
        for (int i = 0; i < line_count; i++) {
            float dx = cos(i * 2.f / line_count * 3.1415f); // on pourra opti ça
            float dy = sin(i * 2.f / line_count * 3.1415f);
            vec2 direction(dx, dy);

            float t = 0;
            float s = 0;
            // calculating intersection
            vec2 best_intersection(-1000, -1000);
            double best_distance = 1000000;

            for (int w = 0; w < walls.size(); w ++) {
                wall = walls[w];
                if (wall.vector.y == 0) {
                    t = (wall.a.y - center.y) / direction.y;
                    s = (center.x + direction.x * t - wall.a.x) / wall.vector.x;
                }
                else {
                    t = wall.vector.y / (direction.x * wall.vector.y - wall.vector.x * direction.y) * (wall.a.x - center.x + wall.vector.x * (center.y - wall.a.y) / wall.vector.y);
                    s = (center.y + direction.y * t - wall.a.y) / wall.vector.y;
                }

                if (t <= 0 || t > 100000 || s < 0 || s > 1) continue;

                vec2 intersection = vec2(center.x + t * direction.x, center.y + t * direction.y);
                double distance = length(intersection - center);

                if (distance < best_distance) {
                    best_distance = distance;
                    best_intersection = intersection;
                }
            }
            if (best_intersection.x == -1000 && best_intersection.y == -1000) continue;

            create_line(center, best_intersection, VBO_lines[i], VAO_lines[i]);
            

            glBindVertexArray(VAO_lines[i]);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);


        // drawing walls
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        for (int i = 0; i < walls.size(); i++) {
            // draw line
            material = data[i]["material_id"];
            glUseProgram(material == 1 ? shader_blue : material == 2 ? shader_red : shader_white);
            glBindVertexArray(VAO_walls[i]);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);



        // draw circle
        // create_circle(15.f, pos_x, pos_y, VBO, VAO);

        // draw rays around circle
        

        glUseProgram(0);


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
