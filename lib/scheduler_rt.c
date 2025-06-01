#include "mypthread.h"

/*
 * Planificador Tiempo Real (EDF) usando un min-heap implementado en arreglo.
 * El campo `deadline` de cada TCB (en microsegundos absolutos) define la prioridad:
 * gana el hilo con deadline más pequeño.
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

#define MAX_RT_THREADS 1024
static my_thread_t *heap[MAX_RT_THREADS];
static size_t        heap_sz = 0;

/* Intercambia posiciones i y j en el arreglo `heap` */
static inline void heap_swap(size_t i, size_t j) {
    my_thread_t *tmp   = heap[i];
    heap[i]            = heap[j];
    heap[j]            = tmp;
}

/* Ajusta hacia arriba (min-heap) a partir de índice `idx` */
static void heapify_up(size_t idx) {
    while (idx > 0) {
        size_t parent = (idx - 1) / 2;
        if (heap[parent]->deadline <= heap[idx]->deadline) {
            break;
        }
        heap_swap(parent, idx);
        idx = parent;
    }
}

/* Ajusta hacia abajo (min-heap) a partir de índice `idx` */
static void heapify_down(size_t idx) {
    while (1) {
        size_t left  = 2*idx + 1;
        size_t right = 2*idx + 2;
        size_t smallest = idx;

        if (left < heap_sz && heap[left]->deadline < heap[smallest]->deadline) {
            smallest = left;
        }
        if (right < heap_sz && heap[right]->deadline < heap[smallest]->deadline) {
            smallest = right;
        }
        if (smallest == idx) {
            break;
        }
        heap_swap(idx, smallest);
        idx = smallest;
    }
}

void rt_init(void) {
    block_timer_signal();
    heap_sz = 0;
    unblock_timer_signal();
}

/**
 * Agrega `t` al min-heap (ordenado por `deadline`).
 * Si el heap ya está lleno, simplemente no lo agrega.
 */
void rt_add(my_thread_t *t) {
    if (!t) return;

    block_timer_signal();
    if (heap_sz >= MAX_RT_THREADS) {
        /* No hay espacio: omitimos el hilo */
        unblock_timer_signal();
        return;
    }
    heap[heap_sz] = t;
    heap_sz++;
    heapify_up(heap_sz - 1);
    t->next = NULL;  /* limpiamos campo `next` para no interferir con otros usos */
    unblock_timer_signal();
}

/**
 * Extrae y retorna el hilo con deadline más pequeño.
 * Retorna NULL si el heap está vacío.
 */
my_thread_t *rt_pick_next(void) {
    block_timer_signal();
    if (heap_sz == 0) {
        unblock_timer_signal();
        return NULL;
    }
    my_thread_t *t = heap[0];
    heap_sz--;
    if (heap_sz > 0) {
        heap[0] = heap[heap_sz];
        heapify_down(0);
    }
    t->next = NULL;
    unblock_timer_signal();
    return t;
}

/**
 * Reencola `current` (que estaba RUNNING) en función de su deadline.
 */
void rt_yield_current(my_thread_t *current) {
    if (!current || current->state != T_RUNNING) return;

    block_timer_signal();
    current->state = T_READY;
    /* Quitar si ya está en el heap */
    {
        size_t i;
        for (i = 0; i < heap_sz; i++) {
            if (heap[i] == current) {
                heap_sz--;
                heap[i] = heap[heap_sz];
                heapify_down(i);
                break;
            }
        }
    }
    /* Reinserción */
    heap[heap_sz] = current;
    heap_sz++;
    heapify_up(heap_sz - 1);
    current->next = NULL;
    unblock_timer_signal();
}

/**
 * Extrae a `t` del min-heap (si existe), para poder re-asignar su política.
 */
void rt_remove(my_thread_t *t) {
    if (!t || heap_sz == 0) return;

    block_timer_signal();
    size_t i;
    for (i = 0; i < heap_sz; i++) {
        if (heap[i] == t) {
            heap_sz--;
            heap[i] = heap[heap_sz];
            heapify_down(i);
            break;
        }
    }
    unblock_timer_signal();
}
