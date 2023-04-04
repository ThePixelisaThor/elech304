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


