/* =========================================================================
* @file    scheduler_lottery.c
 * @brief   Implementación de Lottery Scheduling para mypthreads.
 *          Cada hilo posee "tickets"; el scheduler selecciona uno al azar y
 *          entrega la CPU al hilo cuyo rango contiene el ticket ganador.
 * ========================================================================= */
#include "mypthread.h"
#include <stdlib.h>

/* Lista simplemente enlazada de hilos en LOTTERY */
static my_thread_t *lot_head = NULL;
static uint32_t     lot_total_tickets = 0;

static void lotto_add_internal(my_thread_t *t)
{
    t->next = lot_head;
    lot_head = t;
    lot_total_tickets += t->tickets;
}

/* ------------------------------------------------------------------------- */
void lottery_init(void)
{
    lot_head = NULL;
    lot_total_tickets = 0;
}

void lottery_add(my_thread_t *t)
{
    if (t->tickets == 0) t->tickets = 1;
    lotto_add_internal(t);
}

static my_thread_t *lottery_draw(void)
{
    if (!lot_head) return NULL;
    uint32_t winning = (uint32_t)(rand() % lot_total_tickets);
    uint32_t sum = 0;
    my_thread_t *prev = NULL, *iter = lot_head;
    while (iter) {
        sum += iter->tickets;
        if (sum > winning) {
            /* Quitar de la lista */
            if (prev) prev->next = iter->next; else lot_head = iter->next;
            lot_total_tickets -= iter->tickets;
            return iter;
        }
        prev = iter;
        iter = iter->next;
    }
    return NULL; /* debería no ocurrir */
}

my_thread_t *lottery_pick_next(void)
{
    my_thread_t *t = lottery_draw();
    return t;
}

void lottery_yield_current(my_thread_t *current)
{
    if (!current || current->state != T_RUNNING) return;
    current->state = T_READY;
    lottery_add(current);
}
