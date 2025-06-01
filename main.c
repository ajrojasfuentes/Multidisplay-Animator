#include <stdio.h>
#include <stdlib.h>
#include "config_parser.h"

int main() {
    AnimationConfig *config = load_config("config.json");
    if (!config) {
        fprintf(stderr, "Error al cargar la configuración\n");
        return 1;
    }

    printf("Canvas: %d x %d\n", config->canvas.width, config->canvas.height);
    printf("Número de figuras: %d\n", config->num_figures);

    for (int i = 0; i < config->num_figures; ++i) {
        Figure *f = &config->figures[i];
        printf("\nFigura %d:\n", i + 1);
        printf("  Tiempo: %d -> %d\n", f->t_start, f->t_end);
        printf("  Posición inicial: (%d, %d)\n", f->pos0.x, f->pos0.y);
        printf("  Posición final:   (%d, %d)\n", f->pos1.x, f->pos1.y);
        printf("  Tamaño: %d x %d\n", f->rows, f->cols);

        if (f->rotations[0]) {
            printf("  Rotación 0°:\n");
            for (int r = 0; r < f->rows; ++r) {
                printf("    %s\n", f->rotations[0][r]);
            }
        }

        if (f->rotations[90]) {
            printf("  Rotación 90°:\n");
            for (int r = 0; r < f->rows; ++r) {
                printf("    %s\n", f->rotations[90][r]);
            }
        }
    }

    free_config(config);
    return 0;
}