#ifndef MYPTHREAD_H
#define MYPTHREAD_H

#define _POSIX_C_SOURCE 200112L

#include <ucontext.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>   /* malloc/free, posix_memalign, rand */
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>

/* Tamaño de pila por defecto para cada hilo (1 MiB). */
#define MYTHREAD_DEFAULT_STACK  (1024 * 1024)

/* Adelanto del tipo de hilo para que queues.h lo reconozca. */
typedef struct my_thread my_thread_t;

/* -------------------- Definición de TCB (Thread Control Block) -------------------- */
typedef enum {
    T_READY,
    T_RUNNING,
    T_BLOCKED,
    T_ZOMBIE
} my_state_t;

typedef enum {
    SCHED_RR,
    SCHED_LOTTERY,
    SCHED_RT
} sched_type_t;

/* Estructura completa de un hilo en espacio de usuario */
struct my_thread {
    uint32_t           id;          /* ID interno de 0..MAX_THREADS-1 */
    ucontext_t         ctx;         /* Contexto de CPU (ucontext) */
    void              *retval;      /* Valor devuelto al hacer exit */
    my_state_t         state;       /* T_READY, T_RUNNING, T_BLOCKED, T_ZOMBIE */
    sched_type_t       sched;       /* Algoritmo de planificación actual */
    uint32_t           tickets;     /* N° de tiquetes (Lottery) */
    uint64_t           deadline;    /* Deadline en µs absolutos (RT) */
    void              *stack_base;  /* Puntero al bloque de pila (posix_memalign) */
    my_thread_t       *next;        /* Enlace para colas (schedulers, mutex) */
};

/* -------------------- Ahora que my_thread_t está definido, incluimos queues.h -------------------- */
#include "queues.h"

/* -------------------- Mutex recursivo ligero con lista FIFO -------------------- */
typedef struct {
    volatile int   lock;       /* 0 libre, 1 tomado */
    my_thread_t   *owner;      /* Puntero al TCB dueño o NULL si libre */
    uint32_t       recursion;  /* Contador de recursión (dueño) */
    my_queue_t     waiters;    /* Cola FIFO de hilos bloqueados esperando el mutex */
} my_mutex_t;

/* ============================ API de hilos ============================== */

/**
 * Crea un hilo en espacio de usuario:
 * - thread_out: si no es NULL, se copia *t* superficialmente a esa dirección (incluyendo `id`).
 * - attr_unused: atributos (no usados en esta versión).
 * - start_routine(arg): función a ejecutar en el hilo.
 * - sched: algoritmo inicial (SCHED_RR, SCHED_LOTTERY o SCHED_RT).
 *
 * Retorna 0 si todo bien, o ENOMEM, EAGAIN o algún errno en caso de falla.
 * Por defecto, el hilo principal y los hijos arrancan en Round-Robin (SCHED_RR)
 * si `sched` no coincide con las otras constantes de sched_type_t.
 */
int  my_thread_create(my_thread_t *thread_out,
                      const void  *attr_unused,
                      void *(*start_routine)(void *),
                      void *arg,
                      sched_type_t sched);

/**
 * Termina el hilo actual. Al volver, el scheduler marcará T_ZOMBIE y
 * liberará recursos cuando otro hilo haga join sobre él.
 * Esta función no retorna nunca (noreturn).
 */
void my_thread_exit(void *retval) __attribute__((noreturn));

/**
 * Espera a que `thread` alcance T_ZOMBIE. Si retval != NULL, ahí deja el valor
 * devuelto. Luego libera stack y el TCB real (no la copia de `thread`).
 * Retorna 0 o EDEADLK si haces join sobre ti mismo.
 */
int  my_thread_join(my_thread_t *thread, void **retval);

/**
 * Cede voluntariamente la CPU, forzando un SIGALRM para que el dispatcher
 * elija otro hilo.
 */
void my_thread_yield(void);

/**
 * Cambia el planificador del hilo actual a new_sched
 * (cuando vuelva a ejecutarse el scheduler, usará ese nuevo algoritmo).
 */
int  my_thread_chsched(sched_type_t new_sched);

/* ============================ API de mutex ============================== */

/**
 * Inicializa un mutex recursivo (lock=0, owner=NULL, recursion=0) y cola de waiters vacía.
 */
int  my_mutex_init(my_mutex_t *m);

/**
 * Bloquea el mutex:
 * - Si está libre, `g_current` se lo queda.
 * - Si dueño == `g_current`, se incrementa recursion.
 * - Si está tomado por otro, `g_current` pasa a T_BLOCKED y
 *   se encola en `m->waiters`, luego forzamos dispatch (raise(SIGALRM)).
 */
int  my_mutex_lock(my_mutex_t *m);

/**
 * Libera el mutex:
 * - Si recursion > 1, decrementa y retorna.
 * - Si recursion == 1, libera dueño y:
 *   * Si hay waiters, extrae el primero (FIFO), lo despierta (T_READY),
 *     lo convierte en nuevo dueño y marca lock=1.
 *   * Si no hay waiters, hace lock=0 y owner=NULL.
 */
int  my_mutex_unlock(my_mutex_t *m);

/**
 * Destruye el mutex (no hay memoria dinámica interna en esta implementación).
 */
int  my_mutex_destroy(my_mutex_t *m);

#endif /* MYPTHREAD_H */
