/*
* pthread_test.c
 *
 * Prueba de incremento concurrente usando la biblioteca estándar pthreads.
 *
 * Compilar: gcc -std=gnu11 -Wall -Wextra -O2 pthread_test.c -pthread -o pthread_test
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#define NUM_THREADS       8
#define INCREMENTS_PER_T 100000

static int pt_count = 0;
static pthread_mutex_t pt_mutex;

static void *pt_increment_fn(void *arg) {
    (void)arg;  /* no usamos índice aquí, solo incrementamos */
    for (int i = 0; i < INCREMENTS_PER_T; i++) {
        pthread_mutex_lock(&pt_mutex);
        pt_count++;
        pthread_mutex_unlock(&pt_mutex);
    }
    return (void *)(uintptr_t)INCREMENTS_PER_T;
}

int main(void) {
    printf("=== pthread_test: contador con pthreads ===\n");

    pt_count = 0;
    pthread_mutex_init(&pt_mutex, NULL);

    pthread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_create(&threads[i], NULL, pt_increment_fn, NULL) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        void *retval;
        if (pthread_join(threads[i], &retval) != 0) {
            perror("pthread_join");
            exit(EXIT_FAILURE);
        }
        printf("  [pthread] thread %d devolvió %lu\n",
               i, (unsigned long)(uintptr_t)retval);
    }

    printf("  Resultado final: pt_count = %d (esperado %d)\n",
           pt_count, NUM_THREADS * INCREMENTS_PER_T);

    pthread_mutex_destroy(&pt_mutex);
    return 0;
}
