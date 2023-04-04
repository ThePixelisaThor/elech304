#include "shader_utils.h"

const char* vertex_shader_code_resize_1000x = R"*(
#version 330
layout (location = 0) in vec3 pos;
layout(location = 1) in vec3 vertexColor;

out vec3 fragmentColor;
uniform mat4 MVP; // transformation matrix
void main()
{
	gl_Position = MVP * vec4(0.001*pos.x - 0.5f, 0.001*pos.y + 0.5f, 0.5*pos.z, 1.0);
    fragmentColor = vertexColor;
}
)*";

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

void create_shader(GLuint& shader, const char* shader_code)
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
