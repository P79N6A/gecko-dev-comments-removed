

























#include "event2/event-config.h"

#include <stdint.h>
#include <sys/types.h>
#include <sys/resource.h>
#ifdef _EVENT_HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <sys/queue.h>
#include <sys/epoll.h>
#include <signal.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#ifdef _EVENT_HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "event-internal.h"
#include "evsignal-internal.h"
#include "event2/thread.h"
#include "evthread-internal.h"
#include "log-internal.h"
#include "evmap-internal.h"
#include "changelist-internal.h"

struct epollop {
	struct epoll_event *events;
	int nevents;
	int epfd;
};

static void *epoll_init(struct event_base *);
static int epoll_dispatch(struct event_base *, struct timeval *);
static void epoll_dealloc(struct event_base *);

static const struct eventop epollops_changelist = {
	"epoll (with changelist)",
	epoll_init,
	event_changelist_add,
	event_changelist_del,
	epoll_dispatch,
	epoll_dealloc,
	1, 
	EV_FEATURE_ET|EV_FEATURE_O1,
	EVENT_CHANGELIST_FDINFO_SIZE
};


static int epoll_nochangelist_add(struct event_base *base, evutil_socket_t fd,
    short old, short events, void *p);
static int epoll_nochangelist_del(struct event_base *base, evutil_socket_t fd,
    short old, short events, void *p);

const struct eventop epollops = {
	"epoll",
	epoll_init,
	epoll_nochangelist_add,
	epoll_nochangelist_del,
	epoll_dispatch,
	epoll_dealloc,
	1, 
	EV_FEATURE_ET|EV_FEATURE_O1,
	0
};

#define INITIAL_NEVENT 32
#define MAX_NEVENT 4096







#define MAX_EPOLL_TIMEOUT_MSEC (35*60*1000)

static void *
epoll_init(struct event_base *base)
{
	int epfd;
	struct epollop *epollop;

	

	if ((epfd = epoll_create(32000)) == -1) {
		if (errno != ENOSYS)
			event_warn("epoll_create");
		return (NULL);
	}

	evutil_make_socket_closeonexec(epfd);

	if (!(epollop = mm_calloc(1, sizeof(struct epollop)))) {
		close(epfd);
		return (NULL);
	}

	epollop->epfd = epfd;

	
	epollop->events = mm_calloc(INITIAL_NEVENT, sizeof(struct epoll_event));
	if (epollop->events == NULL) {
		mm_free(epollop);
		close(epfd);
		return (NULL);
	}
	epollop->nevents = INITIAL_NEVENT;

	if ((base->flags & EVENT_BASE_FLAG_EPOLL_USE_CHANGELIST) != 0 ||
	    ((base->flags & EVENT_BASE_FLAG_IGNORE_ENV) == 0 &&
		evutil_getenv("EVENT_EPOLL_USE_CHANGELIST") != NULL))
		base->evsel = &epollops_changelist;

	evsig_init(base);

	return (epollop);
}

static const char *
change_to_string(int change)
{
	change &= (EV_CHANGE_ADD|EV_CHANGE_DEL);
	if (change == EV_CHANGE_ADD) {
		return "add";
	} else if (change == EV_CHANGE_DEL) {
		return "del";
	} else if (change == 0) {
		return "none";
	} else {
		return "???";
	}
}

static const char *
epoll_op_to_string(int op)
{
	return op == EPOLL_CTL_ADD?"ADD":
	    op == EPOLL_CTL_DEL?"DEL":
	    op == EPOLL_CTL_MOD?"MOD":
	    "???";
}

static int
epoll_apply_one_change(struct event_base *base,
    struct epollop *epollop,
    const struct event_change *ch)
{
	struct epoll_event epev;
	int op, events = 0;

	if (1) {
		









		

		if ((ch->read_change & EV_CHANGE_ADD) ||
		    (ch->write_change & EV_CHANGE_ADD)) {
			

			events = 0;
			op = EPOLL_CTL_ADD;
			if (ch->read_change & EV_CHANGE_ADD) {
				events |= EPOLLIN;
			} else if (ch->read_change & EV_CHANGE_DEL) {
				;
			} else if (ch->old_events & EV_READ) {
				events |= EPOLLIN;
			}
			if (ch->write_change & EV_CHANGE_ADD) {
				events |= EPOLLOUT;
			} else if (ch->write_change & EV_CHANGE_DEL) {
				;
			} else if (ch->old_events & EV_WRITE) {
				events |= EPOLLOUT;
			}
			if ((ch->read_change|ch->write_change) & EV_ET)
				events |= EPOLLET;

			if (ch->old_events) {
				













				op = EPOLL_CTL_MOD;
			}
		} else if ((ch->read_change & EV_CHANGE_DEL) ||
		    (ch->write_change & EV_CHANGE_DEL)) {
			

			op = EPOLL_CTL_DEL;

			if (ch->read_change & EV_CHANGE_DEL) {
				if (ch->write_change & EV_CHANGE_DEL) {
					events = EPOLLIN|EPOLLOUT;
				} else if (ch->old_events & EV_WRITE) {
					events = EPOLLOUT;
					op = EPOLL_CTL_MOD;
				} else {
					events = EPOLLIN;
				}
			} else if (ch->write_change & EV_CHANGE_DEL) {
				if (ch->old_events & EV_READ) {
					events = EPOLLIN;
					op = EPOLL_CTL_MOD;
				} else {
					events = EPOLLOUT;
				}
			}
		}

		if (!events)
			return 0;

		memset(&epev, 0, sizeof(epev));
		epev.data.fd = ch->fd;
		epev.events = events;
		if (epoll_ctl(epollop->epfd, op, ch->fd, &epev) == -1) {
			if (op == EPOLL_CTL_MOD && errno == ENOENT) {
				



				if (epoll_ctl(epollop->epfd, EPOLL_CTL_ADD, ch->fd, &epev) == -1) {
					event_warn("Epoll MOD(%d) on %d retried as ADD; that failed too",
					    (int)epev.events, ch->fd);
					return -1;
				} else {
					event_debug(("Epoll MOD(%d) on %d retried as ADD; succeeded.",
						(int)epev.events,
						ch->fd));
				}
			} else if (op == EPOLL_CTL_ADD && errno == EEXIST) {
				






				if (epoll_ctl(epollop->epfd, EPOLL_CTL_MOD, ch->fd, &epev) == -1) {
					event_warn("Epoll ADD(%d) on %d retried as MOD; that failed too",
					    (int)epev.events, ch->fd);
					return -1;
				} else {
					event_debug(("Epoll ADD(%d) on %d retried as MOD; succeeded.",
						(int)epev.events,
						ch->fd));
				}
			} else if (op == EPOLL_CTL_DEL &&
			    (errno == ENOENT || errno == EBADF ||
				errno == EPERM)) {
				


				event_debug(("Epoll DEL(%d) on fd %d gave %s: DEL was unnecessary.",
					(int)epev.events,
					ch->fd,
					strerror(errno)));
			} else {
				event_warn("Epoll %s(%d) on fd %d failed.  Old events were %d; read change was %d (%s); write change was %d (%s)",
				    epoll_op_to_string(op),
				    (int)epev.events,
				    ch->fd,
				    ch->old_events,
				    ch->read_change,
				    change_to_string(ch->read_change),
				    ch->write_change,
				    change_to_string(ch->write_change));
				return -1;
			}
		} else {
			event_debug(("Epoll %s(%d) on fd %d okay. [old events were %d; read change was %d; write change was %d]",
				epoll_op_to_string(op),
				(int)epev.events,
				(int)ch->fd,
				ch->old_events,
				ch->read_change,
				ch->write_change));
		}
	}
	return 0;
}

static int
epoll_apply_changes(struct event_base *base)
{
	struct event_changelist *changelist = &base->changelist;
	struct epollop *epollop = base->evbase;
	struct event_change *ch;

	int r = 0;
	int i;

	for (i = 0; i < changelist->n_changes; ++i) {
		ch = &changelist->changes[i];
		if (epoll_apply_one_change(base, epollop, ch) < 0)
			r = -1;
	}

	return (r);
}

static int
epoll_nochangelist_add(struct event_base *base, evutil_socket_t fd,
    short old, short events, void *p)
{
	struct event_change ch;
	ch.fd = fd;
	ch.old_events = old;
	ch.read_change = ch.write_change = 0;
	if (events & EV_WRITE)
		ch.write_change = EV_CHANGE_ADD |
		    (events & EV_ET);
	if (events & EV_READ)
		ch.read_change = EV_CHANGE_ADD |
		    (events & EV_ET);

	return epoll_apply_one_change(base, base->evbase, &ch);
}

static int
epoll_nochangelist_del(struct event_base *base, evutil_socket_t fd,
    short old, short events, void *p)
{
	struct event_change ch;
	ch.fd = fd;
	ch.old_events = old;
	ch.read_change = ch.write_change = 0;
	if (events & EV_WRITE)
		ch.write_change = EV_CHANGE_DEL;
	if (events & EV_READ)
		ch.read_change = EV_CHANGE_DEL;

	return epoll_apply_one_change(base, base->evbase, &ch);
}

static int
epoll_dispatch(struct event_base *base, struct timeval *tv)
{
	struct epollop *epollop = base->evbase;
	struct epoll_event *events = epollop->events;
	int i, res;
	long timeout = -1;

	if (tv != NULL) {
		timeout = evutil_tv_to_msec(tv);
		if (timeout < 0 || timeout > MAX_EPOLL_TIMEOUT_MSEC) {
			

			timeout = MAX_EPOLL_TIMEOUT_MSEC;
		}
	}

	epoll_apply_changes(base);
	event_changelist_remove_all(&base->changelist, base);

	EVBASE_RELEASE_LOCK(base, th_base_lock);

	res = epoll_wait(epollop->epfd, events, epollop->nevents, timeout);

	EVBASE_ACQUIRE_LOCK(base, th_base_lock);

	if (res == -1) {
		if (errno != EINTR) {
			event_warn("epoll_wait");
			return (-1);
		}

		return (0);
	}

	event_debug(("%s: epoll_wait reports %d", __func__, res));
	EVUTIL_ASSERT(res <= epollop->nevents);

	for (i = 0; i < res; i++) {
		int what = events[i].events;
		short ev = 0;

		if (what & (EPOLLHUP|EPOLLERR)) {
			ev = EV_READ | EV_WRITE;
		} else {
			if (what & EPOLLIN)
				ev |= EV_READ;
			if (what & EPOLLOUT)
				ev |= EV_WRITE;
		}

		if (!ev)
			continue;

		evmap_io_active(base, events[i].data.fd, ev | EV_ET);
	}

	if (res == epollop->nevents && epollop->nevents < MAX_NEVENT) {
		

		int new_nevents = epollop->nevents * 2;
		struct epoll_event *new_events;

		new_events = mm_realloc(epollop->events,
		    new_nevents * sizeof(struct epoll_event));
		if (new_events) {
			epollop->events = new_events;
			epollop->nevents = new_nevents;
		}
	}

	return (0);
}


static void
epoll_dealloc(struct event_base *base)
{
	struct epollop *epollop = base->evbase;

	evsig_dealloc(base);
	if (epollop->events)
		mm_free(epollop->events);
	if (epollop->epfd >= 0)
		close(epollop->epfd);

	memset(epollop, 0, sizeof(struct epollop));
	mm_free(epollop);
}
