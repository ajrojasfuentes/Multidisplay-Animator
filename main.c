#include "config_parser.h"
#include "animator.h"
#include "stdio.h"

int main() {
    AnimationConfig *config = load_config("config.json");
    if (!config) {
        fprintf(stderr, "Error cargando la configuración\n");
        return 1;
    }

    printf("Config cargado correctamente:\n");
    printf("Canvas: %d x %d\n", config->canvas.width, config->canvas.height);
    printf("Figuras: %d\n", config->num_figures);
    for (int i = 0; i < config->num_figures; i++) {
        printf("Figura %d: rotaciones = %d\n", i, config->figures[i].num_rotations);
    }

    simulate_animation(config);
    free_config(config);
    return 0;
}