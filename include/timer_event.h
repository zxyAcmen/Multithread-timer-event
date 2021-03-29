#ifndef __TIMER_EVENT_H__
#define __TIMER_EVENT_H__
#include <stdlib.h>
#include <stdbool.h>

//system call
void time_event_init(void);
//user call
//add timer
bool add_new_time(void (*cb)(void * args), void *args, struct timeval * use_tv);
// update timer frequency
void update_timer_frequency(struct timeval * use_tv, void (*cb)(void *));
// 删除事件
#endif