/* =========================================================================
* @file    scheduler_rr.c
 * @brief   Planificador Round-Robin clásico para mypthreads.
 *          Cada hilo obtiene un quantum fijo y se encola al final al expirar.
 * ========================================================================= */
#include "mypthread.h"

/* Cola enlazada de hilos listos (solo SCHED_RR) */
static my_thread_t *rr_head = NULL;
static my_thread_t *rr_tail = NULL;

/* -------------------------------------------------------------------------
 * rr_init – vacía la cola (usar al arrancar o al resetear pruebas)
 * ------------------------------------------------------------------------- */
void rr_init(void)
{
    rr_head = rr_tail = NULL;
}

/* -------------------------------------------------------------------------
 * rr_add – inserta hilo t al final de la cola RR
 * ------------------------------------------------------------------------- */
void rr_add(my_thread_t *t)
{
    t->next = NULL;
    if (!rr_head) {
        rr_head = rr_tail = t;
    } else {
        rr_tail->next = t;
        rr_tail = t;
    }
}

/* -------------------------------------------------------------------------
 * rr_pick_next – obtiene y retira el siguiente hilo listo o NULL.
 * ------------------------------------------------------------------------- */
my_thread_t *rr_pick_next(void)
{
    my_thread_t *t = rr_head;
    if (!t) return NULL;

    rr_head = t->next;
    if (!rr_head) rr_tail = NULL;
    return t;
}

/* -------------------------------------------------------------------------
 * rr_yield_current – mueve el hilo actual al final de la cola (quantum).
 * ------------------------------------------------------------------------- */
void rr_yield_current(my_thread_t *current)
{
    if (!current || current->state != T_RUNNING) return;
    current->state = T_READY;
    rr_add(current);
}
