#ifndef __TIMER_STRUCT_H__
#define __TIMER_STRUCT_H__

#include <stdlib.h>
#include <stdbool.h>
#include <sys/queue.h>

typedef void (*evcb_callback_type)(void *);
struct event_callback {
	TAILQ_ENTRY(event_callback) evcb_active_next;
	evcb_callback_type evcb_callback;
	void *evcb_arg;
    void * base;
};

struct time_event {
	struct event_callback call_info;
	int idx;
	struct timeval ev_timeout;
	struct timeval delay_time;
};

TAILQ_HEAD(evcall_queue_stru, event_callback);

typedef struct min_heap {
	struct time_event **p;
	unsigned n, a;
} min_heap_t;

typedef struct timer_base {
    pthread_t *exec_func_pid;
    pthread_t dispatch_id;
    pthread_mutex_t event_mutex;
    pthread_mutex_t callback_mutex;
    bool exec_func_runing;
    pthread_cond_t call_cond;
    min_heap_t minheap_t;
    bool dis_runing;
    struct evcall_queue_stru evcall_queue_stru_head;
    int th_notify_fd[2];
    volatile unsigned char queue_empty;
    int timer_exec_th_num;
} timer_base;

#endif //__TIME_STRUCT_H__
