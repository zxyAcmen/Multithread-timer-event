#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <sys/socket.h>  
#include <fcntl.h>  
#include <unistd.h>  
#include <stdio.h>  
#include <sys/time.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <stdint.h>
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
static void add_timer_event(struct time_event *event_data);

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

bool add_new_timer(void (*cb_callback)(void *args), void *args, struct timeval * use_tv) {
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
    add_timer_event(ev);
    return true;
}

void add_timer_event(struct time_event *event_data) {
    struct timeval tmp_now;
    if (event_data->ev_timeout.tv_sec != 0 || event_data->ev_timeout.tv_usec != 0) {
        tmp_now = event_data->ev_timeout;
    } else {
        gettime(&tmp_now);
    }
    timeradd(&tmp_now, &event_data->delay_time, &event_data->ev_timeout);
    bool is_need_notify = false;
    //如果后续需要做动态删除，在结构里再添加一个字段表示这个事件要清除，并统一在此地方做删除，
    //因为在其它地方做删除可能会导致回调完后重新回来是个空指针而导致程序崩溃
    pthread_mutex_lock(&time_internal_data.event_mutex);
    min_heap_push_(&time_internal_data.minheap_t, event_data);
    if (event_data->idx == 0) {
        is_need_notify = true;
    }
    pthread_mutex_unlock(&time_internal_data.event_mutex);
    if (is_need_notify) {
        write(time_internal_data.th_notify_fd[1],"1", 1);
    }
    return;
}

static int timeout_next(struct timeval *tv_p) {
	struct timeval now;
	struct timeval *tv = tv_p;
	int res = 0;
    pthread_mutex_lock(&time_internal_data.event_mutex);
    do {
        struct time_event *  data_base = min_heap_top_(&time_internal_data.minheap_t);
        if (data_base == NULL) {
            tv_p = NULL;
            res = -1;
            break;
        }

        if (gettime(&now) == -1) {
            res = -1;
            break;
        }

        if (evutil_timercmp(&data_base->ev_timeout, &now, <=)) {
            evutil_timerclear(tv);
            res = 3;
            break;
        }
        evutil_timersub(&data_base->ev_timeout, &now, tv);
    } while(0);
    pthread_mutex_unlock(&time_internal_data.event_mutex);
	return (res);
}

static void * timer_dispatch(void *args) {
    UNUSED(args);
    struct timeval default_time = {.tv_sec = 0, .tv_usec = 10000}, get_time, wait_time;
    int nret = 0, timer_ret = 0;
    char buf[2] = {0};
    fd_set rset, allset;
    FD_ZERO(&allset);
    FD_SET(time_internal_data.th_notify_fd[0], &allset);
    while(time_internal_data.dis_runing) {
        rset = allset;
        timer_ret = timeout_next(&get_time);
        if (timer_ret == 0 || timer_ret == 3) {
            wait_time = get_time;
        } else {
            wait_time = default_time;
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
#if 0
static uint64_t get_cur_time(void) {
	struct timeb cur_timeb;
	memset(&cur_timeb, 0 , sizeof(cur_timeb));
	ftime(&cur_timeb);
	return (uint64_t)cur_timeb.time * 1000 + cur_timeb.millitm;
}
#endif
void timeout_process(min_heap_t * minheap) {
    if (!minheap) {
        return;
    }
    struct timeval now;

    struct time_event * event_data = NULL;
    bool notify = false;
    gettime(&now);
    pthread_mutex_lock(&(time_internal_data.event_mutex));
    while ((event_data = min_heap_top_(minheap))) {
		if (evutil_timercmp(&event_data->ev_timeout, &now, >)) {
			break;
        }
        notify = false;
        min_heap_erase_(minheap, event_data);

        //printf("time= %llu timeout_process 44444444444\n ", get_cur_time());
        pthread_mutex_unlock(&(time_internal_data.event_mutex));
        pthread_mutex_lock(&(time_internal_data.callback_mutex));
        if (TAILQ_EMPTY(&time_internal_data.evcall_queue_stru_head) || time_internal_data.queue_empty) {
            //printf("time_internal_data.queue_empty = %d \n ",time_internal_data.queue_empty);
            notify = true;
        }
        TAILQ_INSERT_TAIL(&time_internal_data.evcall_queue_stru_head, &(event_data->call_info), evcb_active_next);
        pthread_mutex_unlock(&(time_internal_data.callback_mutex));
        if (notify == true) {
            //printf("time= %llu timeout_process 5555555555555555 \n ",get_cur_time());
            pthread_cond_signal(&(time_internal_data.call_cond));
        }
        //printf("time= %llu timeout_process 666666666666666666 \n ",get_cur_time());
        pthread_mutex_lock(&(time_internal_data.event_mutex));
    }
    pthread_mutex_unlock(&(time_internal_data.event_mutex));
    return;
}

static void * timer_exec_func(void *args) {
    /* Set thread name for profiling and debuging */
	char thread_name[32] = {0};
	snprintf(thread_name, 32, "timer_th_%d", *(int *)args);
#if defined(__linux__)
	prctl(PR_SET_NAME, thread_name);
#elif defined(__APPLE__) && defined(__MACH__)
	pthread_setname_np(thread_name);
#else
	err("timer_exec_func(): pthread_setname_np is not supported on this system");
#endif

    struct event_callback * callback_info = NULL;
    while(time_internal_data.exec_func_runing) {
        pthread_mutex_lock(&(time_internal_data.callback_mutex));
        //printf("timer= %llu pid= %5lu timer_exec_func work thread geted callbcak_mutex 11111111111111111111\n ",get_cur_time(),syscall(SYS_gettid));
        while (TAILQ_EMPTY(&time_internal_data.evcall_queue_stru_head)) {
            time_internal_data.queue_empty++;
          //  printf("timer= %llu pid= %5lu timer_exec_func work thread wait callbcak_mutex 222222222222222\n ",get_cur_time(),syscall(SYS_gettid));
            pthread_cond_wait(&time_internal_data.call_cond, &(time_internal_data.callback_mutex));
           // printf("timer= %llu pid= %5lu timer_exec_func get callback lock********************\n ",get_cur_time(),syscall(SYS_gettid));
            time_internal_data.queue_empty--;
        }
        callback_info = TAILQ_FIRST(&time_internal_data.evcall_queue_stru_head);
        evcb_callback_type cb = callback_info->evcb_callback;
        void * args = callback_info->evcb_arg;
        if (NULL != callback_info) {
            TAILQ_REMOVE(&time_internal_data.evcall_queue_stru_head, callback_info, evcb_active_next);
        }
        //printf("timer= %llu pid= %5lu timer_exec_func work thread begin working 3333333333333333\n ",get_cur_time(),syscall(SYS_gettid));
        pthread_mutex_unlock(&(time_internal_data.callback_mutex));
        if (NULL != callback_info) {
            cb(args);
        }
        add_timer_event(callback_info->base);
    }
    return NULL;
}

static void timer_wt_thread_init() {
    time_internal_data.exec_func_runing = true;
    time_internal_data.queue_empty = 0;
    for (int i = 0; i < time_internal_data.timer_exec_th_num; ++i) {
        pthread_create(&(time_internal_data.exec_func_pid[i]), NULL, timer_exec_func, (void *)&i);
    }
    return;
}

void timer_event_init(int timer_exec_th_num) {
    if (timer_exec_th_num <= 0) {
        timer_exec_th_num = 1;
    }
    TAILQ_INIT(&time_internal_data.evcall_queue_stru_head);
    time_internal_data.timer_exec_th_num = timer_exec_th_num;
    time_internal_data.exec_func_pid = calloc(1, sizeof(pthread_t) * time_internal_data.timer_exec_th_num);
    if (time_internal_data.exec_func_pid == NULL) {
        exit(0);
    }
    timer_wt_thread_init();
    time_internal_data.dis_runing = true;
    min_heap_ctor_(&time_internal_data.minheap_t);
    min_heap_reserve_(&time_internal_data.minheap_t, 100);
    pthread_mutex_init(&time_internal_data.event_mutex, NULL);
    pthread_mutex_init(&time_internal_data.callback_mutex, NULL);
    pthread_cond_init(&time_internal_data.call_cond, NULL);
    time_internal_data.th_notify_fd[0] = -1;
    time_internal_data.th_notify_fd[1] = -1;
    if (pipe(time_internal_data.th_notify_fd) < 0) {
        exit(0);
        return;
    }
    pthread_create(&time_internal_data.dispatch_id, NULL, timer_dispatch, NULL);
    return;
}

static void destroy_min_heap_elm(void) {
    for (unsigned idx = 0; idx < time_internal_data.minheap_t.n; ++idx) {
        if (time_internal_data.minheap_t.p[idx]) {
            free(time_internal_data.minheap_t.p[idx]);
        }
    }
    return;
}

void time_event_destroy(void) {
    time_internal_data.dis_runing = false;
    time_internal_data.exec_func_runing = false;
    pthread_join(time_internal_data.dispatch_id, NULL);
    for (int idx = 0; idx < time_internal_data.timer_exec_th_num; ++idx) {
        if (time_internal_data.dispatch_id != 0) {
            pthread_join(time_internal_data.exec_func_pid[idx], NULL);
        }
    }
    free(time_internal_data.exec_func_pid);
    destroy_min_heap_elm();
    min_heap_dtor_(&(time_internal_data.minheap_t));
    pthread_mutex_destroy(&(time_internal_data.event_mutex));
    pthread_mutex_destroy(&(time_internal_data.callback_mutex));
    pthread_cond_destroy(&(time_internal_data.call_cond));

    if (time_internal_data.th_notify_fd[1] != -1) {
        close(time_internal_data.th_notify_fd[1]);
    }
    if (time_internal_data.th_notify_fd[0] != -1) {
        close(time_internal_data.th_notify_fd[0]);
    }
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