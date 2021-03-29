#include <stdlib.h>
#include <stdio.h>

#include "timer_event.h"
#include "min_heap.h"
#include "timer_struct.h"

void min_heap_ctor_(min_heap_t* s) { s->p = 0; s->n = 0; s->a = 0; }
void min_heap_dtor_(min_heap_t* s) { if (s->p) free(s->p); }
void min_heap_elem_init_(struct time_event* e) { e->idx = -1; }
int min_heap_empty_(min_heap_t* s) { return 0u == s->n; }
unsigned min_heap_size_(min_heap_t* s) { return s->n; }
struct time_event * min_heap_top_(min_heap_t* s) { return s->n ? *s->p : 0; }

int min_heap_push_(min_heap_t* s, struct time_event* e) {
	if (s->n == __UINT32_MAX__ || min_heap_reserve_(s, s->n + 1)) {
		return -1;
    }
	min_heap_shift_up_(s, s->n++, e);
	return 0;
}

struct time_event* min_heap_pop_(min_heap_t* s) {
	if (s->n) {
		struct time_event* e = *s->p;
		min_heap_shift_down_(s, 0u, s->p[--s->n]);
		e->idx = -1;
		return e;
	}
	return 0;
}

int min_heap_elt_is_top_(const struct time_event *e) {
	return e->idx == 0;
}

int min_heap_erase_(min_heap_t* s, struct time_event* e) {
	if (-1 != e->idx) {
		struct time_event *last = s->p[--s->n];
		unsigned parent = (e->idx - 1) / 2;
		if (e->idx > 0 && min_heap_elem_greater(s->p[parent], last)) {
			min_heap_shift_up_unconditional_(s, e->idx, last);
        } else {
			min_heap_shift_down_(s, e->idx, last);
        }
		e->idx = -1;
		return 0;
	}
	return -1;
}

int min_heap_adjust_(min_heap_t *s, struct time_event *e) {
	if (-1 == e->idx) {
		return min_heap_push_(s, e);
	} else {
		unsigned parent = (e->idx - 1) / 2;
		if (e->idx > 0 && min_heap_elem_greater(s->p[parent], e)) {
			min_heap_shift_up_unconditional_(s, e->idx, e);
        } else {
			min_heap_shift_down_(s, e->idx, e);
        }
		return 0;
	}
}

int min_heap_reserve_(min_heap_t* s, unsigned n) {
	if (s->a < n) {
		struct time_event** p;
		unsigned a = s->a ? s->a * 2 : 8;
		if (a < n) {
			a = n;
        }
		if (!(p = (struct time_event**)realloc(s->p, a * sizeof *p))) {
			return -1;
        }
		s->p = p;
		s->a = a;
	}
	return 0;
}

void min_heap_shift_up_unconditional_(min_heap_t* s, unsigned hole_index, struct time_event* e) {
    unsigned parent = (hole_index - 1) / 2;
    do {
        (s->p[hole_index] = s->p[parent])->idx = hole_index;
        hole_index = parent;
        parent = (hole_index - 1) / 2;
    } while (hole_index && min_heap_elem_greater(s->p[parent], e));
    (s->p[hole_index] = e)->idx = hole_index;
}

void min_heap_shift_up_(min_heap_t* s, unsigned hole_index, struct time_event* e) {
    unsigned parent = (hole_index - 1) / 2;
    while (hole_index && min_heap_elem_greater(s->p[parent], e)) {
        (s->p[hole_index] = s->p[parent])->idx = hole_index;
        hole_index = parent;
        parent = (hole_index - 1) / 2;
    }
    (s->p[hole_index] = e)->idx = hole_index;
}

void min_heap_shift_down_(min_heap_t* s, unsigned hole_index, struct time_event* e) {
    unsigned min_child = 2 * (hole_index + 1);
    while (min_child <= s->n) {
		min_child -= min_child == s->n || min_heap_elem_greater(s->p[min_child], s->p[min_child - 1]);
		if (!(min_heap_elem_greater(e, s->p[min_child]))) {
			break;
        }
		(s->p[hole_index] = s->p[min_child])->idx = hole_index;
		hole_index = min_child;
		min_child = 2 * (hole_index + 1);
	}
    (s->p[hole_index] = e)->idx = hole_index;
}

void min_heap_update_elm(min_heap_t* s, unsigned sec, unsigned usec, void (*cb)(void *)) {
	for (unsigned idx = 0; idx < s->n; ++idx) {
		if (s->p[idx]->call_info.evcb_callback == cb) {
			s->p[idx]->delay_time.tv_sec = sec;
			s->p[idx]->delay_time.tv_usec = usec;
			return;
		}
	}
	return;
}
