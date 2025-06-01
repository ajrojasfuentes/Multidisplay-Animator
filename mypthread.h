#ifndef MYPTHREAD_H
#define MYPTHREAD_H

#include <ucontext.h>

#define STACK_SIZE 8192

typedef struct my_thread {
    ucontext_t context;
    int finished;
    int detached;
    struct my_thread *next;           // Para listas de espera
    struct my_thread *waiting_list;   // Hilos que esperan este
    struct my_thread *rr_next;        // Puntero siguiente en RR
    struct my_thread *rr_prev;        // Puntero anterior en RR
} my_thread_t;

// Variables globales
extern my_thread_t *current_thread;
extern my_thread_t *main_thread;

// API pública
int my_thread_create(my_thread_t **thread, void (*start_routine)(void));
void my_thread_yield(void);
void my_thread_end(void);
int my_thread_join(my_thread_t *target);
int my_thread_detach(my_thread_t *target);

// Interno
void rr_init(void);
void rr_add(my_thread_t *t);
my_thread_t *rr_pick_next(void);
void rr_yield_current(void);
void rr_remove(my_thread_t *t);
void init_timer(void);

#endif // MYPTHREAD_H