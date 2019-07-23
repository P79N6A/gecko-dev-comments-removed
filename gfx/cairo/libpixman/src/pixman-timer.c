




















#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include "pixman-private.h"

static PixmanTimer *timers;

static void
dump_timers (void)
{
    PixmanTimer *timer;

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
pixman_timer_register (PixmanTimer *timer)
{
    static int initialized;

    int atexit(void (*function)(void));

    if (!initialized)
    {
	atexit (dump_timers);
	initialized = 1;
    }
    
    timer->next = timers;
    timers = timer;
}
