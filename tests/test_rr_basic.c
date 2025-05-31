#include "mypthread.h"
#include <stdio.h>

void *worker(void *arg) {
    char id = (char)(intptr_t)arg;
    for (int i = 0; i < 10; ++i) {
        printf("%c", id);
        fflush(stdout);
        my_thread_yield();          /* cede la CPU antes de imprimir de nuevo */
    }
    return NULL;
}

int main(void) {
    my_thread_t a, b;
    my_thread_create(&a, NULL, worker, (void *)'A', SCHED_RR);
    my_thread_create(&b, NULL, worker, (void *)'B', SCHED_RR);
    my_thread_join(&a, NULL);
    my_thread_join(&b, NULL);
    puts("\n[OK] RR alternó A y B");
    return 0;
}
