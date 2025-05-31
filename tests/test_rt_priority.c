#define _POSIX_C_SOURCE 200809L    /* clock_gettime, CLOCK_MONOTONIC */
    #include <time.h>
    #include <unistd.h>
    #include "mypthread.h"

    /* Prototipo del helper del scheduler RT */
    void rt_yield_current(struct my_thread *);
#include <stdio.h>
#include <time.h>

static inline uint64_t now_usec(void)
{
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (uint64_t)ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
    }

void *rt_task(void *arg) {
    (void)arg;
    printf("RT start  %llu us\n", (unsigned long long)now_usec());
    busy_loop();
    printf("RT finish %llu us\n", (unsigned long long)now_usec());
    return NULL;
}

void *bg_task(void *arg) {
    char c = (char)(intptr_t)arg;
    for (int i = 0; i < 5; ++i) {
        printf("BG %c (%d)\n", c, i);
        busy_loop();
    }
    return NULL;
}

int main(void) {
    my_thread_t rt;
    my_thread_t bg1, bg2;

    /* Crear dos hilos de fondo Round-Robin */
    my_thread_create(&bg1, NULL, bg_task, (void *)'A', SCHED_RR);
    my_thread_create(&bg2, NULL, bg_task, (void *)'B', SCHED_RR);

    /* Hilo RT con deadline muy cercano para que sea elegido primero */
    my_thread_create(&rt, NULL, rt_task, NULL, SCHED_RT);
    rt.deadline = now_usec() + 500;   /* 0,5 ms desde ahora */
    rt_yield_current(&rt);            /* re-insertar con deadline seteado */

    my_thread_join(&rt, NULL);
    my_thread_join(&bg1, NULL);
    my_thread_join(&bg2, NULL);
    return 0;
}
