#include <vector>
#include "ray.h"
#include "raytracing.h"
#include "raytracing.h"
#include "constants.h"
#include "zone_tracing.h"
#include "main_utilities.h"

void compute_image_vectors(std::vector<Ray> &all_rays, vec2 center_tx, vec2 center_rx, std::vector<Wall>& walls_obj,
    int& ray_rx_count, int& total_ray_count, unsigned int* ray_cbo_buffer, unsigned int* ray_vbo_buffer, unsigned int* ray_vao_buffer) {
    all_rays = {};
    // trajet direct
    std::vector<Ray> _buffer;

    compute_ray(center_tx, center_rx, center_rx - center_tx, center_tx, all_rays, _buffer, walls_obj, std::vector<int>(), 1.0f, 0, 0, 0);
    std::vector<vec2> only_rx;
    only_rx.push_back(center_rx);
    generate_rays_direction(center_tx, center_rx, center_rx, walls_obj, -1, all_rays, std::vector<int>(), only_rx, reflection_count_shown, 0);

    std::vector<Ray> rays_rx;

    generate_ray_hitting_rx(center_tx, center_rx, walls_obj, rays_rx, reflection_count_shown);
    ray_rx_count = rays_rx.size();

    total_ray_count = all_rays.size();

    for (int i = 0; i < all_rays.size(); i++) {
        all_rays[i].create(ray_vbo_buffer[i], ray_vao_buffer[i], ray_cbo_buffer[i]);
    }
}

void compute_draw_ray_cast(bool render_again, vec2 center_tx,
    unsigned int* VBO_lines, unsigned int* VAO_lines, unsigned int* CBO_lines,
    unsigned int* VBO_lines_r, unsigned int* VAO_lines_r, unsigned int* CBO_lines_r,
    unsigned int* VBO_lines_rr, unsigned int* VAO_lines_rr, unsigned int* CBO_lines_rr,
    std::vector<FancyVector> walls, int& ray_count) {

    FancyVector wall;
    for (int i = 0; i < line_count; i++) {

        float dx = cos(i * 2.f / line_count * PI); // on pourra opti ça
        float dy = sin(i * 2.f / line_count * PI);
        vec2 direction(dx, dy);

        float t = 0;
        float s = 0;
        bool found = true;

        if (render_again) {
            // calculating intersection
            vec2 best_intersection = getIntersection(direction, center_tx, walls, wall, found);

            if (!found) { // obligé psq on continue de render les lignes, donc il faut override
                create_line(center_tx, center_tx, VBO_lines[i], VAO_lines[i], CBO_lines[i], GREEN);
                create_line(center_tx, center_tx, VBO_lines_r[i], VAO_lines_r[i], CBO_lines_r[i], GREEN);
                create_line(center_tx, center_tx, VBO_lines_rr[i], VAO_lines_rr[i], CBO_lines_rr[i], GREEN);
                continue;
            }

            create_line(center_tx, best_intersection, VBO_lines[i], VAO_lines[i], CBO_lines[i], GREEN);
            ray_count++;
            // reflections

            vec2 reflection_vector = direction - 2.f * dot(direction, normalize(vec2(wall.u.y, -wall.u.x))) * normalize(vec2(wall.u.y, -wall.u.x));

            vec2 best_intersection_r = getIntersection(reflection_vector, best_intersection, walls, wall, found);

            if (!found) {
                create_line(center_tx, center_tx, VBO_lines_r[i], VAO_lines_r[i], CBO_lines_r[i], GREEN);
                create_line(center_tx, center_tx, VBO_lines_rr[i], VAO_lines_rr[i], CBO_lines_rr[i], GREEN);
                continue;
            }

            create_line(best_intersection, best_intersection_r, VBO_lines_r[i], VAO_lines_r[i], CBO_lines_r[i], YELLOW);
            ray_count++;
            vec2 reflection_vector_2 = reflection_vector - 2.f * dot(reflection_vector, normalize(vec2(wall.u.y, -wall.u.x))) * normalize(vec2(wall.u.y, -wall.u.x));

            vec2 best_intersection_r2 = getIntersection(reflection_vector_2, best_intersection_r, walls, wall, found);

            if (!found) {
                create_line(center_tx, center_tx, VBO_lines_rr[i], VAO_lines_rr[i], CBO_lines_rr[i], GREEN);
                continue;
            }

            create_line(best_intersection_r, best_intersection_r2, VBO_lines_rr[i], VAO_lines_rr[i], CBO_lines_rr[i], RED);
            ray_count++;
        }
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    draw_arrays(VBO_lines, CBO_lines, line_count);
    draw_arrays(VBO_lines_r, CBO_lines_r, line_count);
    draw_arrays(VBO_lines_rr, CBO_lines_rr, line_count);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void compute_circle(vec2 center_tx, vec2 center_rx,
    unsigned int* VBO_circle_tx, unsigned int* VAO_circle_tx, unsigned int* CBO_circle_tx,
    unsigned int* VBO_circle_rx, unsigned int* VAO_circle_rx, unsigned int* CBO_circle_rx,
    float* circle_color) {

    // pas la méthode la plus opti pour dessiner un cercle mais soit (plus de triangles au centre qu'aux extrémités)
    float x, y;
    for (double i = 0; i <= 360;) {
        x = circle_radius * cos(i * PI / 180.f) + center_tx.x;
        y = circle_radius * sin(i * PI / 180.f) + center_tx.y;
        vec2 a(x, y);
        i = i + 0.5;
        x = circle_radius * cos(i * PI / 180.f) + center_tx.x;
        y = circle_radius * sin(i * PI / 180.f) + center_tx.y;
        vec2 b(x, y);

        int j = (int)2 * (i - 0.5);
        create_triangle(a, center_tx, b, VBO_circle_tx[j], VAO_circle_tx[j], CBO_circle_tx[j], Color{ circle_color[0], circle_color[1], circle_color[2] });
    }

    for (double i = 0; i <= 360;) {
        x = circle_radius * cos(i * PI / 180.f) + center_rx.x;
        y = circle_radius * sin(i * PI / 180.f) + center_rx.y;
        vec2 a(x, y);
        i = i + 0.5;
        x = circle_radius * cos(i * PI / 180.f) + center_rx.x;
        y = circle_radius * sin(i * PI / 180.f) + center_rx.y;
        vec2 b(x, y);

        int j = (int)2 * (i - 0.5);
        create_triangle(a, center_rx, b, VBO_circle_rx[j], VAO_circle_rx[j], CBO_circle_rx[j], Color{ circle_color[0], circle_color[1], circle_color[2] });
    }
}
