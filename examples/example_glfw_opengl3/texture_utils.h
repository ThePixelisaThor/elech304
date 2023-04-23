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
#include "color.h"

using namespace glm;

void load_texture(unsigned int &texture, bool flip, const char* path);
void create_textured_rectangle(vec3 tl, vec3 tr, vec3 br, vec3 bl, GLuint& VBO, GLuint& VAO, GLuint& EBO, Color c);
void draw_textured_rectangle(unsigned int& texture, GLuint& shader, GLuint& shader_textured, unsigned int VAO, unsigned int VBO, unsigned int EBO);
void create_textured_cube(vec3 utr, vec3 ubr, vec3 ubl, vec3 utl, vec3 ltr, vec3 lbr, vec3 lbl, vec3 ltl, GLuint* VBOs, GLuint* VAOs, GLuint* EBOs, Color color);
