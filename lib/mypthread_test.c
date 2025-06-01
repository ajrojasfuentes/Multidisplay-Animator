/*
* mypthread_test.c
 *
 * Prueba de incremento concurrente usando tu biblioteca mypthread.
 *
 * Compilar: gcc -std=gnu11 -Wall -Wextra -O2 mypthread_test.c -L. -lmypthread -o mypthread_test
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "mypthread.h"

#define NUM_THREADS       8
#define INCREMENTS_PER_T 100000

static int my_count = 0;
static my_mutex_t my_mutex;

static void *my_increment_fn(void *arg) {
    (void)arg;  /* no usamos índice aquí, solo incrementamos */
    for (int i = 0; i < INCREMENTS_PER_T; i++) {
        my_mutex_lock(&my_mutex);
        my_count++;
        my_mutex_unlock(&my_mutex);
    }
    return (void *)(uintptr_t)INCREMENTS_PER_T;
}

int main(void) {
    printf("=== mypthread_test: contador con mypthread ===\n");

    my_count = 0;
    my_mutex_init(&my_mutex);
    printf("=== Se inicializo el mutex ===\n");

    my_thread_t threads[NUM_THREADS];
    printf("=== Se crean los hilos ===\n");
    for (int i = 0; i < NUM_THREADS; i++) {
        int err = my_thread_create(&threads[i],
                                   NULL,
                                   my_increment_fn,
                                   NULL,
                                   SCHED_RR);
        if (err != 0) {
            fprintf(stderr, "my_thread_create error: %d\n", err);
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        void *retval;
        int err = my_thread_join(&threads[i], &retval);
        if (err != 0) {
            fprintf(stderr, "my_thread_join error: %d\n", err);
            exit(EXIT_FAILURE);
        }
        printf("  [mypthread] thread %d devolvió %lu\n",
               i, (unsigned long)(uintptr_t)retval);
    }

    printf("  Resultado final: my_count = %d (esperado %d)\n",
           my_count, NUM_THREADS * INCREMENTS_PER_T);

    my_mutex_destroy(&my_mutex);
    return 0;
}
