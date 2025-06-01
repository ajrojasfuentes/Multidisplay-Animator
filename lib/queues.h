#ifndef QUEUES_H
#define QUEUES_H

#include <stdbool.h>  /* para el tipo bool */

/* Adelanto de tipo para que funcione con my_thread_t */
typedef struct my_thread my_thread_t;

/**
 * Cola FIFO que usa el campo `next` de cada my_thread_t.
 */
typedef struct {
    my_thread_t *head;
    my_thread_t *tail;
} my_queue_t;

/**
 * Inicializa la cola (head y tail en NULL).
 */
void queue_init(my_queue_t *q);

/**
 * Retorna true si la cola está vacía.
 */
bool queue_is_empty(const my_queue_t *q);

/**
 * Encola el hilo `t` al final. No permite duplicados: si `t` ya existía, lo remueve primero.
 */
void queue_push(my_queue_t *q, my_thread_t *t);

/**
 * Extrae y retorna el primer hilo de la cola `q`, o NULL si está vacía.
 */
my_thread_t *queue_pop(my_queue_t *q);

/**
 * Si `t` está en la cola `q`, lo elimina (O(n)). Si no está, no hace nada.
 */
void queue_remove(my_queue_t *q, my_thread_t *t);

#endif /* QUEUES_H */
