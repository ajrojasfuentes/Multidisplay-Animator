#include "mypthread.h"

/*
 * Planificador Round-Robin.
 * Usa una cola FIFO sencilla para hilos en estado READY.
 */

/* Bloquea SIGALRM */
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

/* Variables estáticas (cola FIFO) */
static my_thread_t *rr_head = NULL;
static my_thread_t *rr_tail = NULL;

/**
 * Inicializa la cola RR en vacío.
 */
void rr_init(void) {
    block_timer_signal();
    rr_head = rr_tail = NULL;
    unblock_timer_signal();
}

/**
 * Inserta `t` al final de la cola RR.
 * Si ya existía, lo remueve primero para evitar duplicados.
 */
void rr_add(my_thread_t *t) {
    if (!t) return;

    block_timer_signal();
    /* Remover si ya estaba presente */
    if (rr_head) {
        my_thread_t *prev = NULL, *cur = rr_head;
        while (cur) {
            if (cur == t) {
                if (!prev) {
                    rr_head = cur->next;
                } else {
                    prev->next = cur->next;
                }
                if (rr_tail == cur) {
                    rr_tail = prev;
                }
                break;
            }
            prev = cur;
            cur = cur->next;
        }
    }

    /* Insertar al final */
    t->next = NULL;
    if (!rr_head) {
        rr_head = rr_tail = t;
    } else {
        rr_tail->next = t;
        rr_tail = t;
    }
    unblock_timer_signal();
}

/**
 * Extrae y retorna el primer hilo de la cola RR (o NULL si está vacía).
 */
my_thread_t *rr_pick_next(void) {
    block_timer_signal();
    my_thread_t *t = rr_head;
    if (t) {
        rr_head = t->next;
        if (!rr_head) {
            rr_tail = NULL;
        }
        t->next = NULL;
    }
    unblock_timer_signal();
    return t;
}

/**
 * Cuando un hilo que estaba RUNNING pierde su quantum o hace yield,
 * se marca T_READY y se reencola al final.
 */
void rr_yield_current(my_thread_t *current) {
    if (!current || current->state != T_RUNNING) return;

    block_timer_signal();
    current->state = T_READY;
    /* Lo mismo que rr_add: remover si existía, luego encolar */
    if (rr_head) {
        my_thread_t *prev = NULL, *cur = rr_head;
        while (cur) {
            if (cur == current) {
                if (!prev) {
                    rr_head = cur->next;
                } else {
                    prev->next = cur->next;
                }
                if (rr_tail == cur) {
                    rr_tail = prev;
                }
                break;
            }
            prev = cur;
            cur = cur->next;
        }
    }
    current->next = NULL;
    if (!rr_head) {
        rr_head = rr_tail = current;
    } else {
        rr_tail->next = current;
        rr_tail = current;
    }
    unblock_timer_signal();
}

/**
 * Elimina `t` de la cola RR (si estaba), para permitir reasignar política.
 */
void rr_remove(my_thread_t *t) {
    if (!t || !rr_head) return;

    block_timer_signal();
    if (rr_head == t) {
        rr_head = t->next;
        if (!rr_head) rr_tail = NULL;
        t->next = NULL;
        unblock_timer_signal();
        return;
    }
    my_thread_t *prev = rr_head, *cur = rr_head->next;
    while (cur) {
        if (cur == t) {
            prev->next = cur->next;
            if (rr_tail == cur) {
                rr_tail = prev;
            }
            cur->next = NULL;
            break;
        }
        prev = cur;
        cur = cur->next;
    }
    unblock_timer_signal();
}
