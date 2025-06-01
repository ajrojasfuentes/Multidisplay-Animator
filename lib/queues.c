#include "queues.h"
#include "mypthread.h"  /* para saber qué es my_thread_t */
#include <stdlib.h>     /* para NULL */

/* Bloquea SIGALRM (temporizador) */
static void block_timer_signal(void) {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGALRM);
    sigprocmask(SIG_BLOCK, &mask, NULL);
}

/* Desbloquea SIGALRM */
static void unblock_timer_signal(void) {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGALRM);
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

void queue_init(my_queue_t *q) {
    q->head = q->tail = NULL;
}

bool queue_is_empty(const my_queue_t *q) {
    return (q->head == NULL);
}

void queue_push(my_queue_t *q, my_thread_t *t) {
    if (!q || !t) return;

    block_timer_signal();
    /* Si `t` ya está en la cola, lo quitamos antes */
    queue_remove(q, t);

    /* Insertar al final */
    t->next = NULL;
    if (!q->head) {
        q->head = q->tail = t;
    } else {
        q->tail->next = t;
        q->tail = t;
    }
    unblock_timer_signal();
}

my_thread_t *queue_pop(my_queue_t *q) {
    if (!q) return NULL;

    block_timer_signal();
    my_thread_t *t = q->head;
    if (t) {
        q->head = t->next;
        if (!q->head) {
            q->tail = NULL;
        }
        t->next = NULL;
    }
    unblock_timer_signal();
    return t;
}

void queue_remove(my_queue_t *q, my_thread_t *t) {
    if (!q || !t || !q->head) return;

    block_timer_signal();
    /* Si `t` es la cabeza */
    if (q->head == t) {
        q->head = t->next;
        if (!q->head) {
            q->tail = NULL;
        }
        t->next = NULL;
        unblock_timer_signal();
        return;
    }

    /* Buscar en la lista */
    my_thread_t *prev = q->head;
    while (prev->next && prev->next != t) {
        prev = prev->next;
    }
    if (prev->next == t) {
        prev->next = t->next;
        if (q->tail == t) {
            q->tail = prev;
        }
        t->next = NULL;
    }
    unblock_timer_signal();
}
