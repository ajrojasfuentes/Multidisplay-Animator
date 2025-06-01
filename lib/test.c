/*
 * test.c
 *
 * Pruebas comparativas entre la biblioteca mypthread y la biblioteca estándar pthreads.
 *
 * - Verifica:
 *   1) Creación y unión de hilos.
 *   2) Retorno de valores desde hilos (retval).
 *   3) Uso de mutex para proteger un contador compartido.
 *   4) Cambio de política de scheduling con my_thread_chsched.
 *   5) Compatibilidad funcional: mismo número final de incrementos bajo mypthread y pthreads.
 *
 * Compilar con:
 *   gcc -std=gnu11 -Wall -Wextra -O2 test.c -L. -lmypthread -lpthread -o test_prog
 *
 * Asegúrate de haber generado previamente libmypthread.a (ver instrucciones anteriores).
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>

/* Cabeceras de ambas bibliotecas */
#include "mypthread.h"
#include <pthread.h>

/* ----------------------------- Constantes ----------------------------- */
#define NUM_THREADS       8
#define INCREMENTS_PER_T 100000

/* ----------------------- Pruebas con mypthread ------------------------ */

/* Contador compartido y mutex (mypthread) */
static int     my_count = 0;
static my_mutex_t my_mutex;

/* Función que cada hilo de mypthread ejecuta para incrementar contador */
static void *my_increment_fn(void *arg) {
    int idx = *(int *)arg;
    free(arg);

    /* Para demostrar change-sched, el hilo índice par cambia a LOTTERY,
       el impar a RT (pues my_thread_chsched admite estos valores). */
    if (idx % 2 == 0) {
        my_thread_chsched(SCHED_LOTTERY);
    } else {
        my_thread_chsched(SCHED_RT);
    }

    for (int i = 0; i < INCREMENTS_PER_T; i++) {
        my_mutex_lock(&my_mutex);
        my_count++;
        my_mutex_unlock(&my_mutex);
    }
    /* Retornamos el número de incrementos realizados */
    return (void *)(uintptr_t)INCREMENTS_PER_T;
}

/* Prueba completa de contador y join con mypthread */
static void test_mypthread_counter(void) {
    printf("=== test_mypthread_counter ===\n");
    my_count = 0;
    my_mutex_init(&my_mutex);

    my_thread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        int *arg = malloc(sizeof(int));
        if (!arg) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        *arg = i;
        int err = my_thread_create(&threads[i], NULL, my_increment_fn, arg, SCHED_RR);
        if (err != 0) {
            fprintf(stderr, "my_thread_create error: %d\n", err);
            exit(EXIT_FAILURE);
        }
    }

    /* Unir todos y recoger sus valores de retorno */
    for (int i = 0; i < NUM_THREADS; i++) {
        void *retval;
        int err = my_thread_join(&threads[i], &retval);
        if (err != 0) {
            fprintf(stderr, "my_thread_join error: %d\n", err);
            exit(EXIT_FAILURE);
        }
        uintptr_t v = (uintptr_t)retval;
        printf("  [mypthread] thread %d devolvió %lu\n", i, (unsigned long)v);
    }

    printf("  Resultado final con mypthread: my_count = %d (esperado %d)\n\n",
           my_count, NUM_THREADS * INCREMENTS_PER_T);
    my_mutex_destroy(&my_mutex);
}

/* ----------------------- Pruebas con pthreads ------------------------- */

/* Contador compartido y mutex (pthreads) */
static int        pt_count = 0;
static pthread_mutex_t pt_mutex;

/* Función que cada hilo pthread ejecuta para incrementar contador */
static void *pt_increment_fn(void *arg) {
    int idx = *(int *)arg;
    free(arg);

    for (int i = 0; i < INCREMENTS_PER_T; i++) {
        pthread_mutex_lock(&pt_mutex);
        pt_count++;
        pthread_mutex_unlock(&pt_mutex);
    }
    /* Retornamos el número de incrementos realizados */
    return (void *)(uintptr_t)INCREMENTS_PER_T;
}

/* Prueba completa de contador y join con pthreads */
static void test_pthread_counter(void) {
    printf("=== test_pthread_counter ===\n");
    pt_count = 0;
    pthread_mutex_init(&pt_mutex, NULL);

    pthread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        int *arg = malloc(sizeof(int));
        if (!arg) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        *arg = i;
        int err = pthread_create(&threads[i], NULL, pt_increment_fn, arg);
        if (err != 0) {
            fprintf(stderr, "pthread_create error: %d\n", err);
            exit(EXIT_FAILURE);
        }
    }

    /* Unir todos y recoger sus valores de retorno */
    for (int i = 0; i < NUM_THREADS; i++) {
        void *retval;
        int err = pthread_join(threads[i], &retval);
        if (err != 0) {
            fprintf(stderr, "pthread_join error: %d\n", err);
            exit(EXIT_FAILURE);
        }
        uintptr_t v = (uintptr_t)retval;
        printf("  [pthread] thread %d devolvió %lu\n", i, (unsigned long)v);
    }

    printf("  Resultado final con pthreads: pt_count = %d (esperado %d)\n\n",
           pt_count, NUM_THREADS * INCREMENTS_PER_T);
    pthread_mutex_destroy(&pt_mutex);
}

/* --------------------------- Función main ----------------------------- */
int main(void) {
    printf("\n** Iniciando pruebas de mypthread vs pthread **\n\n");

    /* 1) Prueba de contador protegido con mutex en mypthread */
    test_mypthread_counter();

    /* Agregar pequeña pausa */
    usleep(100000);

    /* 2) Prueba análoga con la biblioteca estándar pthreads */
    test_pthread_counter();

    printf("** Todas las pruebas concluyeron **\n");
    return 0;
}
