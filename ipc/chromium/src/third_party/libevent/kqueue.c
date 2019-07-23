



























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <sys/_time.h>
#endif
#include <sys/queue.h>
#include <sys/event.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif




#if defined(HAVE_INTTYPES_H) && !defined(__OpenBSD__) && !defined(__FreeBSD__) && !defined(__darwin__) && !defined(__APPLE__)
#define PTR_TO_UDATA(x)	((intptr_t)(x))
#else
#define PTR_TO_UDATA(x)	(x)
#endif

#include "event.h"
#include "event-internal.h"
#include "log.h"
#include "event-internal.h"

#define EVLIST_X_KQINKERNEL	0x1000

#define NEVENT		64

struct kqop {
	struct kevent *changes;
	int nchanges;
	struct kevent *events;
	struct event_list evsigevents[NSIG];
	int nevents;
	int kq;
	pid_t pid;
};

static void *kq_init	(struct event_base *);
static int kq_add	(void *, struct event *);
static int kq_del	(void *, struct event *);
static int kq_dispatch	(struct event_base *, void *, struct timeval *);
static int kq_insert	(struct kqop *, struct kevent *);
static void kq_dealloc (struct event_base *, void *);

const struct eventop kqops = {
	"kqueue",
	kq_init,
	kq_add,
	kq_del,
	kq_dispatch,
	kq_dealloc,
	1 
};

static void *
kq_init(struct event_base *base)
{
	int i, kq;
	struct kqop *kqueueop;

	
	if (getenv("EVENT_NOKQUEUE"))
		return (NULL);

	if (!(kqueueop = calloc(1, sizeof(struct kqop))))
		return (NULL);

	
	
	if ((kq = kqueue()) == -1) {
		event_warn("kqueue");
		free (kqueueop);
		return (NULL);
	}

	kqueueop->kq = kq;

	kqueueop->pid = getpid();

	
	kqueueop->changes = malloc(NEVENT * sizeof(struct kevent));
	if (kqueueop->changes == NULL) {
		free (kqueueop);
		return (NULL);
	}
	kqueueop->events = malloc(NEVENT * sizeof(struct kevent));
	if (kqueueop->events == NULL) {
		free (kqueueop->changes);
		free (kqueueop);
		return (NULL);
	}
	kqueueop->nevents = NEVENT;

	
	for (i = 0; i < NSIG; ++i) {
		TAILQ_INIT(&kqueueop->evsigevents[i]);
	}

	
	kqueueop->changes[0].ident = -1;
	kqueueop->changes[0].filter = EVFILT_READ;
	kqueueop->changes[0].flags = EV_ADD;
	




	if (kevent(kq,
		kqueueop->changes, 1, kqueueop->events, NEVENT, NULL) != 1 ||
	    kqueueop->events[0].ident != -1 ||
	    kqueueop->events[0].flags != EV_ERROR) {
		event_warn("%s: detected broken kqueue; not using.", __func__);
		free(kqueueop->changes);
		free(kqueueop->events);
		free(kqueueop);
		close(kq);
		return (NULL);
	}

	return (kqueueop);
}

static int
kq_insert(struct kqop *kqop, struct kevent *kev)
{
	int nevents = kqop->nevents;

	if (kqop->nchanges == nevents) {
		struct kevent *newchange;
		struct kevent *newresult;

		nevents *= 2;

		newchange = realloc(kqop->changes,
				    nevents * sizeof(struct kevent));
		if (newchange == NULL) {
			event_warn("%s: malloc", __func__);
			return (-1);
		}
		kqop->changes = newchange;

		newresult = realloc(kqop->events,
				    nevents * sizeof(struct kevent));

		



		if (newresult == NULL) {
			event_warn("%s: malloc", __func__);
			return (-1);
		}
		kqop->events = newresult;

		kqop->nevents = nevents;
	}

	memcpy(&kqop->changes[kqop->nchanges++], kev, sizeof(struct kevent));

	event_debug(("%s: fd %d %s%s",
		__func__, (int)kev->ident, 
		kev->filter == EVFILT_READ ? "EVFILT_READ" : "EVFILT_WRITE",
		kev->flags == EV_DELETE ? " (del)" : ""));

	return (0);
}

static void
kq_sighandler(int sig)
{
	
}

static int
kq_dispatch(struct event_base *base, void *arg, struct timeval *tv)
{
	struct kqop *kqop = arg;
	struct kevent *changes = kqop->changes;
	struct kevent *events = kqop->events;
	struct event *ev;
	struct timespec ts, *ts_p = NULL;
	int i, res;

	if (tv != NULL) {
		TIMEVAL_TO_TIMESPEC(tv, &ts);
		ts_p = &ts;
	}

	res = kevent(kqop->kq, changes, kqop->nchanges,
	    events, kqop->nevents, ts_p);
	kqop->nchanges = 0;
	if (res == -1) {
		if (errno != EINTR) {
                        event_warn("kevent");
			return (-1);
		}

		return (0);
	}

	event_debug(("%s: kevent reports %d", __func__, res));

	for (i = 0; i < res; i++) {
		int which = 0;

		if (events[i].flags & EV_ERROR) {
			











			if (events[i].data == EBADF ||
			    events[i].data == EINVAL ||
			    events[i].data == ENOENT)
				continue;
			errno = events[i].data;
			return (-1);
		}

		if (events[i].filter == EVFILT_READ) {
			which |= EV_READ;
		} else if (events[i].filter == EVFILT_WRITE) {
			which |= EV_WRITE;
		} else if (events[i].filter == EVFILT_SIGNAL) {
			which |= EV_SIGNAL;
		}

		if (!which)
			continue;

		if (events[i].filter == EVFILT_SIGNAL) {
			struct event_list *head =
			    (struct event_list *)events[i].udata;
			TAILQ_FOREACH(ev, head, ev_signal_next) {
				event_active(ev, which, events[i].data);
			}
		} else {
			ev = (struct event *)events[i].udata;

			if (!(ev->ev_events & EV_PERSIST))
				ev->ev_flags &= ~EVLIST_X_KQINKERNEL;

			event_active(ev, which, 1);
		}
	}

	return (0);
}


static int
kq_add(void *arg, struct event *ev)
{
	struct kqop *kqop = arg;
	struct kevent kev;

	if (ev->ev_events & EV_SIGNAL) {
		int nsignal = EVENT_SIGNAL(ev);

		assert(nsignal >= 0 && nsignal < NSIG);
		if (TAILQ_EMPTY(&kqop->evsigevents[nsignal])) {
			struct timespec timeout = { 0, 0 };
			
			memset(&kev, 0, sizeof(kev));
			kev.ident = nsignal;
			kev.filter = EVFILT_SIGNAL;
			kev.flags = EV_ADD;
			kev.udata = PTR_TO_UDATA(&kqop->evsigevents[nsignal]);
			
			


			if (kevent(kqop->kq, &kev, 1, NULL, 0, &timeout) == -1)
				return (-1);
			
			if (_evsignal_set_handler(ev->ev_base, nsignal,
				kq_sighandler) == -1)
				return (-1);
		}

		TAILQ_INSERT_TAIL(&kqop->evsigevents[nsignal], ev,
		    ev_signal_next);
		ev->ev_flags |= EVLIST_X_KQINKERNEL;
		return (0);
	}

	if (ev->ev_events & EV_READ) {
 		memset(&kev, 0, sizeof(kev));
		kev.ident = ev->ev_fd;
		kev.filter = EVFILT_READ;
#ifdef NOTE_EOF
		
		kev.fflags = NOTE_EOF;
#endif
		kev.flags = EV_ADD;
		if (!(ev->ev_events & EV_PERSIST))
			kev.flags |= EV_ONESHOT;
		kev.udata = PTR_TO_UDATA(ev);
		
		if (kq_insert(kqop, &kev) == -1)
			return (-1);

		ev->ev_flags |= EVLIST_X_KQINKERNEL;
	}

	if (ev->ev_events & EV_WRITE) {
 		memset(&kev, 0, sizeof(kev));
		kev.ident = ev->ev_fd;
		kev.filter = EVFILT_WRITE;
		kev.flags = EV_ADD;
		if (!(ev->ev_events & EV_PERSIST))
			kev.flags |= EV_ONESHOT;
		kev.udata = PTR_TO_UDATA(ev);
		
		if (kq_insert(kqop, &kev) == -1)
			return (-1);

		ev->ev_flags |= EVLIST_X_KQINKERNEL;
	}

	return (0);
}

static int
kq_del(void *arg, struct event *ev)
{
	struct kqop *kqop = arg;
	struct kevent kev;

	if (!(ev->ev_flags & EVLIST_X_KQINKERNEL))
		return (0);

	if (ev->ev_events & EV_SIGNAL) {
		int nsignal = EVENT_SIGNAL(ev);
		struct timespec timeout = { 0, 0 };

		assert(nsignal >= 0 && nsignal < NSIG);
		TAILQ_REMOVE(&kqop->evsigevents[nsignal], ev, ev_signal_next);
		if (TAILQ_EMPTY(&kqop->evsigevents[nsignal])) {
			memset(&kev, 0, sizeof(kev));
			kev.ident = nsignal;
			kev.filter = EVFILT_SIGNAL;
			kev.flags = EV_DELETE;
		
			


			if (kevent(kqop->kq, &kev, 1, NULL, 0, &timeout) == -1)
				return (-1);

			if (_evsignal_restore_handler(ev->ev_base,
				nsignal) == -1)
				return (-1);
		}

		ev->ev_flags &= ~EVLIST_X_KQINKERNEL;
		return (0);
	}

	if (ev->ev_events & EV_READ) {
 		memset(&kev, 0, sizeof(kev));
		kev.ident = ev->ev_fd;
		kev.filter = EVFILT_READ;
		kev.flags = EV_DELETE;
		
		if (kq_insert(kqop, &kev) == -1)
			return (-1);

		ev->ev_flags &= ~EVLIST_X_KQINKERNEL;
	}

	if (ev->ev_events & EV_WRITE) {
 		memset(&kev, 0, sizeof(kev));
		kev.ident = ev->ev_fd;
		kev.filter = EVFILT_WRITE;
		kev.flags = EV_DELETE;
		
		if (kq_insert(kqop, &kev) == -1)
			return (-1);

		ev->ev_flags &= ~EVLIST_X_KQINKERNEL;
	}

	return (0);
}

static void
kq_dealloc(struct event_base *base, void *arg)
{
	struct kqop *kqop = arg;

	if (kqop->changes)
		free(kqop->changes);
	if (kqop->events)
		free(kqop->events);
	if (kqop->kq >= 0 && kqop->pid == getpid())
		close(kqop->kq);
	memset(kqop, 0, sizeof(struct kqop));
	free(kqop);
}
