



































#include "cairoint.h"

void
_cairo_observers_notify (cairo_list_t *observers, void *arg)
{
    cairo_observer_t *obs, *next;

    cairo_list_foreach_entry_safe (obs, next,
				   cairo_observer_t,
				   observers, link)
    {
	obs->callback (obs, arg);
    }
}
