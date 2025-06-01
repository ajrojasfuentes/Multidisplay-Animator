#include "mypthread.h"
#include <stdlib.h>    /* rand() */

/*
 * Planificador Lottery: cada hilo lleva un número de tickets.
 * Se almacena en una lista enlazada simple.
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

/* Nodo de lista: usamos el campo `next` de my_thread_t */
static my_thread_t *lot_head = NULL;
static uint32_t     lot_total_tickets = 0;

/* Agrega `t` al frente de la lista y ajusta total de tickets */
static void lotto_add_internal(my_thread_t *t) {
    t->next = lot_head;
    lot_head = t;
    lot_total_tickets += t->tickets;
}

void lottery_init(void) {
    block_timer_signal();
    lot_head = NULL;
    lot_total_tickets = 0;
    unblock_timer_signal();
}

/**
 * Inserta `t` en la lista de Lottery.
 * Si `t` ya estaba, se remueve primero (para evitar duplicados en la lotería).
 */
void lottery_add(my_thread_t *t) {
    if (!t) return;

    block_timer_signal();
    /* Eliminar duplicados si `t` ya existe */
    my_thread_t *prev = NULL, *cur = lot_head;
    while (cur) {
        if (cur == t) {
            /* Quitar cur */
            if (!prev) {
                lot_head = cur->next;
            } else {
                prev->next = cur->next;
            }
            lot_total_tickets -= cur->tickets;
            break;
        }
        prev = cur;
        cur = cur->next;
    }
    /* Insertar al frente */
    lotto_add_internal(t);
    unblock_timer_signal();
}

/**
 * Realiza el sorteo y devuelve el hilo ganador, removiéndolo de la lista.
 * Retorna NULL si no hay hilos.
 */
static my_thread_t *lottery_draw(void) {
    if (!lot_head || lot_total_tickets == 0) {
        return NULL;
    }
    /* Sorteo en [1..lot_total_tickets] */
    uint32_t r = (rand() % lot_total_tickets) + 1;
    uint32_t cumulative = 0;
    my_thread_t *prev = NULL, *curr = lot_head;
    while (curr) {
        cumulative += curr->tickets;
        if (cumulative >= r) {
            /* `curr` es el ganador: sacarlo de la lista */
            if (prev) {
                prev->next = curr->next;
            } else {
                lot_head = curr->next;
            }
            lot_total_tickets -= curr->tickets;
            curr->next = NULL;
            return curr;
        }
        prev = curr;
        curr = curr->next;
    }
    return NULL; /* no debería llegar aquí */
}

my_thread_t *lottery_pick_next(void) {
    block_timer_signal();
    my_thread_t *winner = lottery_draw();
    unblock_timer_signal();
    return winner;
}

/**
 * Cuando el hilo actual pierde CPU o hace yield, se marca READY y se re-en­cola.
 */
void lottery_yield_current(my_thread_t *current) {
    if (!current || current->state != T_RUNNING) return;

    block_timer_signal();
    current->state = T_READY;
    /* Re-encolar en Lottery */
    /* Primero quitar duplicado si existía */
    my_thread_t *prev = NULL, *cur = lot_head;
    while (cur) {
        if (cur == current) {
            if (!prev) {
                lot_head = cur->next;
            } else {
                prev->next = cur->next;
            }
            lot_total_tickets -= cur->tickets;
            break;
        }
        prev = cur;
        cur = cur->next;
    }
    lotto_add_internal(current);
    unblock_timer_signal();
}

/**
 * Elimina `t` de la lista de Lottery (si está) para permitir reasignar política.
 */
void lottery_remove(my_thread_t *t) {
    if (!t || !lot_head) return;

    block_timer_signal();
    my_thread_t *prev = NULL, *cur = lot_head;
    while (cur) {
        if (cur == t) {
            if (!prev) {
                lot_head = cur->next;
            } else {
                prev->next = cur->next;
            }
            lot_total_tickets -= cur->tickets;
            t->next = NULL;
            break;
        }
        prev = cur;
        cur = cur->next;
    }
    unblock_timer_signal();
}
