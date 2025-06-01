#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mypthread.h"

my_thread_t *t1, *t2, *t3;

void tarea_join() {
    for (int i = 0; i < 3; i++) {
        printf("[T1] ciclo %d\n", i);
        usleep(50000);
    }
    printf("[T1] finalizado\n");
    my_thread_end();
}

void tarea_detach() {
    for (int i = 0; i < 3; i++) {
        printf("[T2] ciclo %d\n", i);
        usleep(50000);
    }
    printf("[T2] finalizado (DETACHED)\n");
    my_thread_end();
}

void tarea_principal() {
    if (!t1) {
        printf("[T3] Error: t1 no está inicializado\n");
        my_thread_end();
        return;
    }

    printf("[T3] esperando a T1...\n");
    my_thread_join(t1);
    printf("[T3] T1 terminó, ahora continúa T3\n");

    for (int i = 0; i < 2; i++) {
        printf("[T3] ciclo %d\n", i);
        usleep(50000);
    }

    printf("[T3] finalizado\n");
    my_thread_end();
}

int main() {
    t1 = t2 = t3 = NULL;
    main_thread = malloc(sizeof(my_thread_t));
    getcontext(&main_thread->context);

    rr_init();

    // Crear los hilos primero (asegurarse de que t1 esté listo antes de usarlo)
    if (my_thread_create(&t1, tarea_join) != 0) {
        fprintf(stderr, "Error creando t1\n");
        exit(1);
    }

    if (my_thread_create(&t2, tarea_detach) != 0) {
        fprintf(stderr, "Error creando t2\n");
        exit(1);
    }

    if (my_thread_create(&t3, tarea_principal) != 0) {
        fprintf(stderr, "Error creando t3\n");
        exit(1);
    }

    my_thread_detach(t2);

    // Solo después de crear todos los hilos: iniciar el timer
    init_timer();

    current_thread = rr_pick_next();
    setcontext(&(current_thread->context));

    return 0;
}