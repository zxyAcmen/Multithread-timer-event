#ifndef __MIN_HEAP_H__
#define __MIN_HEAP_H__

#include <stdlib.h>
#include <stdio.h>

struct event_callback;
struct time_event;
struct min_heap;
typedef struct min_heap min_heap_t;

#define	evutil_timercmp(tvp, uvp, cmp)		\
	(((tvp)->tv_sec == (uvp)->tv_sec) ?		\
	 ((tvp)->tv_usec cmp (uvp)->tv_usec) :	\
	 ((tvp)->tv_sec cmp (uvp)->tv_sec))

#define	evutil_timercmp(tvp, uvp, cmp)		\
	(((tvp)->tv_sec == (uvp)->tv_sec) ?		\
	 ((tvp)->tv_usec cmp (uvp)->tv_usec) :	\
	 ((tvp)->tv_sec cmp (uvp)->tv_sec))

#define min_heap_elem_greater(a, b) \
	(evutil_timercmp(&(a)->ev_timeout, &(b)->ev_timeout, >))

#define	evutil_timerclear(tvp)	(tvp)->tv_sec = (tvp)->tv_usec = 0

#define	evutil_timersub(tvp, uvp, vvp)					\
	do {								\
		(vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;		\
		(vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec;	\
		if ((vvp)->tv_usec < 0) {				\
			(vvp)->tv_sec--;				\
			(vvp)->tv_usec += 1000000;			\
		}							\
	} while (0)

#define evutil_timeradd(tvp, uvp, vvp)					\
	do {								\
		(vvp)->tv_sec = (tvp)->tv_sec + (uvp)->tv_sec;		\
		(vvp)->tv_usec = (tvp)->tv_usec + (uvp)->tv_usec;       \
		if ((vvp)->tv_usec >= 1000000) {			\
			(vvp)->tv_sec++;				\
			(vvp)->tv_usec -= 1000000;			\
		}							\
	} while (0)

void min_heap_ctor_(min_heap_t* s);
void min_heap_dtor_(min_heap_t* s);
void min_heap_elem_init_(struct time_event* e);
int min_heap_empty_(min_heap_t* s);
unsigned min_heap_size_(min_heap_t* s);
struct time_event* min_heap_top_(min_heap_t* s);
int min_heap_push_(min_heap_t* s, struct time_event* e);
struct time_event* min_heap_pop_(min_heap_t* s);
int min_heap_elt_is_top_(const struct time_event *e);
int min_heap_erase_(min_heap_t* s, struct time_event* e);
int min_heap_adjust_(min_heap_t *s, struct time_event *e);
int min_heap_reserve_(min_heap_t* s, unsigned n);
void min_heap_shift_up_unconditional_(min_heap_t* s, unsigned hole_index, struct time_event* e);
void min_heap_shift_up_(min_heap_t* s, unsigned hole_index, struct time_event* e);
void min_heap_shift_down_(min_heap_t* s, unsigned hole_index, struct time_event* e);
void min_heap_update_elm(min_heap_t* s, unsigned sec, unsigned usec, void (*cb)(void *));
#endif //__TABLE_INTERNAL_H__
