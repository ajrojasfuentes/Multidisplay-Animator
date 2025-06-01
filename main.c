#include <stdio.h>
#include <stdlib.h>
#include "config_parser.h"
#include "animator.h"

// Si luego usás mypthreads, incluí: #include "mypthread.h"

int main() {
    AnimationConfig *config = load_config("config.json");
    if (!config) {
        fprintf(stderr, "Error cargando la configuración\n");
        return 1;
    }

    // Animación secuencial
    // simulate_animation(config);

    // ⚠️ Aquí iría el despiegue multi-hilo con mypthreads en la Etapa 3
    // simulate_animation_multithread(config); ← futuro

    printf("Configuración cargada correctamente:\n");
    printf("Canvas: %d x %d\n", config->canvas.width, config->canvas.height);
    printf("Figuras: %d\n", config->num_figures);

    simulate_animation(config);

    free_config(config);
    return 0;
}