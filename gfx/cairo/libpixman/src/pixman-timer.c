




















#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include "pixman-private.h"

#ifdef PIXMAN_TIMERS

static pixman_timer_t *timers;

static void
dump_timers (void)
{
    pixman_timer_t *timer;

    for (timer = timers; timer != NULL; timer = timer->next)
    {
	printf ("%s:   total: %llu     n: %llu      avg: %f\n",
	        timer->name,
	        timer->total,
	        timer->n_times,
	        timer->total / (double)timer->n_times);
    }
}

void
pixman_timer_register (pixman_timer_t *timer)
{
    static int initialized;

    int atexit (void (*function)(void));

    if (!initialized)
    {
	atexit (dump_timers);
	initialized = 1;
    }

    timer->next = timers;
    timers = timer;
}

#endif
