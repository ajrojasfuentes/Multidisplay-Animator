#ifndef MYPTHREAD_H
#define MYPTHREAD_H

#include <ucontext.h>

#define STACK_SIZE 8192

/* --- Tipos de scheduler disponibles --- */
#define SCHED_RR       0    // Round‐Robin
#define SCHED_LOTTERY  1    // Lottery Scheduling
#define SCHED_RT       2    // Real‐Time Scheduling

typedef struct my_thread {
    ucontext_t context;
    int finished;
    int detached;
    struct my_thread *next;           // para waiting_list
    struct my_thread *waiting_list;   // hilos que esperan este

    int sched_type;
    int tickets;          // solo para Lottery
    int rt_priority;      // solo para RT

    struct my_thread *rr_next, *rr_prev;
    struct my_thread *lottery_next, *lottery_prev;
    struct my_thread *rt_next, *rt_prev;
    void *arg;
} my_thread_t;

/* Variables globales (definidas en mypthread.c) */
extern my_thread_t *current_thread;
extern my_thread_t *main_thread;

/* Exponer los “heads” de cada lista para test */
extern my_thread_t *rr_head;
extern my_thread_t *lottery_head;
extern my_thread_t *rt_head;

/* =============== Interfaz de hilos =============== */
int my_thread_create(my_thread_t **thread,
                     void (*start_routine)(void),
                     int sched_type,
                     int attr);
int my_thread_chsched(my_thread_t *target,
                      int new_sched,
                      int new_attr);
void my_thread_yield(void);
void my_thread_end(void);
int my_thread_join(my_thread_t *target);
int my_thread_detach(my_thread_t *target);

/* =============== Interfaz de mutex en espacio de usuario =============== */
/*
 * Un mutex simple:
 *   - Si está libre, quien llame a lock lo adquiere.
 *   - Si está ocupado, el hilo actual se bloquea hasta que se llame unlock.
 */
typedef struct my_mutex {
    int locked;                  // 0 = libre, 1 = ocupado
    my_thread_t *waiting_head;   // lista FIFO de hilos bloqueados
    my_thread_t *waiting_tail;
} my_mutex_t;

/*
 * Inicializar el mutex: lo deja desbloqueado y sin lista de espera.
 * Retorna 0 en éxito, -1 si mutex == NULL.
 */
int my_mutex_init(my_mutex_t *mutex);

/*
 * Destruir el mutex: asume que está desbloqueado y sin hilos en espera.
 * Retorna 0 en éxito, -1 si hay hilos bloqueados o mutex == NULL.
 */
int my_mutex_destroy(my_mutex_t *mutex);

/*
 * Intentar adquirir el mutex.
 * - Si estaba libre, lo marca como “locked” y retorna 0.
 * - Si estaba ocupado, bloquea el hilo actual (lo pone en waiting_list)
 *   y hace swapcontext a otro hilo listo.
 * Retorna 0 en éxito, o -1 si mutex == NULL.
 */
int my_mutex_lock(my_mutex_t *mutex);

/*
 * Liberar el mutex: si hay hilos bloqueados, despierta al primero (FILO) y
 * lo rebota a su lista de scheduler; si no, simplemente marca locked = 0.
 * Retorna 0 en éxito, -1 si mutex == NULL o si no estaba locked.
 */
int my_mutex_unlock(my_mutex_t *mutex);

/*
 * Intentar “trylock” sin bloquear:
 * - Si estaba libre, lo marca como locked y retorna 0.
 * - Si estaba ocupado, retorna -1 sin bloquearse.
 */
int my_mutex_trylock(my_mutex_t *mutex);

/* =============== Funciones Internas de RR =============== */
void rr_init(void);
void rr_add(my_thread_t *t);
my_thread_t *rr_pick_next(void);
void rr_yield_current(void);
void rr_remove(my_thread_t *t);

/* =============== Funciones Internas de Lottery =============== */
void lottery_init(void);
void lottery_add(my_thread_t *t);
my_thread_t *lottery_pick_next(void);
void lottery_yield_current(void);
void lottery_remove(my_thread_t *t);

/* =============== Funciones Internas de Real‐Time =============== */
void rt_init(void);
void rt_add(my_thread_t *t);
my_thread_t *rt_pick_next(void);
void rt_yield_current(void);
void rt_remove(my_thread_t *t);

/* =============== Temporizador =============== */
void init_timer(void);

#endif // MYPTHREAD_H
