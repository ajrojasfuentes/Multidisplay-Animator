/* =========================================================================
 * @file    queues.c
 * @brief   Utilidades de colas simples para mypthreads.
 *          Implementa cola FIFO basada en el campo `next` del TCB.
 * ========================================================================= */
#include "mypthread.h"
#include <stdbool.h>

/* -------------------------------------------------------------------------
 * Estructura de cola – declarada aquí para evitar header extra; los módulos
 * que la usen simplemente hacen `extern`.
 * ------------------------------------------------------------------------- */
typedef struct my_queue {
    my_thread_t *head;
    my_thread_t *tail;
} my_queue_t;

/* -------------------------------------------------------------------------
 * queue_init – deja la cola vacía.
 * ------------------------------------------------------------------------- */
void queue_init(my_queue_t *q)
{
    q->head = q->tail = NULL;
}

/* -------------------------------------------------------------------------
 * queue_is_empty – ¿cola vacía?
 * ------------------------------------------------------------------------- */
bool queue_is_empty(const my_queue_t *q)
{
    return q->head == NULL;
}

/* -------------------------------------------------------------------------
 * queue_push – inserta hilo al final (FIFO).
 * ------------------------------------------------------------------------- */
void queue_push(my_queue_t *q, my_thread_t *t)
{
    t->next = NULL;
    if (!q->head) {
        q->head = q->tail = t;
    } else {
        q->tail->next = t;
        q->tail = t;
    }
}

/* -------------------------------------------------------------------------
 * queue_pop – extrae y devuelve el primer hilo o NULL.
 * ------------------------------------------------------------------------- */
my_thread_t *queue_pop(my_queue_t *q)
{
    my_thread_t *t = q->head;
    if (t) {
        q->head = t->next;
        if (!q->head) q->tail = NULL;
    }
    return t;
}

/* -------------------------------------------------------------------------
 * queue_remove – elimina un elemento intermedio. O(n).
 * ------------------------------------------------------------------------- */
void queue_remove(my_queue_t *q, my_thread_t *t)
{
    if (!q->head || !t) return;
    if (q->head == t) {
        queue_pop(q);
        return;
    }
    my_thread_t *prev = q->head;
    while (prev->next && prev->next != t) prev = prev->next;
    if (prev->next == t) {
        prev->next = t->next;
        if (q->tail == t) q->tail = prev;
    }
}
