#include "mypthread.h"
#include <stdio.h>

void *chameleon(void *unused) {
    for (int i = 0; i < 6; ++i) {
        printf(\"[%d] running\\n\", i);
        if (i == 2) my_thread_chsched(SCHED_LOTTERY);
        if (i == 4) my_thread_chsched(SCHED_RT);
        my_thread_yield();
    }
    return NULL;
}

int main(void) {
    my_thread_t t;
    my_thread_create(&t, NULL, chameleon, NULL, SCHED_RR);
    my_thread_join(&t, NULL);
    puts(\"[OK] Cambio de scheduler ejecutado\");
    return 0;
}
