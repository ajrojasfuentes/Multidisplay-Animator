#include "mypthread.h"
#include <stdio.h>

#define N_THREADS 4
#define PER_THREAD 250000

static long counter = 0;
static my_mutex_t mtx;

void *adder(void *unused) {
    for (int i = 0; i < PER_THREAD; ++i) {
        my_mutex_lock(&mtx);
        ++counter;
        my_mutex_unlock(&mtx);
    }
    return NULL;
}

int main(void) {
    my_mutex_init(&mtx);
    my_thread_t thr[N_THREADS];

    for (int i = 0; i < N_THREADS; ++i)
        my_thread_create(&thr[i], NULL, adder, NULL, SCHED_RR);

    for (int i = 0; i < N_THREADS; ++i)
        my_thread_join(&thr[i], NULL);

    printf("counter = %ld (esperado %d)\n", counter, N_THREADS * PER_THREAD);
    return counter == N_THREADS * PER_THREAD ? 0 : 1;
}
