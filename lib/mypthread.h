#ifndef MYPTHREAD_H
#define MYPTHREAD_H

#include <ucontext.h>

#define STACK_SIZE 8192

/* --- Tipos de scheduler disponibles --- */
#define SCHED_RR       0    // Round‐Robin
#define SCHED_LOTTERY  1    // Lottery Scheduling
#define SCHED_RT       2    // Real‐Time Scheduling

typedef struct my_thread {
    /* Contexto de ejecución */
    ucontext_t context;

    /* Flags de ciclo de vida */
    int finished;         // 1 si el hilo ha terminado
    int detached;         // 1 si se marcó como detached

    /* Lista de hilos que esperan un join sobre este */
    struct my_thread *next;           // para “waiting_list”
    struct my_thread *waiting_list;   // punteros a los que hacen join(this)

    /* --- Campos para scheduler --- */
    int sched_type;       // SCHED_RR, SCHED_LOTTERY o SCHED_RT
    int tickets;          // (Solo para Lottery) Número de boletos
    int rt_priority;      // (Solo para Real‐Time) Prioridad estática

    /* --- Enlaces para listas circulares por scheduler --- */
    struct my_thread *rr_next, *rr_prev;          // para Round‐Robin
    struct my_thread *lottery_next, *lottery_prev; // para Lottery
    struct my_thread *rt_next, *rt_prev;           // para Real‐Time
} my_thread_t;

/* Variables globales (definidas en mypthread.c) */
extern my_thread_t *current_thread;
extern my_thread_t *main_thread;

/* ==== Exponemos los “heads” de cada lista para que test.c pueda verlos ==== */
extern my_thread_t *rr_head;
extern my_thread_t *lottery_head;
extern my_thread_t *rt_head;

/* =============== Interfaz Pública =============== */

/*
 * my_thread_create:
 *   - thread: salida, puntero a la estructura nueva.
 *   - start_routine: función void(void) que ejecutará el hilo.
 *   - sched_type: SCHED_RR, SCHED_LOTTERY o SCHED_RT.
 *   - attr:
 *       * si SCHED_LOTTERY → número de tickets (>0).
 *       * si SCHED_RT     → rt_priority (>=0).
 *       * si SCHED_RR     → atributo ignorado (0).
 *
 * Retorna: 0 en éxito, -1 en error.
 */
int my_thread_create(my_thread_t **thread,
                     void (*start_routine)(void),
                     int sched_type,
                     int attr);

/*
 * my_thread_chsched:
 *   Cambia el scheduler de ‘target’ a new_sched, con parámetro new_attr.
 *   Retorna 0 en éxito, -1 en error.
 */
int my_thread_chsched(my_thread_t *target,
                      int new_sched,
                      int new_attr);

/*
 * Ceder voluntariamente (o preemptivo si llegó SIGALRM) el CPU al siguiente hilo
 * según la política del hilo actual.
 */
void my_thread_yield(void);

/*
 * Marcar el hilo actual como terminado:
 *   - Removerlo de su lista de scheduler.
 *   - Reinyectar (en su política) a quienes hicieron join sobre él.
 *   - Si estaba detached, liberar pila y estructura.
 *   - Despachar el siguiente hilo listo (RR>Lottery>RT). Si no quedan, exit(0).
 */
void my_thread_end(void);

/*
 * Esperar a que ‘target’ termine. Si ya terminó, retorna 0. Si no, bloquea el hilo
 * actual, hace swapcontext al siguiente listo. Cuando target termine, despertará
 * a este hilo y retornará 0. Retorna -1 si target == NULL, target == current_thread
 * o target estaba detached.
 */
int my_thread_join(my_thread_t *target);

/*
 * Marcar ‘target’ como detached. Cuando target termine, liberará sus recursos.
 * Retorna 0 en éxito, -1 si target == NULL o ya estaba detached.
 */
int my_thread_detach(my_thread_t *target);

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
/*
 * init_timer:
 *   Configura SIGALRM para invocar a my_thread_yield() cada 100 ms.
 */
void init_timer(void);

#endif // MYPTHREAD_H
