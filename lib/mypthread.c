/* =========================================================================
 * @file    mypthread.c
 * @brief   Implementación en espacio de usuario de una API mínima tipo
 *          Pthreads con planificador Round‑Robin preemptivo (fase 1).
 *
 *          Fases futuras añadirán Lottery y Tiempo Real.
 *
 * @author  Valery Carvajal y Anthony Rojas
 * @date    29 may 2025
 * ========================================================================= */
#include "mypthread.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

/* ============================= Constantes =============================== */
#define MAX_THREADS         1024          /* límite arbitrario */
#define QUANTUM_USEC        10000         /* 10 ms */

/* ========================= Estructuras internas ========================= */
static my_thread_t       *g_threads[MAX_THREADS];  /* tabla global de TCB */
static uint32_t           g_next_id = 0;
static my_thread_t       *g_current = NULL;        /* hilo en CPU */

/* Cola simple enlazada para RR */
static my_thread_t *g_ready_head = NULL;
static my_thread_t *g_ready_tail = NULL;

/* Forward ‑ prototypes internos */
static void scheduler_init(void);
static void enqueue_ready(my_thread_t *t);
static my_thread_t *dequeue_ready(void);
static void dispatch(int sig);
static void thread_trampoline(void *(*start_routine)(void *), void *arg);

/* ========================= Utilidades atómicas ========================== */
static inline int xchg(volatile int *ptr, int val)
{
    return __sync_lock_test_and_set(ptr, val);
}

/* ========================= Inicialización global ======================== */
__attribute__((constructor))
static void runtime_bootstrap(void)
{
    /* Crear TCB para el hilo principal */
    my_thread_t *main_t = calloc(1, sizeof(*main_t));
    if (!main_t) { perror("calloc"); exit(EXIT_FAILURE); }
    main_t->id    = g_next_id++;
    main_t->state = T_RUNNING;
    main_t->sched = SCHED_RR;
    g_current     = main_t;
    g_threads[main_t->id] = main_t;

    /* Configurar SIGALRM para RR */
    scheduler_init();
}

static void scheduler_init(void)
{
    struct sigaction sa = {0};
    sa.sa_handler = dispatch;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags   = SA_NODEFER;
    if (sigaction(SIGALRM, &sa, NULL) != 0) {
        perror("sigaction"); exit(EXIT_FAILURE);
    }

    struct itimerval tv;
    tv.it_interval.tv_sec  = 0;
    tv.it_interval.tv_usec = QUANTUM_USEC;
    tv.it_value = tv.it_interval; /* arranca de inmediato */
    if (setitimer(ITIMER_REAL, &tv, NULL) != 0) {
        perror("setitimer"); exit(EXIT_FAILURE);
    }
}

/* ====================== Funciones de colas READY ======================= */
static void enqueue_ready(my_thread_t *t)
{
    t->next = NULL;
    if (!g_ready_head) {
        g_ready_head = g_ready_tail = t;
    } else {
        g_ready_tail->next = t;
        g_ready_tail = t;
    }
}

static my_thread_t *dequeue_ready(void)
{
    my_thread_t *t = g_ready_head;
    if (t) {
        g_ready_head = t->next;
        if (!g_ready_head) g_ready_tail = NULL;
    }
    return t;
}

/* ========================= Cambio de contexto =========================== */
static void switch_to(my_thread_t *next)
{
    my_thread_t *prev = g_current;
    g_current = next;
    next->state = T_RUNNING;
    if (swapcontext(&prev->ctx, &next->ctx) == -1) {
        perror("swapcontext"); exit(EXIT_FAILURE);
    }
}

/* ============================ Dispatcher ================================ */
static void dispatch(int sig)
{
    (void)sig;

    /* No cambiar si solo hay un hilo */
    if (!g_ready_head) return;

    my_thread_t *prev = g_current;

    if (prev->state == T_RUNNING) {
        prev->state = T_READY;
        enqueue_ready(prev);
    }

    my_thread_t *next = dequeue_ready();
    if (!next) return; /* nada disponible */

    switch_to(next);
}

/* ========================== Trampolín de hilos ========================== */
static void thread_trampoline(void *(*start_routine)(void *), void *arg)
{
    void *ret = start_routine(arg);
    my_thread_exit(ret);
}

/* =============================== API =================================== */
int my_thread_create(my_thread_t *thread,
                     const void *attr_unused,
                     void *(*start_routine)(void *),
                     void *arg,
                     sched_type_t sched)
{
    (void)attr_unused; /* atributos no utilizados en esta versión */

    if (g_next_id >= MAX_THREADS) return EAGAIN;

    my_thread_t *t = calloc(1, sizeof(*t));
    if (!t) return ENOMEM;

    t->id    = g_next_id++;
    t->state = T_READY;
    t->sched = sched;
    t->tickets = 1; /* default */

    /* Reserva de pila */
    size_t stack_sz = MYTHREAD_DEFAULT_STACK;
    void *stack = NULL;
    if (posix_memalign(&stack, 16, stack_sz) != 0) {
        free(t); return ENOMEM;
    }

    /* Contexto */
    if (getcontext(&t->ctx) == -1) { free(stack); free(t); return errno; }
    t->ctx.uc_stack.ss_sp   = stack;
    t->ctx.uc_stack.ss_size = stack_sz;
    t->ctx.uc_link          = NULL;
    makecontext(&t->ctx, (void (*)(void))thread_trampoline, 2,
                start_routine, arg);

    /* Registrar y encolar */
    g_threads[t->id] = t;
    enqueue_ready(t);

    if (thread) *thread = *t; /* copiar descriptor externo */

    return 0;
}

void my_thread_exit(void *retval)
{
    g_current->retval = retval;
    g_current->state  = T_ZOMBIE;

    /* Despertar posibles joiners… (simplificado: busy‑wait en join) */

    /* Pasar control al siguiente listo */
    my_thread_t *next = dequeue_ready();
    if (!next) {
        /* Último hilo → terminar proceso */
        exit(0);
    }
    switch_to(next);

    __builtin_unreachable();
}

int my_thread_join(my_thread_t *thread, void **retval)
{
    while (thread->state != T_ZOMBIE) {
        /* Yield explícito para evitar busy CPU */
        my_thread_yield();
    }
    if (retval) *retval = thread->retval;
    return 0;
}

void my_thread_yield(void)
{
    raise(SIGALRM); /* fuerza al dispatcher */
}

int my_thread_chsched(sched_type_t new_sched)
{
    g_current->sched = new_sched;
    return 0; /* TODO: mover entre colas específicas en fases futuras */
}

/* ============================ Mutex simple ============================== */
int my_mutex_init(my_mutex_t *m)
{
    m->lock = 0;
    m->owner = NULL;
    m->recursion = 0;
    return 0;
}

int my_mutex_lock(my_mutex_t *m)
{
    my_thread_t *self = g_current;
    if (m->owner == self) {
        m->recursion++; return 0; /* recursivo */
    }
    while (xchg(&m->lock, 1)) {
        /* Bloquear hilo: ponerlo en BLOCKED y despachar */
        self->state = T_BLOCKED;
        raise(SIGALRM);
    }
    m->owner = self;
    m->recursion = 1;
    return 0;
}

int my_mutex_unlock(my_mutex_t *m)
{
    if (m->owner != g_current) return EPERM;
    if (--m->recursion == 0) {
        m->owner = NULL;
        xchg(&m->lock, 0);
    }
    return 0;
}

int my_mutex_destroy(my_mutex_t *m)
{
    (void)m; return 0; /* nada que liberar */
}
