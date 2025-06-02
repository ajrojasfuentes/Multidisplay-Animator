#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>       // sigaction, SIGALRM
#include <sys/time.h>     // setitimer, struct itimerval
#include <time.h>         // srand(), rand()
#include "mypthread.h"

/* ====================== Variables Globales ====================== */
my_thread_t *current_thread = NULL;
my_thread_t *main_thread    = NULL;

/* --- Quitar static para que test.c pueda “extern” estos variables --- */
my_thread_t *rr_head       = NULL;
my_thread_t *lottery_head  = NULL;
my_thread_t *rt_head       = NULL;

/* ====================== UTILIDADES COMUNES ====================== */
/*
 * Quita a ‘t’ de la lista circular apuntada por *head_ptr,
 * donde “mode” indica a qué lista corresponde:
 *   mode == SCHED_RR      → rr_head
 *   mode == SCHED_LOTTERY → lottery_head
 *   mode == SCHED_RT      → rt_head
 */
static void remove_from_list(my_thread_t **head_ptr, my_thread_t *t, int mode) {
    if (!t || !(*head_ptr)) return;

    my_thread_t *head = *head_ptr;
    if (mode == SCHED_RR) {
        if (!t->rr_next || !t->rr_prev) return;
        if (t->rr_next == t) {
            *head_ptr = NULL;
        } else {
            t->rr_prev->rr_next = t->rr_next;
            t->rr_next->rr_prev = t->rr_prev;
            if (head == t) {
                *head_ptr = t->rr_next;
            }
        }
        t->rr_next = t->rr_prev = NULL;

    } else if (mode == SCHED_LOTTERY) {
        if (!t->lottery_next || !t->lottery_prev) return;
        if (t->lottery_next == t) {
            *head_ptr = NULL;
        } else {
            t->lottery_prev->lottery_next = t->lottery_next;
            t->lottery_next->lottery_prev = t->lottery_prev;
            if (head == t) {
                *head_ptr = t->lottery_next;
            }
        }
        t->lottery_next = t->lottery_prev = NULL;

    } else if (mode == SCHED_RT) {
        if (!t->rt_next || !t->rt_prev) return;
        if (t->rt_next == t) {
            *head_ptr = NULL;
        } else {
            t->rt_prev->rt_next = t->rt_next;
            t->rt_next->rt_prev = t->rt_prev;
            if (head == t) {
                *head_ptr = t->rt_next;
            }
        }
        t->rt_next = t->rt_prev = NULL;
    }
}

/* ====================== ROUND-ROBIN (RR) ====================== */
void rr_init(void) {
    rr_head = NULL;
}

void rr_add(my_thread_t *t) {
    if (!t) return;
    t->rr_next = t->rr_prev = NULL;

    if (!rr_head) {
        rr_head = t;
        t->rr_next = t->rr_prev = t;
    } else {
        my_thread_t *tail = rr_head->rr_prev;
        tail->rr_next = t;
        t->rr_prev       = tail;
        t->rr_next       = rr_head;
        rr_head->rr_prev = t;
    }
}

my_thread_t *rr_pick_next(void) {
    return rr_head;
}

void rr_remove(my_thread_t *t) {
    remove_from_list(&rr_head, t, SCHED_RR);
}

void rr_yield_current(void) {
    my_thread_t *prev = current_thread;
    my_thread_t *next = rr_pick_next();
    if (next && next != prev) {
        current_thread = next;
        swapcontext(&prev->context, &next->context);
    }
}

/* ====================== LOTTERY SCHEDULER ====================== */
void lottery_init(void) {
    lottery_head = NULL;
}

void lottery_add(my_thread_t *t) {
    if (!t) return;
    t->lottery_next = t->lottery_prev = NULL;

    if (!lottery_head) {
        lottery_head        = t;
        t->lottery_next     = t;
        t->lottery_prev     = t;
    } else {
        my_thread_t *tail      = lottery_head->lottery_prev;
        tail->lottery_next     = t;
        t->lottery_prev        = tail;
        t->lottery_next        = lottery_head;
        lottery_head->lottery_prev = t;
    }
}

void lottery_remove(my_thread_t *t) {
    remove_from_list(&lottery_head, t, SCHED_LOTTERY);
}

my_thread_t *lottery_pick_next(void) {
    if (!lottery_head) return NULL;

    /* Calcular total de boletos */
    int total_tickets = 0;
    my_thread_t *iter = lottery_head;
    do {
        total_tickets += iter->tickets;
        iter = iter->lottery_next;
    } while (iter != lottery_head);

    if (total_tickets <= 0) {
        /* Todos a cero → devolvemos el head */
        return lottery_head;
    }

    /* Número aleatorio en [1..total_tickets] */
    int winner = (rand() % total_tickets) + 1;

    int acum = 0;
    iter = lottery_head;
    do {
        acum += iter->tickets;
        if (acum >= winner) {
            return iter;
        }
        iter = iter->lottery_next;
    } while (iter != lottery_head);

    /* Fallback improbable */
    return lottery_head;
}

void lottery_yield_current(void) {
    my_thread_t *prev = current_thread;
    my_thread_t *next = lottery_pick_next();
    if (next && next != prev) {
        current_thread = next;
        swapcontext(&prev->context, &next->context);
    }
}

/* ====================== REAL-TIME SCHEDULER ====================== */
void rt_init(void) {
    rt_head = NULL;
}

void rt_add(my_thread_t *t) {
    if (!t) return;
    t->rt_next = t->rt_prev = NULL;

    if (!rt_head) {
        rt_head   = t;
        t->rt_next = t;
        t->rt_prev = t;
    } else {
        my_thread_t *tail  = rt_head->rt_prev;
        tail->rt_next      = t;
        t->rt_prev         = tail;
        t->rt_next         = rt_head;
        rt_head->rt_prev   = t;
    }
}

void rt_remove(my_thread_t *t) {
    remove_from_list(&rt_head, t, SCHED_RT);
}

my_thread_t *rt_pick_next(void) {
    if (!rt_head) return NULL;

    /* Buscar máxima prioridad */
    my_thread_t *mejor = rt_head;
    my_thread_t *iter  = rt_head->rt_next;
    do {
        if (iter->rt_priority > mejor->rt_priority) {
            mejor = iter;
        }
        iter = iter->rt_next;
    } while (iter != rt_head);

    return mejor;
}

void rt_yield_current(void) {
    my_thread_t *prev = current_thread;
    my_thread_t *next = rt_pick_next();
    if (next && next != prev) {
        current_thread = next;
        swapcontext(&prev->context, &next->context);
    }
}

/* ====================== INTERFAZ PÚBLICA ====================== */
int my_thread_create(my_thread_t **thread,
                     void (*start_routine)(void),
                     int sched_type,
                     int attr)
{
    if (!thread || !start_routine) return -1;
    if (sched_type != SCHED_RR &&
        sched_type != SCHED_LOTTERY &&
        sched_type != SCHED_RT) {
        return -1;
    }

    *thread = (my_thread_t*) malloc(sizeof(my_thread_t));
    if (!*thread) return -1;

    /* Inicializar contexto y pila */
    getcontext(&((*thread)->context));
    (*thread)->context.uc_stack.ss_sp   = malloc(STACK_SIZE);
    if (!(*thread)->context.uc_stack.ss_sp) {
        free(*thread);
        return -1;
    }
    (*thread)->context.uc_stack.ss_size = STACK_SIZE;
    (*thread)->context.uc_link          = NULL;

    /* Ciclo de vida */
    (*thread)->finished     = 0;
    (*thread)->detached     = 0;
    (*thread)->waiting_list = NULL;
    (*thread)->next         = NULL;

    /* Scheduler y campos auxiliares */
    (*thread)->sched_type = sched_type;
    if (sched_type == SCHED_LOTTERY) {
        (*thread)->tickets     = (attr > 0 ? attr : 1);
        (*thread)->rt_priority = 0;
    } else if (sched_type == SCHED_RT) {
        (*thread)->rt_priority = (attr >= 0 ? attr : 0);
        (*thread)->tickets     = 0;
    } else { /* SCHED_RR */
        (*thread)->tickets     = 0;
        (*thread)->rt_priority = 0;
    }

    /* Punteros de lista en NULL (serán seteados en el add) */
    (*thread)->rr_next        = NULL;
    (*thread)->rr_prev        = NULL;
    (*thread)->lottery_next   = NULL;
    (*thread)->lottery_prev   = NULL;
    (*thread)->rt_next        = NULL;
    (*thread)->rt_prev        = NULL;

    /* Preparar contexto para arrancar start_routine() */
    makecontext(&((*thread)->context), start_routine, 0);

    /* Insertar en la lista correspondiente */
    if (sched_type == SCHED_RR) {
        rr_add(*thread);
    } else if (sched_type == SCHED_LOTTERY) {
        lottery_add(*thread);
    } else {
        rt_add(*thread);
    }

    return 0;
}

int my_thread_chsched(my_thread_t *target,
                      int new_sched,
                      int new_attr)
{
    if (!target) return -1;
    if (new_sched != SCHED_RR &&
        new_sched != SCHED_LOTTERY &&
        new_sched != SCHED_RT) {
        return -1;
    }
    if (target == current_thread) {
        /* No permitimos cambiar el scheduler del hilo actual */
        return -1;
    }

    /* 1) Remover de la lista actual */
    int old_sched = target->sched_type;
    if (old_sched == SCHED_RR) {
        rr_remove(target);
    } else if (old_sched == SCHED_LOTTERY) {
        lottery_remove(target);
    } else {
        rt_remove(target);
    }

    /* 2) Ajustar campos según new_sched */
    target->sched_type = new_sched;
    if (new_sched == SCHED_LOTTERY) {
        target->tickets     = (new_attr > 0 ? new_attr : 1);
        target->rt_priority = 0;
    } else if (new_sched == SCHED_RT) {
        target->rt_priority = (new_attr >= 0 ? new_attr : 0);
        target->tickets     = 0;
    } else { /* SCHED_RR */
        target->tickets     = 0;
        target->rt_priority = 0;
    }

    /* 3) Insertar en la lista nueva */
    if (new_sched == SCHED_RR) {
        rr_add(target);
    } else if (new_sched == SCHED_LOTTERY) {
        lottery_add(target);
    } else {
        rt_add(target);
    }

    return 0;
}

void my_thread_yield(void)
{
    if (!current_thread) return;

    switch (current_thread->sched_type) {
        case SCHED_RR:
            rr_yield_current();
            break;
        case SCHED_LOTTERY:
            lottery_yield_current();
            break;
        case SCHED_RT:
            rt_yield_current();
            break;
        default:
            rr_yield_current();
            break;
    }
}

int my_thread_join(my_thread_t *target)
{
    if (!target) return -1;
    if (target == current_thread) return -1;
    if (target->detached) return -1;
    if (target->finished) return 0;

    /* Agregar current_thread a la waiting_list de target */
    current_thread->next = NULL;
    if (!target->waiting_list) {
        target->waiting_list = current_thread;
    } else {
        my_thread_t *tmp = target->waiting_list;
        while (tmp->next) tmp = tmp->next;
        tmp->next = current_thread;
    }

    /* Remover current_thread de su lista de scheduler */
    int me_sched = current_thread->sched_type;
    if (me_sched == SCHED_RR) {
        rr_remove(current_thread);
    } else if (me_sched == SCHED_LOTTERY) {
        lottery_remove(current_thread);
    } else {
        rt_remove(current_thread);
    }

    /* Elegir siguiente hilo listo: RR > Lottery > RT */
    my_thread_t *next = NULL;
    if (rr_head) {
        next = rr_pick_next();
    } else if (lottery_head) {
        next = lottery_pick_next();
    } else if (rt_head) {
        next = rt_pick_next();
    }

    if (next) {
        my_thread_t *prev = current_thread;
        current_thread    = next;
        swapcontext(&prev->context, &next->context);
    }

    return 0;
}

int my_thread_detach(my_thread_t *target)
{
    if (!target) return -1;
    if (target->detached) return -1;
    target->detached = 1;
    return 0;
}

void my_thread_end(void)
{
    if (!current_thread) {
        return;
    }

    /* 1) Marcar terminado y remover de lista */
    current_thread->finished = 1;
    int me_sched = current_thread->sched_type;
    if (me_sched == SCHED_RR) {
        rr_remove(current_thread);
    } else if (me_sched == SCHED_LOTTERY) {
        lottery_remove(current_thread);
    } else {
        rt_remove(current_thread);
    }

    /* 2) Reinyectar a todos los que hacían join sobre este hilo */
    while (current_thread->waiting_list) {
        my_thread_t *w = current_thread->waiting_list;
        current_thread->waiting_list = w->next;
        w->next = NULL;

        if (w->sched_type == SCHED_RR) {
            rr_add(w);
        } else if (w->sched_type == SCHED_LOTTERY) {
            lottery_add(w);
        } else {
            rt_add(w);
        }
    }

    /* 3) Si estaba detached, liberar memoria */
    if (current_thread->detached) {
        free(current_thread->context.uc_stack.ss_sp);
        free(current_thread);
        /* current_thread ya no es válido después de liberar */
    }

    /* 4) Despachar siguiente hilo listo: RR > Lottery > RT */
    my_thread_t *next = NULL;
    if (rr_head) {
        next = rr_pick_next();
    } else if (lottery_head) {
        next = lottery_pick_next();
    } else if (rt_head) {
        next = rt_pick_next();
    }

    if (next) {
        current_thread = next;
        setcontext(&next->context);
    } else {
        /* Ningún hilo listo: detener timer y salir */
        struct itimerval stop = {0};
        setitimer(ITIMER_REAL, &stop, NULL);
        printf("[DEBUG] Todos los hilos han terminado.\n");
        exit(0);
    }
}

/* ====================== Temporizador (SIGALRM) ====================== */
static void scheduler_handler(int signum) {
    (void)signum;
    if (current_thread) {
        my_thread_yield();
    }
}

void init_timer(void)
{
    struct sigaction sa;
    sa.sa_handler = scheduler_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGALRM, &sa, NULL);

    struct itimerval timer;
    timer.it_value.tv_sec   = 0;
    timer.it_value.tv_usec  = 100000;      /* 100 ms */
    timer.it_interval        = timer.it_value;
    setitimer(ITIMER_REAL, &timer, NULL);
}
