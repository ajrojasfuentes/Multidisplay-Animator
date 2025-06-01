#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>       // sigaction, sigemptyset, SIGALRM
#include <sys/time.h>     // struct itimerval, setitimer
#include "mypthread.h"

my_thread_t *current_thread = NULL;
my_thread_t *main_thread = NULL;

static my_thread_t *rr_head = NULL;

// ==================== Scheduler RR ====================
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
        t->rr_prev = tail;
        t->rr_next = rr_head;
        rr_head->rr_prev = t;
    }
}

my_thread_t *rr_pick_next(void) {
    return rr_head;
}

void rr_remove(my_thread_t *t) {
    if (!t || !t->rr_next || !t->rr_prev) return;

    if (t->rr_next == t) {
        rr_head = NULL;
    } else {
        t->rr_prev->rr_next = t->rr_next;
        t->rr_next->rr_prev = t->rr_prev;
        if (rr_head == t) rr_head = t->rr_next;
    }

    t->rr_next = t->rr_prev = NULL;
}

void rr_yield_current(void) {
    my_thread_t *prev = current_thread;
    my_thread_t *next = rr_pick_next();

    if (next && next != prev) {
        current_thread = next;
        swapcontext(&(prev->context), &(next->context));
    }
}

// ==================== API de hilos ====================
int my_thread_create(my_thread_t **thread, void (*start_routine)(void)) {
    *thread = malloc(sizeof(my_thread_t));
    if (!*thread) return -1;

    getcontext(&((*thread)->context));
    (*thread)->context.uc_stack.ss_sp = malloc(STACK_SIZE);
    (*thread)->context.uc_stack.ss_size = STACK_SIZE;
    (*thread)->context.uc_link = NULL;
    (*thread)->finished = 0;
    (*thread)->detached = 0;
    (*thread)->waiting_list = NULL;
    (*thread)->next = NULL;
    (*thread)->rr_next = NULL;
    (*thread)->rr_prev = NULL;

    makecontext(&((*thread)->context), start_routine, 0);
    rr_add(*thread);
    return 0;
}

void my_thread_yield(void) {
    rr_yield_current();
}

int my_thread_join(my_thread_t *target) {
    if (!target || target->detached || target == current_thread) return -1;
    if (target->finished) return 0;

    // Agregar al waiting_list
    current_thread->next = NULL;
    if (!target->waiting_list) {
        target->waiting_list = current_thread;
    } else {
        my_thread_t *tmp = target->waiting_list;
        while (tmp->next) tmp = tmp->next;
        tmp->next = current_thread;
    }

    rr_remove(current_thread);

    my_thread_t *next = rr_pick_next();
    if (next) {
        my_thread_t *prev = current_thread;
        current_thread = next;
        swapcontext(&(prev->context), &(next->context));
    }

    return 0;
}

int my_thread_detach(my_thread_t *target) {
    if (!target || target->detached) return -1;
    target->detached = 1;
    return 0;
}

void my_thread_end(void) {
    current_thread->finished = 1;
    rr_remove(current_thread);

    while (current_thread->waiting_list) {
        my_thread_t *w = current_thread->waiting_list;
        current_thread->waiting_list = w->next;
        w->next = NULL;
        rr_add(w);
    }

    if (current_thread->detached) {
        free(current_thread->context.uc_stack.ss_sp);
        free(current_thread);
    }

    my_thread_t *next = rr_pick_next();
    if (next) {
        current_thread = next;
        setcontext(&(next->context));
    } else {
        struct itimerval stop = {0};
        setitimer(ITIMER_REAL, &stop, NULL);
        printf("[DEBUG] Todos los hilos han terminado.\n");
        exit(0);
    }
}

// ==================== Temporizador ====================
void scheduler_handler(int signum) {
    (void)signum;
    my_thread_yield();
}

void init_timer(void) {
    struct sigaction sa;
    sa.sa_handler = scheduler_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGALRM, &sa, NULL);

    struct itimerval timer;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 100000;
    timer.it_interval = timer.it_value;
    setitimer(ITIMER_REAL, &timer, NULL);
}