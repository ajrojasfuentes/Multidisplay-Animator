#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "lib/mypthread.h"
#include "anim_config.h"
#include "anim_utils.h"

#define MAX_HEIGHT 100
#define MAX_WIDTH  100

// Canvas global compartido
static char canvas[MAX_HEIGHT][MAX_WIDTH];
static my_mutex_t canvas_mutex;

typedef struct {
    Figure *figure;
    int canvas_width;
    int canvas_height;
} AnimatorArgs;

/**
 * Función que ejecuta cada hilo para animar su figura en el canvas.
 */
void animator_thread_func(void) {
    AnimatorArgs *args = (AnimatorArgs *) current_thread->arg;
    Figure *f = args->figure;
    int width = args->canvas_width;
    int height = args->canvas_height;

    for (int t = 0; t <= f->t_end; t++) {
        if (t < f->t_start) {
            usleep(100000);  // esperar su turno
            my_thread_yield();
            continue;
        }

        Position pos = interpolate_position(f->pos0, f->pos1, t, f->t_start, f->t_end);
        int angles[] = {0, 90, 180, 270};
        int angle = angles[((t - f->t_start) / 2) % 4];
        char **shape = f->rotations[angle];
        if (!shape) continue;

        my_mutex_lock(&canvas_mutex);

        // Pintar figura en canvas
        for (int r = 0; r < f->rows; r++) {
            for (int c = 0; c < f->cols; c++) {
                int y = pos.y + r;
                int x = pos.x + c;
                if (y >= 0 && y < height && x >= 0 && x < width && shape[r][c] != ' ') {
                    canvas[y][x] = shape[r][c];
                }
            }
        }

        // Imprimir canvas (una sola figura puede hacerlo, por ahora)
        printf("\033[2J\033[H");  // limpiar pantalla
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                putchar(canvas[i][j]);
            }
            putchar('\n');
        }

        my_mutex_unlock(&canvas_mutex);

        usleep(100000);  // 100 ms
        my_thread_yield();
    }

    my_thread_end();
}

/**
 * Lanza un hilo mypthread por cada figura y sincroniza su ejecución.
 */
void simulate_animation_multithread(const AnimationConfig *config) {
    my_thread_t *threads[config->num_figures];

    // Inicializar canvas a espacios
    memset(canvas, ' ', sizeof(canvas));

    // Inicializar mutex del canvas
    if (my_mutex_init(&canvas_mutex) != 0) {
        fprintf(stderr, "Error inicializando mutex del canvas\n");
        return;
    }

    // Crear un hilo por figura
    for (int i = 0; i < config->num_figures; i++) {
        AnimatorArgs *args = malloc(sizeof(AnimatorArgs));
        args->figure = &config->figures[i];
        args->canvas_width = config->canvas.width;
        args->canvas_height = config->canvas.height;

        if (my_thread_create(&threads[i], animator_thread_func, SCHED_RR, 0) != 0) {
            fprintf(stderr, "Error creando hilo para figura %d\n", i);
            exit(1);
        }

        threads[i]->arg = (void *)args;
    }

    // Iniciar temporizador de mypthreads (SIGALRM)
    init_timer();

    // Activar el primer hilo listo: RR → Lottery → RT
    if (rr_head) {
        current_thread = rr_head;
        setcontext(&rr_head->context);
    } else if (lottery_head) {
        current_thread = lottery_head;
        setcontext(&lottery_head->context);
    } else if (rt_head) {
        current_thread = rt_head;
        setcontext(&rt_head->context);
    }

    printf("No hay hilos que ejecutar.\n");
}