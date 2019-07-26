



























#include "event2/event-config.h"

#include <sys/types.h>
#ifdef _EVENT_HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <sys/queue.h>
#include <poll.h>
#include <signal.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "event-internal.h"
#include "evsignal-internal.h"
#include "log-internal.h"
#include "evmap-internal.h"
#include "event2/thread.h"
#include "evthread-internal.h"

struct pollidx {
	int idxplus1;
};

struct pollop {
	int event_count;		
	int nfds;			
	int realloc_copy;		

	struct pollfd *event_set;
	struct pollfd *event_set_copy;
};

static void *poll_init(struct event_base *);
static int poll_add(struct event_base *, int, short old, short events, void *_idx);
static int poll_del(struct event_base *, int, short old, short events, void *_idx);
static int poll_dispatch(struct event_base *, struct timeval *);
static void poll_dealloc(struct event_base *);

const struct eventop pollops = {
	"poll",
	poll_init,
	poll_add,
	poll_del,
	poll_dispatch,
	poll_dealloc,
	0, 
	EV_FEATURE_FDS,
	sizeof(struct pollidx),
};

static void *
poll_init(struct event_base *base)
{
	struct pollop *pollop;

	if (!(pollop = mm_calloc(1, sizeof(struct pollop))))
		return (NULL);

	evsig_init(base);

	return (pollop);
}

#ifdef CHECK_INVARIANTS
static void
poll_check_ok(struct pollop *pop)
{
	int i, idx;
	struct event *ev;

	for (i = 0; i < pop->fd_count; ++i) {
		idx = pop->idxplus1_by_fd[i]-1;
		if (idx < 0)
			continue;
		EVUTIL_ASSERT(pop->event_set[idx].fd == i);
	}
	for (i = 0; i < pop->nfds; ++i) {
		struct pollfd *pfd = &pop->event_set[i];
		EVUTIL_ASSERT(pop->idxplus1_by_fd[pfd->fd] == i+1);
	}
}
#else
#define poll_check_ok(pop)
#endif

static int
poll_dispatch(struct event_base *base, struct timeval *tv)
{
	int res, i, j, nfds;
	long msec = -1;
	struct pollop *pop = base->evbase;
	struct pollfd *event_set;

	poll_check_ok(pop);

	nfds = pop->nfds;

#ifndef _EVENT_DISABLE_THREAD_SUPPORT
	if (base->th_base_lock) {
		




		if (pop->realloc_copy) {
			struct pollfd *tmp = mm_realloc(pop->event_set_copy,
			    pop->event_count * sizeof(struct pollfd));
			if (tmp == NULL) {
				event_warn("realloc");
				return -1;
			}
			pop->event_set_copy = tmp;
			pop->realloc_copy = 0;
		}
		memcpy(pop->event_set_copy, pop->event_set,
		    sizeof(struct pollfd)*nfds);
		event_set = pop->event_set_copy;
	} else {
		event_set = pop->event_set;
	}
#else
	event_set = pop->event_set;
#endif

	if (tv != NULL) {
		msec = evutil_tv_to_msec(tv);
		if (msec < 0 || msec > INT_MAX)
			msec = INT_MAX;
	}

	EVBASE_RELEASE_LOCK(base, th_base_lock);

	res = poll(event_set, nfds, msec);

	EVBASE_ACQUIRE_LOCK(base, th_base_lock);

	if (res == -1) {
		if (errno != EINTR) {
			event_warn("poll");
			return (-1);
		}

		return (0);
	}

	event_debug(("%s: poll reports %d", __func__, res));

	if (res == 0 || nfds == 0)
		return (0);

	i = random() % nfds;
	for (j = 0; j < nfds; j++) {
		int what;
		if (++i == nfds)
			i = 0;
		what = event_set[i].revents;
		if (!what)
			continue;

		res = 0;

		
		if (what & (POLLHUP|POLLERR))
			what |= POLLIN|POLLOUT;
		if (what & POLLIN)
			res |= EV_READ;
		if (what & POLLOUT)
			res |= EV_WRITE;
		if (res == 0)
			continue;

		evmap_io_active(base, event_set[i].fd, res);
	}

	return (0);
}

static int
poll_add(struct event_base *base, int fd, short old, short events, void *_idx)
{
	struct pollop *pop = base->evbase;
	struct pollfd *pfd = NULL;
	struct pollidx *idx = _idx;
	int i;

	EVUTIL_ASSERT((events & EV_SIGNAL) == 0);
	if (!(events & (EV_READ|EV_WRITE)))
		return (0);

	poll_check_ok(pop);
	if (pop->nfds + 1 >= pop->event_count) {
		struct pollfd *tmp_event_set;
		int tmp_event_count;

		if (pop->event_count < 32)
			tmp_event_count = 32;
		else
			tmp_event_count = pop->event_count * 2;

		
		tmp_event_set = mm_realloc(pop->event_set,
				 tmp_event_count * sizeof(struct pollfd));
		if (tmp_event_set == NULL) {
			event_warn("realloc");
			return (-1);
		}
		pop->event_set = tmp_event_set;

		pop->event_count = tmp_event_count;
		pop->realloc_copy = 1;
	}

	i = idx->idxplus1 - 1;

	if (i >= 0) {
		pfd = &pop->event_set[i];
	} else {
		i = pop->nfds++;
		pfd = &pop->event_set[i];
		pfd->events = 0;
		pfd->fd = fd;
		idx->idxplus1 = i + 1;
	}

	pfd->revents = 0;
	if (events & EV_WRITE)
		pfd->events |= POLLOUT;
	if (events & EV_READ)
		pfd->events |= POLLIN;
	poll_check_ok(pop);

	return (0);
}





static int
poll_del(struct event_base *base, int fd, short old, short events, void *_idx)
{
	struct pollop *pop = base->evbase;
	struct pollfd *pfd = NULL;
	struct pollidx *idx = _idx;
	int i;

	EVUTIL_ASSERT((events & EV_SIGNAL) == 0);
	if (!(events & (EV_READ|EV_WRITE)))
		return (0);

	poll_check_ok(pop);
	i = idx->idxplus1 - 1;
	if (i < 0)
		return (-1);

	
	pfd = &pop->event_set[i];
	if (events & EV_READ)
		pfd->events &= ~POLLIN;
	if (events & EV_WRITE)
		pfd->events &= ~POLLOUT;
	poll_check_ok(pop);
	if (pfd->events)
		
		return (0);

	
	idx->idxplus1 = 0;

	--pop->nfds;
	if (i != pop->nfds) {
		



		memcpy(&pop->event_set[i], &pop->event_set[pop->nfds],
		       sizeof(struct pollfd));
		idx = evmap_io_get_fdinfo(&base->io, pop->event_set[i].fd);
		EVUTIL_ASSERT(idx);
		EVUTIL_ASSERT(idx->idxplus1 == pop->nfds + 1);
		idx->idxplus1 = i + 1;
	}

	poll_check_ok(pop);
	return (0);
}

static void
poll_dealloc(struct event_base *base)
{
	struct pollop *pop = base->evbase;

	evsig_dealloc(base);
	if (pop->event_set)
		mm_free(pop->event_set);
	if (pop->event_set_copy)
		mm_free(pop->event_set_copy);

	memset(pop, 0, sizeof(struct pollop));
	mm_free(pop);
}
