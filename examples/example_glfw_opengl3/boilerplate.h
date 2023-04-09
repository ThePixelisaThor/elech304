#pragma once
#include "../libs/GL/glew.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW\glfw3.h>
#include <iostream>
int load_all(int WIDTH, int HEIGHT, GLFWwindow* &mainWindow, int& bufferWidth, int& bufferHeight, ImGuiIO &io);
void render_end(GLFWwindow* &mainWindow, ImGuiIO& io);
void terminate(GLFWwindow* &mainWindow);
