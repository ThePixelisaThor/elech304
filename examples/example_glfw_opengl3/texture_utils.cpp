#include "texture_utils.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <filesystem>
#include "color.h"


using namespace glm;

void draw_textured_rectangle(unsigned int &texture, GLuint& shader, GLuint& shader_textured, unsigned int VAO, unsigned int VBO, unsigned int EBO) {
    glUseProgram(shader_textured);
    // bind textures on corresponding texture units

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    // render container
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glUseProgram(shader);
}

void load_texture(unsigned int &texture, bool flip, const char* path) {
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glActiveTexture(GL_TEXTURE0);

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_MIRRORED_REPEAT (default wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load image, create texture and generate mipmaps
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(flip); // tell stb_image.h to flip loaded texture's on the y-axis.
    // The FileSystem::getPath(...) is part of the GitHub repository so we can find files on any IDE/platform; replace it with your own image path.
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 3);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);
}

void create_textured_rectangle(vec3 tl, vec3 tr, vec3 br, vec3 bl, GLuint& VBO, GLuint& VAO, GLuint& EBO, Color color) {
    float texture_size = 25.f; // 2.5 meter

    float x_ratio = 1.f;
    float y_ratio = 1.f;
    float texture_ratio = 1.f;

    if (abs(tl.x - br.x) > abs(tl.y - br.y) && tl.z - br.z == 0) {
        y_ratio *= abs((tl.x - br.x) / (tl.y - br.y));
        texture_ratio = abs(tl.y - br.y) / texture_size;
    }
    else if (abs(tl.x - br.x) < abs(tl.y - br.y) && tl.z - br.z == 0) {
        y_ratio *= abs((tl.y - br.y) / (tl.x - br.x));
        texture_ratio = abs(tl.x - br.x) / texture_size;
    }
    else if (abs(tl.x - br.x) > abs(tl.z - br.z) && tl.y - br.y == 0) {
        x_ratio *= abs((tl.x - br.x) / (tl.z - br.z));
        texture_ratio = abs(tl.z - br.z) / texture_size;
    }
    else if (abs(tl.x - br.x) < abs(tl.z - br.z) && tl.y - br.y == 0) {
        x_ratio *= abs((tl.z - br.z) / (tl.x - br.x));
        texture_ratio = abs(tl.x - br.x) / texture_size;
    }
    else if (abs(tl.y - br.y) > abs(tl.z - br.z) && tl.x - br.x == 0) {
        x_ratio *= abs((tl.y - br.y) / (tl.z - br.z));
        texture_ratio = abs(tl.z - br.z) / texture_size;
    }
    else {
        x_ratio *= abs((tl.z - br.z) / (tl.y - br.y));
        texture_ratio = abs(tl.y - br.y) / texture_size;
    }

    x_ratio *= texture_ratio;
    y_ratio *= texture_ratio;

    float vertices[] = {
        // positions          // colors                     // texture coords
         tr.x, tr.y, tr.z,   color.r, color.g, color.b,   x_ratio, y_ratio,   // top right
         br.x, br.y, br.z,   color.r, color.g, color.b,   x_ratio, 0.f,   // bottom right
         bl.x, bl.y, bl.z,   color.r, color.g, color.b,   0.0f, 0.0f,   // bottom left
         tl.x, tl.y, tl.z,   color.r, color.g, color.b,   0.0f, y_ratio  // top left 
    };

    unsigned int indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
}

void create_textured_cube(vec3 utr, vec3 ubr, vec3 ubl, vec3 utl, vec3 ltr, vec3 lbr, vec3 lbl, vec3 ltl, GLuint *VBOs, GLuint* VAOs, GLuint *EBOs, Color color) {
    create_textured_rectangle(utr, ubr, ubl, utl, VBOs[0], VAOs[0], EBOs[0], color);
    create_textured_rectangle(utl, ubl, lbl, ltl, VBOs[1], VAOs[1], EBOs[1], color);
    create_textured_rectangle(ubl, ubr, lbr, lbl, VBOs[2], VAOs[2], EBOs[2], color);
    create_textured_rectangle(ubr, utr, ltr, lbr, VBOs[3], VAOs[3], EBOs[3], color);
    create_textured_rectangle(utr, utl, ltl, ltr, VBOs[4], VAOs[4], EBOs[4], color);
    create_textured_rectangle(ltr, lbr, lbl, ltl, VBOs[5], VAOs[5], EBOs[5], color);
}
