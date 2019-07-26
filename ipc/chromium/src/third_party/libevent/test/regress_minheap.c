

























#include <stdlib.h>
#include "event2/event_struct.h"

#include "tinytest.h"
#include "tinytest_macros.h"
#include "../minheap-internal.h"

static void
set_random_timeout(struct event *ev)
{
	ev->ev_timeout.tv_sec = rand();
	ev->ev_timeout.tv_usec = rand() & 0xfffff;
	ev->ev_timeout_pos.min_heap_idx = -1;
}

static void
check_heap(struct min_heap *heap)
{
	unsigned i;
	for (i = 1; i < heap->n; ++i) {
		unsigned parent_idx = (i-1)/2;
		tt_want(evutil_timercmp(&heap->p[i]->ev_timeout,
			&heap->p[parent_idx]->ev_timeout, >=));
	}
}

static void
test_heap_randomized(void *ptr)
{
	struct min_heap heap;
	struct event *inserted[1024];
	struct event *e, *last_e;
	int i;

	min_heap_ctor(&heap);

	for (i = 0; i < 1024; ++i) {
		inserted[i] = malloc(sizeof(struct event));
		set_random_timeout(inserted[i]);
		min_heap_push(&heap, inserted[i]);
	}
	check_heap(&heap);

	tt_assert(min_heap_size(&heap) == 1024);

	for (i = 0; i < 512; ++i) {
		min_heap_erase(&heap, inserted[i]);
		if (0 == (i % 32))
			check_heap(&heap);
	}
	tt_assert(min_heap_size(&heap) == 512);

	last_e = min_heap_pop(&heap);
	while (1) {
		e = min_heap_pop(&heap);
		if (!e)
			break;
		tt_want(evutil_timercmp(&last_e->ev_timeout,
			&e->ev_timeout, <=));
	}
	tt_assert(min_heap_size(&heap) == 0);
end:
	for (i = 0; i < 1024; ++i)
		free(inserted[i]);

	min_heap_dtor(&heap);
}

struct testcase_t minheap_testcases[] = {
	{ "randomized", test_heap_randomized, 0, NULL, NULL },
	END_OF_TESTCASES
};
