/* =========================================================================
 * @file    mypthread.c
 * @brief   Reimplementación completa en espacio de usuario de una API mínima tipo Pthreads
 *          con soporte simultáneo de tres planificadores: Round-Robin, Lottery
 *          y Tiempo Real (EDF). Este módulo concentra el cambio de contexto
 *          y delega la política de selección a los módulos scheduler_*.c
 *
 * @author  Valery Carvajal y Anthony Rojas
 * @date    30 may 2025
 * ========================================================================= */
#define _POSIX_C_SOURCE 200112L

#include "mypthread.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

/* ===================== Prototipos de schedulers ===================== */
void rr_init(void);
void rr_add(my_thread_t *t);
my_thread_t *rr_pick_next(void);
void rr_yield_current(my_thread_t *current);
void rr_remove(my_thread_t *t);

void lottery_init(void);
void lottery_add(my_thread_t *t);
my_thread_t *lottery_pick_next(void);
void lottery_yield_current(my_thread_t *current);
void lottery_remove(my_thread_t *t);

void rt_init(void);
void rt_add(my_thread_t *t);
my_thread_t *rt_pick_next(void);
void rt_yield_current(my_thread_t *current);
void rt_remove(my_thread_t *t);

/* ======================== Parámetros globales ========================= */
#define MAX_THREADS  1024
#define QUANTUM_USEC 10000  /* 10 ms de quantum RR */

static my_thread_t *g_threads[MAX_THREADS] = {0};
static uint32_t     g_next_id = 0;
static my_thread_t *g_current = NULL;

/* ====================== Helpers de señal ====================== */
/* Máscara que contiene solo SIGALRM */
static sigset_t timer_mask;

/* Bloquea SIGALRM */
static void block_timer_signal(void) {
    sigprocmask(SIG_BLOCK, &timer_mask, NULL);
}

/* Desbloquea SIGALRM */
static void unblock_timer_signal(void) {
    sigprocmask(SIG_UNBLOCK, &timer_mask, NULL);
}

/* ====================== Helpers de planificación ====================== */
static inline void schedule_add(my_thread_t *t) {
    switch (t->sched) {
        case SCHED_RR:
            rr_add(t);
            break;
        case SCHED_LOTTERY:
            lottery_add(t);
            break;
        case SCHED_RT:
            rt_add(t);
            break;
        default:
            /* Por defecto, RR */
            rr_add(t);
            break;
    }
}

static inline void schedule_yield_current(my_thread_t *t) {
    switch (t->sched) {
        case SCHED_RR:
            rr_yield_current(t);
            break;
        case SCHED_LOTTERY:
            lottery_yield_current(t);
            break;
        case SCHED_RT:
            rt_yield_current(t);
            break;
        default:
            rr_yield_current(t);
            break;
    }
}

/**
 * Selecciona el próximo hilo listo en orden de **prioridad rígida**:
 *   1) RT (EDF)
 *   2) Lottery
 *   3) RR
 * Devuelve NULL si ninguna cola tiene hilos.
 */
static my_thread_t *schedule_next(void) {
    my_thread_t *n;

    block_timer_signal();
    n = rt_pick_next();
    if (n) {
        unblock_timer_signal();
        return n;
    }
    n = lottery_pick_next();
    if (n) {
        unblock_timer_signal();
        return n;
    }
    n = rr_pick_next();
    unblock_timer_signal();
    return n;
}

/* ========================== Cambio de contexto ========================= */
/**
 * Cambia de `prev` (el que estaba en CPU) a `next`. Se asume que
 * `prev->ctx` ya contiene el contexto previo, y `next->ctx` está listo.
 */
static void context_switch(my_thread_t *next) {
    my_thread_t *prev = g_current;
    g_current = next;
    next->state = T_RUNNING;
    if (swapcontext(&prev->ctx, &next->ctx) == -1) {
        perror("swapcontext");
        exit(EXIT_FAILURE);
    }
}

/* ========================= Dispatcher SIGALRM ========================== */
/**
 * Manejador de SIGALRM: se dispara cada `QUANTUM_USEC` microsegundos.
 * Si el hilo actual estaba `T_RUNNING`, se lo marca `T_READY` y se reencola.
 * Luego busca el siguiente con `schedule_next()` y hace `context_switch`.
 */
static void sig_dispatcher(int sig) {
    (void)sig;

    block_timer_signal();
    my_thread_t *prev = g_current;
    if (prev->state == T_RUNNING) {
        prev->state = T_READY;
        schedule_yield_current(prev);
    }

    my_thread_t *next = schedule_next();
    if (!next) {
        /* No hay otro listo → el mismo hilo sigue corriendo */
        prev->state = T_RUNNING;
        unblock_timer_signal();
        return;
    }
    context_switch(next);
    /* Nunca regresa aquí */
    /* Se descargará el contexto de `next` */
}

/**
 * Inicializa temporizador preemptivo (SIGALRM cada `QUANTUM_USEC`).
 * Instala `sig_dispatcher` como handler de SIGALRM.
 */
static void timer_init(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sig_dispatcher;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; /* dejamos que la señal se vuelva a generar si cae otra */
    if (sigaction(SIGALRM, &sa, NULL) != 0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    struct itimerval tv;
    tv.it_interval.tv_sec  = 0;
    tv.it_interval.tv_usec = QUANTUM_USEC;
    tv.it_value = tv.it_interval; /* arranca tras QUANTUM_USEC */
    if (setitimer(ITIMER_REAL, &tv, NULL) != 0) {
        perror("setitimer");
        exit(EXIT_FAILURE);
    }
}

/* ======================== Trampolín de hilos ========================== */
/* Prototipo adelantado para makecontext, con firma CORRECTA */
static void thread_trampoline(void *(*start_routine)(void *), void *arg);

static void thread_trampoline(void *(*start_routine)(void *), void *arg) {
    void *ret = NULL;
    /* Llamar a la función de usuario con firma correcta */
    ret = start_routine(arg);
    my_thread_exit(ret);
}

/* ======================== Runtime Bootstrap =========================== */
/**
 * Se ejecuta automáticamente antes de `main()`.
 * - Inicializa `timer_mask` para bloquear/desbloquear SIGALRM.
 * - Crea el TCB para el hilo principal (main) y le hace getcontext().
 * - Inicializa los tres schedulers.
 * - Arranca el temporizador preemptivo.
 */
__attribute__((constructor))
static void runtime_bootstrap(void) {
    /* Preparamos la máscara con solo SIGALRM */
    sigemptyset(&timer_mask);
    sigaddset(&timer_mask, SIGALRM);

    /* Seed para la lotería */
    srand((unsigned)time(NULL));

    /* Crear TCB dinámico para el hilo principal */
    my_thread_t *main_t = calloc(1, sizeof(*main_t));
    if (!main_t) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    /* ======== CORRECCIÓN: Asignar ID dentro de la sección crítica ======== */
    block_timer_signal();
    if (g_next_id >= MAX_THREADS) {
        unblock_timer_signal();
        fprintf(stderr, "No hay IDs disponibles para hilo principal\n");
        exit(EXIT_FAILURE);
    }
    main_t->id = g_next_id++;
    unblock_timer_signal();

    main_t->state      = T_RUNNING;
    main_t->sched      = SCHED_RR;  /* Por defecto RR */
    main_t->tickets    = 0;
    main_t->deadline   = 0;
    main_t->stack_base = NULL;      /* Usa la pila del propio proceso */
    main_t->next       = NULL;

    /* === CORRECCIÓN CLAVE: inicializar el contexto real del hilo “main” === */
    if (getcontext(&main_t->ctx) == -1) {
        perror("getcontext");
        exit(EXIT_FAILURE);
    }

    g_current = main_t;
    g_threads[main_t->id] = main_t;

    /* Inicializar cada scheduler */
    rr_init();
    lottery_init();
    rt_init();

    /* Inicializar temporizador preemptivo */
    timer_init();
}

/* ============================= API ===================================== */

/**
 * Atómico para swap de enteros (se usa en mutex).
 */
static inline int atomic_xchg(volatile int *ptr, int val) {
    return __sync_lock_test_and_set(ptr, val);
}

/**
 * Crea un nuevo hilo de usuario.
 * - thread_out: si no es NULL, copia superficialmente el TCB (incluyendo `id`) en esa zona.
 * - attr_unused: (no usado).
 * - start_routine: función que ejecutará el hilo.
 * - arg: argumento para start_routine.
 * - sched: política inicial (SCHED_RR, SCHED_LOTTERY o SCHED_RT). Si no coincide, se usa RR.
 *
 * Retorna:
 *   0       → éxito
 *   EAGAIN  → no hay IDs disponibles (se alcanzó MAX_THREADS)
 *   ENOMEM  → falla de memoria
 *   Otro errno → error de `getcontext` o similar
 */
int my_thread_create(my_thread_t *thread_out,
                     const void *attr_unused,
                     void *(*start_routine)(void *),
                     void *arg,
                     sched_type_t sched) {
    (void)attr_unused;

    /* ======== CORRECCIÓN: Asignar ID dentro de la sección crítica ======== */
    block_timer_signal();
    if (g_next_id >= MAX_THREADS) {
        unblock_timer_signal();
        return EAGAIN;
    }
    uint32_t new_id = g_next_id++;
    unblock_timer_signal();

    /* Reservar TCB dinámico */
    my_thread_t *t = calloc(1, sizeof(*t));
    if (!t) {
        return ENOMEM;
    }
    /* Asignar campos básicos */
    t->id       = new_id;
    t->state    = T_READY;
    /* Validar `sched`; si no es uno de los tres, dejamos RR */
    if (sched == SCHED_LOTTERY || sched == SCHED_RT) {
        t->sched = sched;
    } else {
        t->sched = SCHED_RR;
    }
    t->tickets    = (t->sched == SCHED_LOTTERY) ? 1 : 0;
    t->deadline   = 0;  /* si luego cambia a SCHED_RT, se asigna deadline válido */
    t->next       = NULL;

    /* Reserva de pila alineada */
    size_t stack_sz = MYTHREAD_DEFAULT_STACK;
    void *stack     = NULL;
    if (posix_memalign(&stack, 16, stack_sz) != 0) {
        free(t);
        return ENOMEM;
    }
    t->stack_base = stack;

    /* Inicializar contexto */
    if (getcontext(&t->ctx) == -1) {
        free(stack);
        free(t);
        return errno;
    }
    t->ctx.uc_stack.ss_sp   = stack;
    t->ctx.uc_stack.ss_size = stack_sz;
    t->ctx.uc_link          = NULL;
    makecontext(&t->ctx,
                (void (*)(void))thread_trampoline,
                2,
                start_routine,
                arg);

    /* Registrar y encolar en el scheduler */
    block_timer_signal();
    g_threads[t->id] = t;
    schedule_add(t);
    unblock_timer_signal();

    /* Copia superficial del TCB (para que el usuario tenga su “handle” con el mismo `id`) */
    if (thread_out) {
        *thread_out = *t;
    }
    return 0;
}

/**
 * Termina el hilo actual. Marca T_ZOMBIE y elige el siguiente.
 * Si no hay siguiente, hace `exit(0)` para cerrar el proceso.
 */
void my_thread_exit(void *retval) {
    block_timer_signal();
    g_current->retval = retval;
    g_current->state  = T_ZOMBIE;

    /* Elegir siguiente hilo listo */
    my_thread_t *next = schedule_next();
    if (!next) {
        /* Último hilo vivo → terminamos el proceso */
        unblock_timer_signal();
        exit(0);
    }
    unblock_timer_signal();
    context_switch(next);

    __builtin_unreachable();
}

/**
 * Espera a que el hilo con ID = `thread->id` llegue a T_ZOMBIE.
 * Luego, si `retval != NULL`, deja el valor en `*retval`.
 * Finalmente libera la pila y el TCB real (no la copia que hizo el usuario).
 */
int my_thread_join(my_thread_t *thread, void **retval) {
    if (!thread) return EINVAL;

    /* Si el usuario intentó hacer join sobre sí mismo (comparando `id`), falla */
    block_timer_signal();
    if (thread->id == g_current->id) {
        unblock_timer_signal();
        return EDEADLK;
    }
    /* Buscar el TCB real según el `id` en g_threads[] */
    my_thread_t *real = NULL;
    if (thread->id < MAX_THREADS) {
        real = g_threads[thread->id];
    }
    if (!real) {
        /* No existe (puede haber sido liberado ya) */
        unblock_timer_signal();
        return ESRCH;
    }
    unblock_timer_signal();

    /* Esperar activamente (cooperativo) hasta que alcance T_ZOMBIE */
    while (1) {
        block_timer_signal();
        if (real->state == T_ZOMBIE) {
            unblock_timer_signal();
            break;
        }
        unblock_timer_signal();
        my_thread_yield();
    }

    /* Ahora el hilo real está en T_ZOMBIE */
    if (retval) {
        *retval = real->retval;
    }

    /* Liberar recursos */
    block_timer_signal();
    free(real->stack_base);
    g_threads[real->id] = NULL;
    unblock_timer_signal();
    free(real);
    return 0;
}

/**
 * Cede voluntariamente la CPU: genera un SIGALRM que hará que el scheduler
 * elija otro hilo (igual que si se agotara el quantum).
 */
void my_thread_yield(void) {
    raise(SIGALRM);
}

/**
 * Cambia la política de scheduling del hilo actual.
 * Debe extraerlo de su cola anterior y re-encolarlo en la nueva.
 * Si pasa a SCHED_RT, asignamos un deadline = tiempo actual + 100 ms.
 */
int my_thread_chsched(sched_type_t new_sched) {
    block_timer_signal();
    my_thread_t *self = g_current;

    /* Quitarlo de la cola actual */
    switch (self->sched) {
        case SCHED_RR:
            rr_remove(self);
            break;
        case SCHED_LOTTERY:
            lottery_remove(self);
            break;
        case SCHED_RT:
            rt_remove(self);
            break;
        default:
            rr_remove(self);
            break;
    }

    /* Asignar nueva política (o RR si `new_sched` no coincide) */
    if (new_sched == SCHED_LOTTERY || new_sched == SCHED_RT) {
        self->sched = new_sched;
    } else {
        self->sched = SCHED_RR;
    }

    /* Si es RT, dar un deadline = ahora + 100 ms */
    if (self->sched == SCHED_RT) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        uint64_t now_us = (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
        self->deadline = now_us + 100000;  /* ej. 100 ms a futuro */
    }

    /* Re-encolar en el scheduler correspondiente */
    schedule_add(self);
    unblock_timer_signal();
    return 0;
}

/* ============================ Mutex ==================================== */

/**
 * Inicializa un mutex recursivo:
 * lock=0 (libre), owner=NULL, recursion=0, waiters vacía.
 */
int my_mutex_init(my_mutex_t *m) {
    if (!m) return EINVAL;

    m->lock = 0;
    m->owner = NULL;
    m->recursion = 0;
    queue_init(&m->waiters);
    return 0;
}

/**
 * Intenta tomar el mutex:
 * - Si `owner == self`, incrementa `recursion`.
 * - Si `lock==0`, lo toma: owner=self, recursion=1.
 * - Si `lock==1` y owner!=self, se bloquea el hilo en `waiters`
 *   y se hace `raise(SIGALRM)` para despachar otro.
 */
int my_mutex_lock(my_mutex_t *m) {
    if (!m) return EINVAL;

    my_thread_t *self = g_current;
    block_timer_signal();
    /* Si es reentrante */
    if (m->owner == self) {
        m->recursion++;
        unblock_timer_signal();
        return 0;
    }

    /* Intentar tomar atómicamente */
    if (atomic_xchg(&m->lock, 1) == 0) {
        /* Lo tomó con éxito */
        m->owner = self;
        m->recursion = 1;
        unblock_timer_signal();
        return 0;
    }

    /* Si llega aquí, ya estaba tomado por otro */
    self->state = T_BLOCKED;
    queue_push(&m->waiters, self);
    unblock_timer_signal();
    /* Forzar dispatch: el hilo actual ya no corre */
    raise(SIGALRM);

    /* Nunca regresa aquí */
    return 0;
}

/**
 * Libera el mutex:
 * - Si `recursion > 1`, decrementa y retorna.
 * - Si `recursion == 1`, lo libera y despierta al primer `waiter`:
 *     * owner = next, recursion = 1, lock = 1
 *     * y se re-encola `next`.
 * - Si no hay `waiters`, pone lock=0, owner=NULL.
 */
int my_mutex_unlock(my_mutex_t *m) {
    if (!m) return EINVAL;

    block_timer_signal();
    if (m->owner != g_current) {
        unblock_timer_signal();
        return EPERM;
    }
    if (--m->recursion > 0) {
        /* Aún queda recursión */
        unblock_timer_signal();
        return 0;
    }
    /* Recursión == 0 → liberar dueño */
    m->owner = NULL;
    atomic_xchg(&m->lock, 0);

    /* Despertar al siguiente waiter, si existe */
    my_thread_t *next = queue_pop(&m->waiters);
    if (next) {
        next->state = T_READY;
        m->owner = next;
        m->recursion = 1;
        /* Marcar nuevamente lock=1 para reflejar que next es dueño */
        atomic_xchg(&m->lock, 1);
        /* Re-encolar en el scheduler de `next` */
        schedule_add(next);
    }
    unblock_timer_signal();
    return 0;
}

/**
 * Destruye el mutex. En esta implementación no hay memoria dinámica
 * adicional que liberar; si hubiera hilos en `waiters`, se los deja bloqueados.
 */
int my_mutex_destroy(my_mutex_t *m) {
    (void)m;
    return 0;
}
