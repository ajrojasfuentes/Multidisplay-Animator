//
// Created by ajrf on 5/29/25.
//
/* =========================================================================
 * @file    scheduler_rt.c
 * @brief   Planificador Tiempo Real (EDF simplificado) para mypthreads.
 *          Selecciona siempre el hilo con deadline absoluto más cercano.
 * ========================================================================= */
#include "mypthread.h"
#include <stdlib.h>

/* ---------------- Min‑heap de hilos ordenado por deadline --------------- */
#define MAX_RT_THREADS 1024
static my_thread_t *heap[MAX_RT_THREADS];
static size_t        heap_sz = 0;

static inline void heap_swap(size_t i, size_t j)
{
    my_thread_t *tmp = heap[i];
    heap[i] = heap[j];
    heap[j] = tmp;
}

static void heapify_up(size_t idx)
{
    while (idx && heap[idx]->deadline < heap[(idx - 1) / 2]->deadline) {
        size_t parent = (idx - 1) / 2;
        heap_swap(idx, parent);
        idx = parent;
    }
}

static void heapify_down(size_t idx)
{
    while (1) {
        size_t l = idx * 2 + 1, r = idx * 2 + 2, smallest = idx;
        if (l < heap_sz && heap[l]->deadline < heap[smallest]->deadline) smallest = l;
        if (r < heap_sz && heap[r]->deadline < heap[smallest]->deadline) smallest = r;
        if (smallest == idx) break;
        heap_swap(idx, smallest);
        idx = smallest;
    }
}

/* ------------------------------------------------------------------------- */
void rt_init(void)
{
    heap_sz = 0;
}

void rt_add(my_thread_t *t)
{
    if (heap_sz >= MAX_RT_THREADS) return; /* descartar en overflow */
    heap[heap_sz] = t;
    heapify_up(heap_sz);
    heap_sz++;
}

my_thread_t *rt_pick_next(void)
{
    if (!heap_sz) return NULL;
    my_thread_t *t = heap[0];
    heap[0] = heap[--heap_sz];
    heapify_down(0);
    return t;
}

void rt_yield_current(my_thread_t *current)
{
    if (!current || current->state != T_RUNNING) return;
    current->state = T_READY;
    rt_add(current);
}
