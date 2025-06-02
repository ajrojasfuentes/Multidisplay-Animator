/*==============================================================================
  test.c

  Programa de prueba ampliado para la biblioteca “mypthreads”.
  - Crea hilos Round-Robin, Lottery y Real-Time; prueba join, detach y cambio de scheduler.
  - Agrega pruebas de mutex: lock, unlock, trylock y comportamiento con varios hilos.
==============================================================================*/
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>    // para usleep()
#include <time.h>      // para srand() y rand()
#include "mypthread.h"

/* ----------------------- Variables globales ----------------------- */

/* Hilos para pruebas de scheduler original */
my_thread_t *rr_for_change;
my_thread_t *changer_thread_ptr;
my_thread_t *join_target_ptr;
my_thread_t *joiner_ptr;
my_thread_t *detach_ptr;

/* Variables para pruebas de mutex */
my_mutex_t mtx;
int shared_counter = 0;

/* Hilos para incrementar contador protegido por mutex */
my_thread_t *inc_thread1;
my_thread_t *inc_thread2;

/* Hilo que prueba trylock */
my_thread_t *trylock_thread;

/* ----------------------- Funciones de hilo de ejemplo ----------------------- */

/* 1) Hilo Round-Robin simple: imprime 5 iteraciones y termina */
void rr_func(void) {
    for (int i = 0; i < 5; i++) {
        printf("[RR]   iteration %d\n", i);
        usleep(100000);
        my_thread_yield();
    }
    printf("[RR]   ending\n");
    my_thread_end();
}

/* 2) Hilo Lottery: imprime 5 iteraciones, muestra cuántos tickets tiene */
void lot_func(void) {
    for (int i = 0; i < 5; i++) {
        printf("[LOT]  iteration %d (tickets=%d)\n",
               i, current_thread->tickets);
        usleep(100000);
        my_thread_yield();
    }
    printf("[LOT]  ending\n");
    my_thread_end();
}

/* 3) Hilo Real-Time de prioridad alta: imprime 5 iteraciones */
void rt_high_func(void) {
    for (int i = 0; i < 5; i++) {
        printf("[RT+]  iteration %d (p=%d)\n",
               i, current_thread->rt_priority);
        usleep(100000);
        my_thread_yield();
    }
    printf("[RT+]  ending\n");
    my_thread_end();
}

/* 4) Hilo Real-Time de prioridad baja: imprime 5 iteraciones */
void rt_low_func(void) {
    for (int i = 0; i < 5; i++) {
        printf("[RT-]  iteration %d (p=%d)\n",
               i, current_thread->rt_priority);
        usleep(100000);
        my_thread_yield();
    }
    printf("[RT-]  ending\n");
    my_thread_end();
}

/* 5) Hilo “join target”: el hilo que otro hilo intentará unirse a él */
void join_target_func(void) {
    printf("[JT]   started\n");
    usleep(300000);
    printf("[JT]   ending\n");
    my_thread_end();
}

/* 6) Hilo “joiner”: hace un my_thread_join(join_target_ptr) */
void joiner_func(void) {
    printf("[JR]   waiting for JT\n");
    my_thread_join(join_target_ptr);
    printf("[JR]   JT finished, resuming\n");
    my_thread_end();
}

/* 7) Hilo detached: se marca como detached antes de iniciar scheduler */
void detach_func(void) {
    printf("[DT]   running (detached)\n");
    usleep(200000);
    printf("[DT]   ending\n");
    my_thread_end();
}

/* 8) Hilo que cambia scheduler de otro hilo:
      - Inicialmente rr_for_change está en RR.
      - Este hilo lo cambia a Lottery con 3 tickets.
*/
void changer_func(void) {
    printf("[CH]   rr_for_change was RR\n");
    usleep(150000);
    my_thread_chsched(rr_for_change, SCHED_LOTTERY, 3);
    printf("[CH]   changed rr_for_change → LOTTERY (3 tickets)\n");
    my_thread_end();
}

/* ----------------------- Funciones para pruebas de mutex ----------------------- */

/* 9) Hilo que incrementa shared_counter dentro de una sección crítica */
void inc_func(void) {
    for (int i = 0; i < 5; i++) {
        /* Intentar bloquear el mutex */
        printf("[INC]  hilo %p intentando lock...\n", (void*)current_thread);
        my_mutex_lock(&mtx);

        /* Sección crítica */
        int val = shared_counter;
        usleep(50000); // Simular trabajo
        shared_counter = val + 1;
        printf("[INC]  hilo %p incrementó counter a %d\n",
               (void*)current_thread, shared_counter);

        my_mutex_unlock(&mtx);
        printf("[INC]  hilo %p liberó lock\n", (void*)current_thread);

        usleep(100000);
        my_thread_yield();
    }
    my_thread_end();
}

/* 10) Hilo que prueba trylock:
      - Intenta adquirir sin bloquearse.
      - Si falla, imprime mensaje y vuelve a intentar 3 veces.
*/
void trylock_func(void) {
    for (int attempt = 1; attempt <= 3; attempt++) {
        printf("[TRY]  intento %d para trylock por hilo %p\n",
               attempt, (void*)current_thread);
        if (my_mutex_trylock(&mtx) == 0) {
            /* Éxito */
            printf("[TRY]  hilo %p adquirió lock con trylock\n", (void*)current_thread);
            usleep(100000);
            my_mutex_unlock(&mtx);
            printf("[TRY]  hilo %p liberó lock tras trylock\n", (void*)current_thread);
            my_thread_end();
            return;
        } else {
            /* Fracaso */
            printf("[TRY]  hilo %p no pudo adquirir lock, retry\n", (void*)current_thread);
            usleep(150000);
            my_thread_yield();
        }
    }
    printf("[TRY]  hilo %p renunció tras 3 intentos\n", (void*)current_thread);
    my_thread_end();
}

/* ----------------------------- Función main ----------------------------- */
int main(void) {
    /* 1) Inicializar las listas vacías y semilla de rand */
    rr_init();
    lottery_init();
    rt_init();
    srand((unsigned) time(NULL));

    /* 2) Inicializar mutex */
    if (my_mutex_init(&mtx) != 0) {
        fprintf(stderr, "Error inicializando mutex\n");
        exit(1);
    }

    /* ----- Pruebas de scheduler e hilos básicos ----- */

    /* 2.1) Hilo RR que más tarde cambiaremos de scheduler */
    if (my_thread_create(&rr_for_change, rr_func, SCHED_RR, 0) != 0) {
        fprintf(stderr, "Error creando rr_for_change (RR)\n");
        exit(1);
    }

    /* 2.2) Hilo que cambiará el scheduler de rr_for_change */
    if (my_thread_create(&changer_thread_ptr, changer_func, SCHED_RR, 0) != 0) {
        fprintf(stderr, "Error creando changer_thread (RR)\n");
        exit(1);
    }

    /* 2.3) Hilos para probar join/detach */
    if (my_thread_create(&join_target_ptr, join_target_func, SCHED_RR, 0) != 0) {
        fprintf(stderr, "Error creando join_target (RR)\n");
        exit(1);
    }
    if (my_thread_create(&joiner_ptr, joiner_func, SCHED_RR, 0) != 0) {
        fprintf(stderr, "Error creando joiner (RR)\n");
        exit(1);
    }
    if (my_thread_create(&detach_ptr, detach_func, SCHED_RR, 0) != 0) {
        fprintf(stderr, "Error creando detach (RR)\n");
        exit(1);
    }
    /* Lo marcamos como detached inmediatamente */
    my_thread_detach(detach_ptr);

    /* 2.4) Hilo de ejemplo Lottery (5 tickets) */
    my_thread_t *lot_thread;
    if (my_thread_create(&lot_thread, lot_func, SCHED_LOTTERY, 5) != 0) {
        fprintf(stderr, "Error creando lot_thread (Lottery)\n");
        exit(1);
    }

    /* 2.5) Hilos de ejemplo Real-Time: uno “alto” (p=5) y uno “bajo” (p=1) */
    my_thread_t *rt_h, *rt_l;
    if (my_thread_create(&rt_h, rt_high_func, SCHED_RT, 5) != 0) {
        fprintf(stderr, "Error creando rt_high (RT)\n");
        exit(1);
    }
    if (my_thread_create(&rt_l, rt_low_func, SCHED_RT, 1) != 0) {
        fprintf(stderr, "Error creando rt_low (RT)\n");
        exit(1);
    }

    /* ----- Pruebas de mutex: incrementadores y trylock ----- */

    /* 2.6) Dos hilos que compiten para incrementar shared_counter */
    if (my_thread_create(&inc_thread1, inc_func, SCHED_RR, 0) != 0) {
        fprintf(stderr, "Error creando inc_thread1 (RR)\n");
        exit(1);
    }
    if (my_thread_create(&inc_thread2, inc_func, SCHED_RR, 0) != 0) {
        fprintf(stderr, "Error creando inc_thread2 (RR)\n");
        exit(1);
    }

    /* 2.7) Un hilo que prueba trylock repetidamente */
    if (my_thread_create(&trylock_thread, trylock_func, SCHED_RR, 0) != 0) {
        fprintf(stderr, "Error creando trylock_thread (RR)\n");
        exit(1);
    }

    /* 3) Iniciar temporizador para preempción cada 100 ms */
    init_timer();

    /* 4) Entregar el primer hilo listo: RR → Lottery → RT */
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

    /* Si no hubo hilos, llegamos aquí */
    printf("No hay hilos que ejecutar.\n");

    /* 5) Intento de destruir mutex tras ejecución (debería estar desbloqueado y sin esperas) */
    if (my_mutex_destroy(&mtx) == 0) {
        printf("[MAIN] mutex destruido exitosamente\n");
    } else {
        printf("[MAIN] falla al destruir mutex (quizá hilos bloqueados o locked)\n");
    }

    return 0;
}
