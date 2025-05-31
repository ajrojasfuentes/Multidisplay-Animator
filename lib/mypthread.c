/* =========================================================================
 * @file    mypthread.c
 * @brief   Reimplementación completa  en el espacio de usuario de una API mínima tipo Pthreads con
 *          soporte simultáneo de tres planificadores: Round‑Robin, Lottery
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
#include <stdatomic.h>

/* ------------------- Prototipos de los schedulers ---------------------- */
void rr_init(void);      void rr_add(my_thread_t*);      my_thread_t* rr_pick_next(void);      void rr_yield_current(my_thread_t*);
void lottery_init(void); void lottery_add(my_thread_t*); my_thread_t* lottery_pick_next(void); void lottery_yield_current(my_thread_t*);
void rt_init(void);      void rt_add(my_thread_t*);      my_thread_t* rt_pick_next(void);      void rt_yield_current(my_thread_t*);

/* ===================== Prototipo adelantado ============================ */
static void thread_trampoline(void *(*start)(void *), void *arg);

/* ========================= Configuración global ======================== */
#define MAX_THREADS  1024
#define QUANTUM_USEC 10000

static my_thread_t *g_threads[MAX_THREADS] = {0};
static uint32_t     g_next_id = 0;
static my_thread_t *g_current = NULL;

/* ===================== Utilidades de planificación ===================== */
static inline void schedule_add(my_thread_t* t){
    switch(t->sched){
        case SCHED_RR: rr_add(t); break;
        case SCHED_LOTTERY: lottery_add(t); break;
        case SCHED_RT: rt_add(t); break;
    }
}
static inline void schedule_yield_current(my_thread_t* t){
    switch(t->sched){
        case SCHED_RR: rr_yield_current(t); break;
        case SCHED_LOTTERY: lottery_yield_current(t); break;
        case SCHED_RT: rt_yield_current(t); break;
    }
}
static my_thread_t* schedule_next(void){
    my_thread_t* n = rt_pick_next(); if(n) return n;
    n = lottery_pick_next(); if(n) return n;
    return rr_pick_next();
}

/* ======================== Dispatcher y temporizador ==================== */
static void context_switch(my_thread_t* next){
    my_thread_t* prev = g_current;
    g_current = next;
    next->state = T_RUNNING;
    if(swapcontext(&prev->ctx,&next->ctx)==-1){perror("swapcontext");exit(EXIT_FAILURE);} }

static void sig_dispatcher(int sig){ (void)sig;
    my_thread_t* prev = g_current;
    if(prev->state==T_RUNNING){ prev->state = T_READY; schedule_yield_current(prev);}
    my_thread_t* next = schedule_next();
    if(!next){ prev->state = T_RUNNING; return; }
    context_switch(next);
}
static void timer_init(void){
    struct sigaction sa; memset(&sa,0,sizeof(sa));
    sa.sa_handler = sig_dispatcher; sigemptyset(&sa.sa_mask);
    if(sigaction(SIGALRM,&sa,NULL)!=0){perror("sigaction");exit(EXIT_FAILURE);}
    struct itimerval tv; tv.it_interval.tv_sec=0; tv.it_interval.tv_usec=QUANTUM_USEC; tv.it_value=tv.it_interval;
    if(setitimer(ITIMER_REAL,&tv,NULL)!=0){perror("setitimer");exit(EXIT_FAILURE);} }

/* ===================== Inicialización del runtime ====================== */
__attribute__((constructor))
static void runtime_bootstrap(void){
    srand((unsigned)time(NULL));
    my_thread_t* main_t = calloc(1,sizeof(*main_t)); if(!main_t){perror("calloc");exit(EXIT_FAILURE);}
    main_t->id = g_next_id++; main_t->state=T_RUNNING; main_t->sched=SCHED_RR; g_current=main_t; g_threads[main_t->id]=main_t;
    rr_init(); lottery_init(); rt_init();
    timer_init(); }

/* ============================== API ==================================== */
static inline int atomic_xchg(volatile int* ptr,int val){return __sync_lock_test_and_set(ptr,val);}

int my_thread_create(my_thread_t* thread_out,const void* attr_unused,void*(*start_routine)(void*),void* arg,sched_type_t sched){ (void)attr_unused;
    if(g_next_id>=MAX_THREADS) return EAGAIN;
    my_thread_t* t = calloc(1,sizeof(*t)); if(!t) return ENOMEM;
    t->id=g_next_id++; t->state=T_READY; t->sched=sched; if(sched==SCHED_LOTTERY) t->tickets=1;
    size_t stack_sz=MYTHREAD_DEFAULT_STACK; void* stack=NULL; if(posix_memalign(&stack,16,stack_sz)!=0){free(t);return ENOMEM;}
    if(getcontext(&t->ctx)==-1){free(stack);free(t);return errno;} t->ctx.uc_stack.ss_sp=stack; t->ctx.uc_stack.ss_size=stack_sz; t->ctx.uc_link=NULL; t->stack_base=stack;
    makecontext(&t->ctx,(void(*)(void))thread_trampoline,2,start_routine,arg);
    g_threads[t->id]=t; schedule_add(t); if(thread_out) *thread_out=*t; return 0; }

static void thread_trampoline(void *(*start)(void *), void *arg){ void* ret=start(arg); my_thread_exit(ret);}

void my_thread_exit(void* retval){
    g_current->retval = retval; g_current->state = T_ZOMBIE;
    my_thread_t* next = schedule_next(); if(!next) exit(0);
    context_switch(next); __builtin_unreachable(); }

int my_thread_join(my_thread_t* thread, void** retval){
    if(thread==g_current) return EDEADLK;
    while(thread->state!=T_ZOMBIE) my_thread_yield();
    if(retval) *retval = thread->retval;
    free(thread->stack_base); free(thread); return 0; }

void my_thread_yield(void){ raise(SIGALRM);}

int my_thread_chsched(sched_type_t new_sched){ g_current->sched=new_sched; return 0; }

/* ============================== Mutex =================================== */
int my_mutex_init(my_mutex_t* m){ m->lock=0; m->owner=NULL; m->recursion=0; return 0; }

int my_mutex_lock(my_mutex_t* m){
    my_thread_t* self = g_current;
    if(m->owner==self){ m->recursion++; return 0; }
    while(atomic_xchg(&m->lock,1)) my_thread_yield();
    m->owner=self; m->recursion=1; return 0; }

int my_mutex_unlock(my_mutex_t* m){
    if(m->owner!=g_current) return EPERM;
    if(--m->recursion==0){ m->owner=NULL; atomic_xchg(&m->lock,0);} return 0; }

int my_mutex_destroy(my_mutex_t* m){ (void)m; return 0; }
