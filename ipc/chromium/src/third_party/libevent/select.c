



























#include "event2/event-config.h"

#include <sys/types.h>
#ifdef _EVENT_HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef _EVENT_HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#include <sys/queue.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "event-internal.h"
#include "evsignal-internal.h"
#include "event2/thread.h"
#include "evthread-internal.h"
#include "log-internal.h"
#include "evmap-internal.h"

#ifndef _EVENT_HAVE_FD_MASK

typedef unsigned long fd_mask;
#endif

#ifndef NFDBITS
#define NFDBITS (sizeof(fd_mask)*8)
#endif


#define DIV_ROUNDUP(x, y)   (((x)+((y)-1))/(y))


#define SELECT_ALLOC_SIZE(n) \
	(DIV_ROUNDUP(n, NFDBITS) * sizeof(fd_mask))

struct selectop {
	int event_fds;		
	int event_fdsz;
	int resize_out_sets;
	fd_set *event_readset_in;
	fd_set *event_writeset_in;
	fd_set *event_readset_out;
	fd_set *event_writeset_out;
};

static void *select_init(struct event_base *);
static int select_add(struct event_base *, int, short old, short events, void*);
static int select_del(struct event_base *, int, short old, short events, void*);
static int select_dispatch(struct event_base *, struct timeval *);
static void select_dealloc(struct event_base *);

const struct eventop selectops = {
	"select",
	select_init,
	select_add,
	select_del,
	select_dispatch,
	select_dealloc,
	0, 
	EV_FEATURE_FDS,
	0,
};

static int select_resize(struct selectop *sop, int fdsz);
static void select_free_selectop(struct selectop *sop);

static void *
select_init(struct event_base *base)
{
	struct selectop *sop;

	if (!(sop = mm_calloc(1, sizeof(struct selectop))))
		return (NULL);

	if (select_resize(sop, SELECT_ALLOC_SIZE(32 + 1))) {
		select_free_selectop(sop);
		return (NULL);
	}

	evsig_init(base);

	return (sop);
}

#ifdef CHECK_INVARIANTS
static void
check_selectop(struct selectop *sop)
{
	
}
#else
#define check_selectop(sop) do { (void) sop; } while (0)
#endif

static int
select_dispatch(struct event_base *base, struct timeval *tv)
{
	int res=0, i, j, nfds;
	struct selectop *sop = base->evbase;

	check_selectop(sop);
	if (sop->resize_out_sets) {
		fd_set *readset_out=NULL, *writeset_out=NULL;
		size_t sz = sop->event_fdsz;
		if (!(readset_out = mm_realloc(sop->event_readset_out, sz)))
			return (-1);
		sop->event_readset_out = readset_out;
		if (!(writeset_out = mm_realloc(sop->event_writeset_out, sz))) {
			



			return (-1);
		}
		sop->event_writeset_out = writeset_out;
		sop->resize_out_sets = 0;
	}

	memcpy(sop->event_readset_out, sop->event_readset_in,
	       sop->event_fdsz);
	memcpy(sop->event_writeset_out, sop->event_writeset_in,
	       sop->event_fdsz);

	nfds = sop->event_fds+1;

	EVBASE_RELEASE_LOCK(base, th_base_lock);

	res = select(nfds, sop->event_readset_out,
	    sop->event_writeset_out, NULL, tv);

	EVBASE_ACQUIRE_LOCK(base, th_base_lock);

	check_selectop(sop);

	if (res == -1) {
		if (errno != EINTR) {
			event_warn("select");
			return (-1);
		}

		return (0);
	}

	event_debug(("%s: select reports %d", __func__, res));

	check_selectop(sop);
	i = random() % nfds;
	for (j = 0; j < nfds; ++j) {
		if (++i >= nfds)
			i = 0;
		res = 0;
		if (FD_ISSET(i, sop->event_readset_out))
			res |= EV_READ;
		if (FD_ISSET(i, sop->event_writeset_out))
			res |= EV_WRITE;

		if (res == 0)
			continue;

		evmap_io_active(base, i, res);
	}
	check_selectop(sop);

	return (0);
}

static int
select_resize(struct selectop *sop, int fdsz)
{
	fd_set *readset_in = NULL;
	fd_set *writeset_in = NULL;

	if (sop->event_readset_in)
		check_selectop(sop);

	if ((readset_in = mm_realloc(sop->event_readset_in, fdsz)) == NULL)
		goto error;
	sop->event_readset_in = readset_in;
	if ((writeset_in = mm_realloc(sop->event_writeset_in, fdsz)) == NULL) {
		





		goto error;
	}
	sop->event_writeset_in = writeset_in;
	sop->resize_out_sets = 1;

	memset((char *)sop->event_readset_in + sop->event_fdsz, 0,
	    fdsz - sop->event_fdsz);
	memset((char *)sop->event_writeset_in + sop->event_fdsz, 0,
	    fdsz - sop->event_fdsz);

	sop->event_fdsz = fdsz;
	check_selectop(sop);

	return (0);

 error:
	event_warn("malloc");
	return (-1);
}


static int
select_add(struct event_base *base, int fd, short old, short events, void *p)
{
	struct selectop *sop = base->evbase;
	(void) p;

	EVUTIL_ASSERT((events & EV_SIGNAL) == 0);
	check_selectop(sop);
	



	if (sop->event_fds < fd) {
		int fdsz = sop->event_fdsz;

		if (fdsz < (int)sizeof(fd_mask))
			fdsz = (int)sizeof(fd_mask);

		


		while (fdsz < (int) SELECT_ALLOC_SIZE(fd + 1))
			fdsz *= 2;

		if (fdsz != sop->event_fdsz) {
			if (select_resize(sop, fdsz)) {
				check_selectop(sop);
				return (-1);
			}
		}

		sop->event_fds = fd;
	}

	if (events & EV_READ)
		FD_SET(fd, sop->event_readset_in);
	if (events & EV_WRITE)
		FD_SET(fd, sop->event_writeset_in);
	check_selectop(sop);

	return (0);
}





static int
select_del(struct event_base *base, int fd, short old, short events, void *p)
{
	struct selectop *sop = base->evbase;
	(void)p;

	EVUTIL_ASSERT((events & EV_SIGNAL) == 0);
	check_selectop(sop);

	if (sop->event_fds < fd) {
		check_selectop(sop);
		return (0);
	}

	if (events & EV_READ)
		FD_CLR(fd, sop->event_readset_in);

	if (events & EV_WRITE)
		FD_CLR(fd, sop->event_writeset_in);

	check_selectop(sop);
	return (0);
}

static void
select_free_selectop(struct selectop *sop)
{
	if (sop->event_readset_in)
		mm_free(sop->event_readset_in);
	if (sop->event_writeset_in)
		mm_free(sop->event_writeset_in);
	if (sop->event_readset_out)
		mm_free(sop->event_readset_out);
	if (sop->event_writeset_out)
		mm_free(sop->event_writeset_out);

	memset(sop, 0, sizeof(struct selectop));
	mm_free(sop);
}

static void
select_dealloc(struct event_base *base)
{
	evsig_dealloc(base);

	select_free_selectop(base->evbase);
}
