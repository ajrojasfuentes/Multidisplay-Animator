#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "animator.h"
#include "anim_utils.h"

void simulate_animation(const AnimationConfig *config) {
    int max_time = 0;
    for (int i = 0; i < config->num_figures; i++) {
        if (config->figures[i].t_end > max_time) {
            max_time = config->figures[i].t_end;
        }
    }

    for (int t = 0; t <= max_time; t++) {
        printf("\033[2J\033[H");  // Limpiar pantalla
        char canvas[config->canvas.height][config->canvas.width];
        memset(canvas, ' ', sizeof(canvas));

        for (int i = 0; i < config->num_figures; i++) {
            Figure *f = &config->figures[i];
            if (t < f->t_start || t > f->t_end) continue;

            Position pos = interpolate_position(f->pos0, f->pos1, t, f->t_start, f->t_end);

            // Seleccionar la rotación real (0, 90, 180, 270) según el tiempo
            int angles[] = {0, 90, 180, 270};
            int angle = angles[((t - f->t_start) / 2) % 4];
            char **shape = f->rotations[angle];
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

        usleep(100000); // 100 ms
    }

    printf("\n[FIN DE LA ANIMACIÓN]\n");
}