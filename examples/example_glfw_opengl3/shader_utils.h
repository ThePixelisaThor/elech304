#pragma once
#include <iostream>
#include <string>

#include "../libs/GL/glew.h"
#include <GLFW\glfw3.h>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <wtypes.h>
#include <fstream>

void add_shader(GLuint program, const char* shader_code, GLenum type);
void create_shader(GLuint& shader, const char* shader_code);
void create_shader(GLuint& shader, GLuint& shader_texture);
