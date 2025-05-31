#include "mypthread.h"
#include <stdio.h>
#include <unistd.h>          /* usleep */
#include <stdint.h>
#include <unistd.h>

#define SECS 1
static volatile int stop = 0;

struct stats { uint32_t slices; uint32_t tickets; };
static struct stats sA = {0, 1}, sB = {0, 10};

void *lotto_worker(void *arg) {
    struct stats *st = arg;
    while (!stop) { ++st->slices; my_thread_yield(); }
    return NULL;
}

int main(void) {
    my_thread_t tA, tB;
    my_thread_create(&tA, NULL, lotto_worker, &sA, SCHED_LOTTERY);
    my_thread_create(&tB, NULL, lotto_worker, &sB, SCHED_LOTTERY);

    /* dejar correr un segundo */
    usleep(SECS * 1000000);
    stop = 1;
    my_thread_join(&tA, NULL);
    my_thread_join(&tB, NULL);

    printf("1 ticket ⇒ %u slices | 10 tickets ⇒ %u slices\n",
           sA.slices, sB.slices);
    double ratio = (double)sB.slices / sA.slices;
    printf("ratio ≈ %.1f (ideal 10)\n", ratio);
    return 0;
}
