#pragma once
void compute_image_vectors(std::vector<Ray>& all_rays, vec2 center_tx, vec2 center_rx, std::vector<Wall>& walls_obj,
    int& ray_rx_count, int& total_ray_count, unsigned int* ray_cbo_buffer, unsigned int* ray_vbo_buffer, unsigned int* ray_vao_buffer);

void compute_draw_ray_cast(bool render_again, vec2 center_tx,
    unsigned int* VBO_lines, unsigned int* VAO_lines, unsigned int* CBO_lines,
    unsigned int* VBO_lines_r, unsigned int* VAO_lines_r, unsigned int* CBO_lines_r,
    unsigned int* VBO_lines_rr, unsigned int* VAO_lines_rr, unsigned int* CBO_lines_rr,
    std::vector<FancyVector> walls, int& ray_count);

void compute_circle(vec2 center_tx, vec2 center_rx,
    unsigned int* VBO_circle_tx, unsigned int* VAO_circle_tx, unsigned int* CBO_circle_tx,
    unsigned int* VBO_circle_rx, unsigned int* VAO_circle_rx, unsigned int* CBO_circle_rx,
    float* circle_color);
