#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <sys/socket.h>  
#include <fcntl.h>  
#include <unistd.h>  
#include <stdio.h>  
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <sys/queue.h>
#if defined(__linux__)
#include <sys/prctl.h>
#endif

#include "min_heap.h"
#include "timer_struct.h"
#include "timer_event.h"

#define UNUSED(x) (void)(x)

static timer_base time_internal_data;
long evutil_tv_to_msec_(const struct timeval *tv) { 
    return (tv->tv_sec * 1000) + ((tv->tv_usec + 999) / 1000); 
}
static void timeout_process(min_heap_t * minheap);
static void add_time_event(struct time_event *event_data, struct timeval *now);

static int get_clock_time(struct timespec *spec) {
    if (clock_gettime(CLOCK_MONOTONIC, spec) == -1) {
        return -1;
    }
    return 0;
}

int gettime(struct timeval *tp) {
    if (NULL == tp) {
        return -1;
    }
#ifdef CLOCK_MONOTONIC
    struct timespec spec;
    if (-1 == get_clock_time(&spec)) {
        printf("get_clock_time error \n");
        return -1;
    }
    tp->tv_sec = spec.tv_sec;
    tp->tv_usec = spec.tv_nsec / 1000;
    return 0;
#endif
    struct timeval cur_time;
    gettimeofday(&cur_time, NULL);
    tp->tv_sec = cur_time.tv_sec;
    tp->tv_usec = cur_time.tv_usec;
    return 0;
}

bool add_new_time(void (*cb_callback)(void *args), void *args, struct timeval * use_tv) {
    if (use_tv == NULL || (use_tv->tv_sec == 0 && use_tv->tv_usec == 0)) {
        return false;
    }
    struct time_event * ev = calloc(1, sizeof(struct time_event));
    if (ev == NULL) {
        return false;
    }
    ev->call_info.evcb_callback = cb_callback;
    ev->call_info.evcb_arg = args;
    ev->delay_time = *use_tv;
    ev->call_info.base = ev;
    add_time_event(ev, NULL);
    return true;
}

void add_time_event(struct time_event *event_data, struct timeval *now)
{
    struct timeval tmp_now;
    if (NULL == now) {
        gettime(&tmp_now);
    } else {
        tmp_now = *now;
    }
    timeradd(&tmp_now, &event_data->delay_time, &event_data->ev_timeout);
    pthread_mutex_lock(&time_internal_data.event_mutex);
    min_heap_push_(&time_internal_data.minheap_t, event_data);
    pthread_mutex_unlock(&time_internal_data.event_mutex);
    write(time_internal_data.th_notify_fd[1],"1", 1);
    return;
}

static int timeout_next(struct timeval *tv_p) {
	struct timeval now;
	struct timeval *tv = tv_p;
	int res = 0;
    pthread_mutex_lock(&time_internal_data.event_mutex);
	struct time_event *  data_base = min_heap_top_(&time_internal_data.minheap_t);
    pthread_mutex_unlock(&time_internal_data.event_mutex);
	if (data_base == NULL) {
		tv_p = NULL;
		goto out;
	}

	if (gettime(&now) == -1) {
		res = -1;
		goto out;
	}

	if (evutil_timercmp(&data_base->ev_timeout, &now, <=)) {
		evutil_timerclear(tv);
		goto out;
	}
    evutil_timersub(&data_base->ev_timeout, &now, tv);
out:
	return (res);
}

static void * time_dispatch(void *args) {
    UNUSED(args);
    struct timeval dafeult = {.tv_sec = 0, .tv_usec = 10000}, get_time, wait_time;
    int nret = 0;
    char buf[2] = {0};
    fd_set rset, allset;
    FD_ZERO(&allset);
    FD_SET(time_internal_data.th_notify_fd[0], &allset);
    while(time_internal_data.dis_runing) {
        rset = allset;
        timeout_next(&get_time);
        wait_time = dafeult;
        if (get_time.tv_sec != 0 || get_time.tv_usec != 0) {
            wait_time = get_time;
        }
        nret = select(time_internal_data.th_notify_fd[0] + 1, &rset, NULL, NULL, &wait_time);
        if (nret < 0) {
			exit(0);
        }
        if (nret > 0) {
            read(time_internal_data.th_notify_fd[0], buf, 1);
        }
        timeout_process(&time_internal_data.minheap_t);
    }
    return NULL;
}

void timeout_process(min_heap_t * minheap) {
    if (!minheap) {
        return;
    }
    struct timeval now;
    pthread_mutex_lock(&time_internal_data.event_mutex);
	if (min_heap_empty_(minheap)) {
        pthread_mutex_unlock(&time_internal_data.event_mutex);
        return;
	}
    struct time_event * event_data = NULL;
    bool notify = false;
    while ((event_data = min_heap_top_(minheap))) {
        gettime(&now);
		if (evutil_timercmp(&event_data->ev_timeout, &now, >)) {
			break;
        }
        notify = false;
        min_heap_erase_(minheap, event_data);
        pthread_mutex_lock(&time_internal_data.callback_mutex);
        if (TAILQ_EMPTY(&time_internal_data.evcall_queue_stru_head) || time_internal_data.queue_empty) {
            notify = true;
        }
        TAILQ_INSERT_TAIL(&time_internal_data.evcall_queue_stru_head, &(event_data->call_info), evcb_active_next);
        pthread_mutex_unlock(&time_internal_data.callback_mutex);
        if (notify) {
            pthread_cond_signal(&time_internal_data.call_cond);
        }
        printf(" timer test pid = %5lu 1111111111111111111 \n", syscall(SYS_gettid));
	}
    pthread_mutex_unlock(&time_internal_data.event_mutex);
    printf(" timer test pid = %5ld 22222222222222222222 \n", syscall(SYS_gettid));
    return;
}

static void * timer_exec_func(void *args) {
    /* Set thread name for profiling and debuging */
	char thread_name[32] = {0};
	snprintf(thread_name, 32, "eli_timer_th_%d", *(int *)args);
#if defined(__linux__)
	prctl(PR_SET_NAME, thread_name);
#elif defined(__APPLE__) && defined(__MACH__)
	pthread_setname_np(thread_name);
#else
	err("timer_exec_func(): pthread_setname_np is not supported on this system");
#endif

    struct event_callback * callback_info = NULL;
    while(time_internal_data.exec_func_runing) {
        pthread_mutex_lock(&time_internal_data.callback_mutex);
        while (TAILQ_EMPTY(&time_internal_data.evcall_queue_stru_head)) {
            time_internal_data.queue_empty++;
            printf(" pid= %5ld test 99999999999999999\n ",syscall(SYS_gettid));
            pthread_cond_wait(&time_internal_data.call_cond, &time_internal_data.callback_mutex);
        }
        time_internal_data.queue_empty--;
        callback_info = TAILQ_FIRST(&time_internal_data.evcall_queue_stru_head);
        evcb_callback_type cb = callback_info->evcb_callback;
        void * args = callback_info->evcb_arg;
        if (NULL != callback_info) {
            printf(" pid= %5ld test 7777777777777777\n ", syscall(SYS_gettid));
            TAILQ_REMOVE(&time_internal_data.evcall_queue_stru_head, callback_info, evcb_active_next);
        }
        pthread_mutex_unlock(&time_internal_data.callback_mutex);
        if (NULL != callback_info) {
            cb(args);
        }
        add_time_event(callback_info->base, NULL);
    }
    return NULL;
}

static void time_thread_init() {
    time_internal_data.exec_func_runing = true;
    time_internal_data.queue_empty = 0;
    for (int i = 0; i < 3; ++i) {
        pthread_create(&time_internal_data.exec_func_pid[i], NULL, timer_exec_func, (void *)&i);
    }
    return;
}

void time_event_init(void) {
    TAILQ_INIT(&time_internal_data.evcall_queue_stru_head);
    time_thread_init();
    time_internal_data.dis_runing = true;
    min_heap_ctor_(&time_internal_data.minheap_t);
    min_heap_reserve_(&time_internal_data.minheap_t, 100);
    pthread_mutex_init(&time_internal_data.event_mutex, NULL);
    pthread_mutex_init(&time_internal_data.callback_mutex, NULL);
    pthread_cond_init(&time_internal_data.call_cond, NULL);
    time_internal_data.th_notify_fd[0] = -1;
    time_internal_data.th_notify_fd[1] = -1;
    if (pipe(time_internal_data.th_notify_fd) != 0) {
        return;
    }
    pthread_create(&time_internal_data.dispatch_id, NULL, time_dispatch, NULL);
    return;
}

void time_event_destroy(void) {
    time_internal_data.dis_runing = false;
    min_heap_dtor_(&(time_internal_data.minheap_t));
    pthread_mutex_destroy(&(time_internal_data.event_mutex));
    pthread_mutex_destroy(&(time_internal_data.callback_mutex));
    pthread_cond_destroy(&(time_internal_data.call_cond));
    pthread_join(time_internal_data.dispatch_id, NULL);
    return;
}

void update_timer_frequency(struct timeval * use_tv, void (*cb)(void *)) {
    if (NULL == use_tv) {
        return;
    }
    unsigned sec = use_tv->tv_sec;
    unsigned usec = use_tv->tv_usec;
    pthread_mutex_lock(&(time_internal_data.event_mutex));
    min_heap_update_elm(&time_internal_data.minheap_t, sec, usec, cb);
    pthread_mutex_unlock(&(time_internal_data.event_mutex));
    return;
}