#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/timeb.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "timer_event.h"
static uint64_t get_cur_time(void)
{
	struct timeb cur_timeb;
	memset(&cur_timeb, 0 , sizeof(cur_timeb));
	ftime(&cur_timeb);
	return (uint64_t)cur_timeb.time * 1000 + cur_timeb.millitm;
}
void test_print(void * c) {
    printf(" test entry timer= %lu ========111111111==========\n",get_cur_time());
    printf(" test leave timer= %lu ========111111111==========\n",get_cur_time());
    return ;
}

void test_print1(void * c) {
    printf(" test1 entry timer= %lu ========222222222==========\n",get_cur_time());
    printf(" test1 leave timer= %lu ========222222222==========\n",get_cur_time());
    return ;
}

void test_print2(void * c) {
    printf(" test2 entry timer= %lu ========333333333==========\n",get_cur_time());
    printf(" test2 leave timer= %lu ========333333333==========\n",get_cur_time());
    return ;
}


int main(int argc, char **argv)  
{
    //system call
    time_event_init(3);
    struct timeval timer1;
    timer1.tv_sec = 0;
    timer1.tv_usec = 10000;//100000 才是100ms
    add_new_timer(test_print, NULL, &timer1);
    timer1.tv_sec = 0;
    timer1.tv_usec = 20000;//100000 才是100ms
    add_new_timer(test_print1, NULL, &timer1);
    timer1.tv_sec = 0;
    timer1.tv_usec = 30000;//100000 才是100ms
    add_new_timer(test_print2, NULL, &timer1);
    while (1)
    {
        sleep(12);
    }
    
    return 0;
}