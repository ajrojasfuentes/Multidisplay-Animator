#include "config_parser.h"
#include "animator.h"
#include "stdio.h"

int main() {
    AnimationConfig *config = load_config("config.json");
    if (!config) {
        fprintf(stderr, "Error cargando la configuración\n");
        return 1;
    }

    simulate_animation(config);
    free_config(config);
    return 0;
}