#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/timeb.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <syscall.h>
#include <stdint.h>

#include "timer_event.h"

static uint64_t get_cur_time(void)
{
	struct timeb cur_timeb;
	memset(&cur_timeb, 0 , sizeof(cur_timeb));
	ftime(&cur_timeb);
	return (uint64_t)cur_timeb.time * 1000 + cur_timeb.millitm;
}
void test_print(void * c) {
    printf("timer= %lu pid= %5lu test entry  ========111111111==========\n", get_cur_time(),syscall(SYS_gettid));
    //printf(" test leave timer= %lu ========111111111==========\n",get_cur_time());
    return ;
}

void test_print1(void * c) {
    printf("timer= %lu pid= %5lu test1 entry  ========222222222==========\n",get_cur_time(),syscall(SYS_gettid));
    //printf(" test1 leave timer= %lu ========222222222==========\n",get_cur_time());
    return ;
}

void test_print2(void * c) {
    printf("timer= %lu pid= %5lu test2 entry  ========333333333==========\n",  get_cur_time(),syscall(SYS_gettid));
    //printf(" test2 leave timer= %lu ========333333333==========\n",get_cur_time());
    return ;
}


void test_print3(void * c) {
    printf("timer= %lu pid= %5lu test3 entry  ========4444444444==========\n", get_cur_time(),syscall(SYS_gettid));
    //printf(" test3 leave timer= %lu ========44444444444==========\n",get_cur_time());
    return ;
}


void test_print4(void * c) {
    printf("timer= %lu pid= %5lu test4 entry  ========55555555555==========\n",  get_cur_time(),syscall(SYS_gettid));
    //printf(" test4 leave timer= %lu ========5555555555555555==========\n",get_cur_time());
    return ;
}


void test_print5(void * c) {
    printf("timer= %lu pid= %5lu test5 entry  ========6666666666666==========\n", get_cur_time(),syscall(SYS_gettid));
    //printf(" test5 leave timer= %lu ========6666666666666666666==========\n",get_cur_time());
    return ;
}

void test_print6(void * c) {
    printf("timer= %lu pid= %5lu test6 entry  ========777777777==========\n",  get_cur_time(),syscall(SYS_gettid));
    //printf(" test6 leave timer= %lu ========7777777777777==========\n",get_cur_time());
    return ;
}


void test_print7(void * c) {
    printf("timer= %lu pid= %5lu test7 entry  ========8888888888==========\n", get_cur_time(),syscall(SYS_gettid));
    //printf(" test7 leave timer= %lu ========888888888888==========\n",get_cur_time());
    return ;
}



void test_print8(void * c) {
    printf("timer= %lu pid= %5lu test8 entry  ========999999999999==========\n", get_cur_time(),syscall(SYS_gettid));
    //printf(" test8 leave timer= %lu ========9999999999==========\n",get_cur_time());
    return ;
}



void test_print9(void * c) {
    printf("timer= %lu pid= %5lu test9 entry  ========10101010101010==========\n", get_cur_time(),syscall(SYS_gettid));
   // printf(" test9 leave timer= %lu ========10101010101010==========\n",get_cur_time());
    return ;
}



void test_print10(void * c) {
    printf("timer= %lu pid= %5lu test10 entry  ========11-11-11-11-11-11==========\n",  get_cur_time(),syscall(SYS_gettid));
    //printf(" test10 leave timer= %lu ========11-11-11-11-11-11==========\n",get_cur_time());
    return ;
}

int main(int argc, char **argv)  
{
    //system call
    timer_event_init(1);
    struct timeval timer1;
    timer1.tv_sec = 0;
    timer1.tv_usec = 2000;//100000 才是100ms
    timer1.tv_sec = 0;
    timer1.tv_usec = 2000;//100000 才是100ms
    timer1.tv_sec = 0;
    add_new_timer(test_print, NULL, &timer1);
    timer1.tv_usec = 4000;//100000 才是100ms
    add_new_timer(test_print1, NULL, &timer1);
    timer1.tv_usec = 6000;//100000 才是100ms
    add_new_timer(test_print2, NULL, &timer1);
    timer1.tv_usec = 8000;//100000 才是100ms
    add_new_timer(test_print3, NULL, &timer1);
    timer1.tv_usec = 10000;//100000 才是100ms
    add_new_timer(test_print4, NULL, &timer1);
    timer1.tv_usec = 12000;//100000 才是100ms
    add_new_timer(test_print5, NULL, &timer1);
    timer1.tv_usec = 14000;//100000 才是100ms
    add_new_timer(test_print6, NULL, &timer1);
    timer1.tv_usec = 16000;//100000 才是100ms
    add_new_timer(test_print7, NULL, &timer1);
    timer1.tv_usec = 18000;//100000 才是100ms
    add_new_timer(test_print8, NULL, &timer1);
/*
    add_new_timer(test_print9, NULL, &timer1);
    add_new_timer(test_print10, NULL, &timer1);
    */
    while (1)
    {
        sleep(12);
    }
    
    return 0;
}