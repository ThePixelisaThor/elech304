#pragma once
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
#include "color.h"


void create_line(vec2 a, vec2 b, GLuint& VBO, GLuint& VAO, GLuint& colorbuffer, Color color);
void create_triangle(vec2 a, vec2 b, vec2 c, GLuint& VBO, GLuint& VAO, GLuint& colorbuffer, Color color);
void create_rectangle(vec2 a, vec2 b, vec2 c, vec2 d, GLuint& VBO, GLuint& VAO, GLuint& colorbuffer, Color color);

void draw_arrays_rect(GLuint VBOs[], GLuint CBOs[], int length);
void draw_arrays(unsigned int VBOs[], unsigned int CBOs[], int length);
void draw_array(unsigned int VBOs, unsigned int CBOs);

void create_arrays(unsigned int VBOs[], unsigned int VAOs[], unsigned int CBOs[], int length);
void draw_array_rect(unsigned int VBO, unsigned int CBO);
