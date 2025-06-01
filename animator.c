#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "animator.h"

static Position interpolate_position(Position p0, Position p1, int t, int t_start, int t_end) {
    if (t <= t_start) return p0;
    if (t >= t_end) return p1;

    float alpha = (float)(t - t_start) / (t_end - t_start);
    Position result;
    result.x = p0.x + (int)((p1.x - p0.x) * alpha);
    result.y = p0.y + (int)((p1.y - p0.y) * alpha);
    return result;
}

void simulate_animation(const AnimationConfig *config) {
    int max_time = 0;
    for (int i = 0; i < config->num_figures; i++) {
        if (config->figures[i].t_end > max_time) {
            max_time = config->figures[i].t_end;
        }
    }

    for (int t = 0; t <= max_time; t++) {
        printf("\033[2J\033[H");  // Clear screen
        char canvas[config->canvas.height][config->canvas.width];
        memset(canvas, ' ', sizeof(canvas));

        for (int i = 0; i < config->num_figures; i++) {
            Figure *f = &config->figures[i];
            if (t < f->t_start || t > f->t_end) continue;

            Position pos = interpolate_position(f->pos0, f->pos1, t, f->t_start, f->t_end);

            // Selecciona la rotación: 0, 90, 180, 270 (una cada 2 segundos)
            int rotation_index = ((t - f->t_start) / 2) % 4;
            char **shape = f->rotations[rotation_index];
            if (!shape) continue; // puede que no exista esa rotación

            for (int r = 0; r < f->rows; r++) {
                for (int c = 0; c < f->cols; c++) {
                    int y = pos.y + r;
                    int x = pos.x + c;
                    if (y >= 0 && y < config->canvas.height &&
                        x >= 0 && x < config->canvas.width &&
                        shape[r][c] != ' ') {
                        canvas[y][x] = shape[r][c];
                    }
                }
            }
        }

        for (int i = 0; i < config->canvas.height; i++) {
            for (int j = 0; j < config->canvas.width; j++) {
                putchar(canvas[i][j]);
            }
            putchar('\n');
        }

        usleep(100000); // 100ms
    }

    printf("\n[FIN DE LA ANIMACIÓN]\n");
}