#include "render_utils.h"

void create_line(vec2 a, vec2 b, GLuint& VBO, GLuint& VAO, GLuint& colorbuffer, Color color)
{
    GLfloat colors[] = {
        color.r, color.g, color.b,
        color.r, color.g, color.b,
        color.r, color.g, color.b,
    };

    // setup color
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);

    GLfloat vertices[] = {
        a.x, a.y, 0.0f,
        a.x, a.y, 0.0f,
        b.x, b.y, 0.0f,
    };

    // setup vertices
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
}

void create_triangle(vec2 a, vec2 b, vec2 c, GLuint& VBO, GLuint& VAO, GLuint& colorbuffer, Color color)
{
    GLfloat colors[] = {
        color.r, color.g, color.b,
        color.r, color.g, color.b,
        color.r, color.g, color.b,
    };

    // setup color
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);

    GLfloat vertices[] = {
        a.x, a.y, 0.0f,
        b.x, b.y, 0.0f,
        c.x, c.y, 0.0f,
    };

    // setup vertices
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
}

void draw_arrays(unsigned int VBOs[], unsigned int CBOs[], int length) {
    for (int i = 0; i < length; i++) draw_array(VBOs[i], CBOs[i]);
}

void draw_array(unsigned int VBO, unsigned int CBO) {
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, CBO);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void create_arrays(unsigned int VBOs[], unsigned int VAOs[], unsigned int CBOs[], int length) {
    glGenVertexArrays(length, VAOs); // we can generate multiple VAOs or buffers at the same time
    glGenBuffers(length, VBOs);
    glGenBuffers(length, CBOs);
}
