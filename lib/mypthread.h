/* =========================================================================
 * @file    mypthread.h
 * @brief   Reimplementación mínima de la API Pthreads para el Proyecto 1 –
 *          MultiDisplay Animator.
 *          Provee primitivas de manejo de hilos y sincronización totalmente
 *          en espacio de usuario, soportando tres planificadores: Round‑Robin,
 *          Sorteo (Lottery) y Tiempo Real (EDF simplificado).
 *
 * 0 =infinito
 * @author  Valery Carvajal y Anthony Rojas
 * @date    29 may 2025
 * ========================================================================= */
#ifndef MYPTHREAD_H
#define MYPTHREAD_H

#include <ucontext.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <signal.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =========================== Parámetros globales ======================== */
/** Longitud de pila por defecto para cada hilo (bytes) */
#define MYTHREAD_DEFAULT_STACK   (64 * 1024)

/* ============================= Tipos básicos ============================ */
/** Estados posibles de un hilo */
typedef enum {
    T_READY,
    T_RUNNING,
    T_BLOCKED,
    T_ZOMBIE
} my_state_t;

/** Planificadores soportados */
typedef enum {
    SCHED_RR,       /**< Round‑Robin (quantum fijo) */
    SCHED_LOTTERY,  /**< Sorteo (Lottery Scheduling) */
    SCHED_RT        /**< Tiempo Real (EDF simplificado) */
} sched_type_t;

/** Descriptor de hilo (equivalente a pthread_t) */
typedef struct my_thread {
    uint32_t           id;          /**< Identificador interno */
    ucontext_t         ctx;         /**< Contexto de ejecución */
    void              *retval;      /**< Valor devuelto por el hilo */
    my_state_t         state;       /**< Estado actual */
    sched_type_t       sched;       /**< Planificador asignado */
    uint32_t           tickets;     /**< N° de tiquetes (LOTTERY) */
    uint64_t           deadline;    /**< Deadline absoluto (RT) */
    struct my_thread  *next;        /**< Enlaces en colas */
} my_thread_t;

/** Mutex ligero (spin‑lock con soporte de recursión) */
typedef struct {
    volatile int lock;      /**< 0 = libre, 1 = ocupado */
    my_thread_t *owner;     /**< Dueño actual */
    uint32_t     recursion; /**< Nivel de recursión */
} my_mutex_t;

/* ============================ API de hilos ============================== */
int  my_thread_create(my_thread_t *thread,
                      const void  *attr_unused,
                      void *(*start_routine)(void *),
                      void *arg,
                      sched_type_t sched);

void my_thread_exit(void *retval) __attribute__((noreturn));

int  my_thread_join(my_thread_t *thread, void **retval);

void my_thread_yield(void);

int  my_thread_chsched(sched_type_t new_sched);

/* ============================ API de mutex ============================== */
int  my_mutex_init(my_mutex_t *m);
int  my_mutex_lock(my_mutex_t *m);
int  my_mutex_unlock(my_mutex_t *m);
int  my_mutex_destroy(my_mutex_t *m);

#ifdef __cplusplus
}
#endif

#endif /* MYPTHREAD_H */
