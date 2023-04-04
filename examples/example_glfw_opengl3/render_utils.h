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
