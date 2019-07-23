

























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/resource.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <sys/_time.h>
#endif
#include <sys/queue.h>
#include <sys/devpoll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>

#include "event.h"
#include "event-internal.h"
#include "evsignal.h"
#include "log.h"




struct evdevpoll {
	struct event *evread;
	struct event *evwrite;
};

struct devpollop {
	struct evdevpoll *fds;
	int nfds;
	struct pollfd *events;
	int nevents;
	int dpfd;
	struct pollfd *changes;
	int nchanges;
};

static void *devpoll_init	(struct event_base *);
static int devpoll_add	(void *, struct event *);
static int devpoll_del	(void *, struct event *);
static int devpoll_dispatch	(struct event_base *, void *, struct timeval *);
static void devpoll_dealloc	(struct event_base *, void *);

struct eventop devpollops = {
	"devpoll",
	devpoll_init,
	devpoll_add,
	devpoll_del,
	devpoll_dispatch,
	devpoll_dealloc,
	1 
};

#define NEVENT	32000

static int
devpoll_commit(struct devpollop *devpollop)
{
	



	if (pwrite(devpollop->dpfd, devpollop->changes,
		sizeof(struct pollfd) * devpollop->nchanges, 0) == -1)
		return(-1);

	devpollop->nchanges = 0;
	return(0);
}

static int
devpoll_queue(struct devpollop *devpollop, int fd, int events) {
	struct pollfd *pfd;

	if (devpollop->nchanges >= devpollop->nevents) {
		



		if (devpoll_commit(devpollop) != 0)
			return(-1);
	}

	pfd = &devpollop->changes[devpollop->nchanges++];
	pfd->fd = fd;
	pfd->events = events;
	pfd->revents = 0;

	return(0);
}

static void *
devpoll_init(struct event_base *base)
{
	int dpfd, nfiles = NEVENT;
	struct rlimit rl;
	struct devpollop *devpollop;

	
	if (getenv("EVENT_NODEVPOLL"))
		return (NULL);

	if (!(devpollop = calloc(1, sizeof(struct devpollop))))
		return (NULL);

	if (getrlimit(RLIMIT_NOFILE, &rl) == 0 &&
	    rl.rlim_cur != RLIM_INFINITY)
		nfiles = rl.rlim_cur;

	
	if ((dpfd = open("/dev/poll", O_RDWR)) == -1) {
                event_warn("open: /dev/poll");
		free(devpollop);
		return (NULL);
	}

	devpollop->dpfd = dpfd;

	
	devpollop->events = calloc(nfiles, sizeof(struct pollfd));
	if (devpollop->events == NULL) {
		free(devpollop);
		close(dpfd);
		return (NULL);
	}
	devpollop->nevents = nfiles;

	devpollop->fds = calloc(nfiles, sizeof(struct evdevpoll));
	if (devpollop->fds == NULL) {
		free(devpollop->events);
		free(devpollop);
		close(dpfd);
		return (NULL);
	}
	devpollop->nfds = nfiles;

	devpollop->changes = calloc(nfiles, sizeof(struct pollfd));
	if (devpollop->changes == NULL) {
		free(devpollop->fds);
		free(devpollop->events);
		free(devpollop);
		close(dpfd);
		return (NULL);
	}

	evsignal_init(base);

	return (devpollop);
}

static int
devpoll_recalc(struct event_base *base, void *arg, int max)
{
	struct devpollop *devpollop = arg;

	if (max >= devpollop->nfds) {
		struct evdevpoll *fds;
		int nfds;

		nfds = devpollop->nfds;
		while (nfds <= max)
			nfds <<= 1;

		fds = realloc(devpollop->fds, nfds * sizeof(struct evdevpoll));
		if (fds == NULL) {
			event_warn("realloc");
			return (-1);
		}
		devpollop->fds = fds;
		memset(fds + devpollop->nfds, 0,
		    (nfds - devpollop->nfds) * sizeof(struct evdevpoll));
		devpollop->nfds = nfds;
	}

	return (0);
}

static int
devpoll_dispatch(struct event_base *base, void *arg, struct timeval *tv)
{
	struct devpollop *devpollop = arg;
	struct pollfd *events = devpollop->events;
	struct dvpoll dvp;
	struct evdevpoll *evdp;
	int i, res, timeout = -1;

	if (devpollop->nchanges)
		devpoll_commit(devpollop);

	if (tv != NULL)
		timeout = tv->tv_sec * 1000 + (tv->tv_usec + 999) / 1000;

	dvp.dp_fds = devpollop->events;
	dvp.dp_nfds = devpollop->nevents;
	dvp.dp_timeout = timeout;

	res = ioctl(devpollop->dpfd, DP_POLL, &dvp);

	if (res == -1) {
		if (errno != EINTR) {
			event_warn("ioctl: DP_POLL");
			return (-1);
		}

		evsignal_process(base);
		return (0);
	} else if (base->sig.evsignal_caught) {
		evsignal_process(base);
	}

	event_debug(("%s: devpoll_wait reports %d", __func__, res));

	for (i = 0; i < res; i++) {
		int which = 0;
		int what = events[i].revents;
		struct event *evread = NULL, *evwrite = NULL;

		assert(events[i].fd < devpollop->nfds);
		evdp = &devpollop->fds[events[i].fd];
   
                if (what & POLLHUP)
                        what |= POLLIN | POLLOUT;
                else if (what & POLLERR)
                        what |= POLLIN | POLLOUT;

		if (what & POLLIN) {
			evread = evdp->evread;
			which |= EV_READ;
		}

		if (what & POLLOUT) {
			evwrite = evdp->evwrite;
			which |= EV_WRITE;
		}

		if (!which)
			continue;

		if (evread != NULL && !(evread->ev_events & EV_PERSIST))
			event_del(evread);
		if (evwrite != NULL && evwrite != evread &&
		    !(evwrite->ev_events & EV_PERSIST))
			event_del(evwrite);

		if (evread != NULL)
			event_active(evread, EV_READ, 1);
		if (evwrite != NULL)
			event_active(evwrite, EV_WRITE, 1);
	}

	return (0);
}


static int
devpoll_add(void *arg, struct event *ev)
{
	struct devpollop *devpollop = arg;
	struct evdevpoll *evdp;
	int fd, events;

	if (ev->ev_events & EV_SIGNAL)
		return (evsignal_add(ev));

	fd = ev->ev_fd;
	if (fd >= devpollop->nfds) {
		
		if (devpoll_recalc(ev->ev_base, devpollop, fd) == -1)
			return (-1);
	}
	evdp = &devpollop->fds[fd];

	






	events = 0;
	if (ev->ev_events & EV_READ) {
		if (evdp->evread && evdp->evread != ev) {
		   
		   return(-1);
		}
		events |= POLLIN;
	}

	if (ev->ev_events & EV_WRITE) {
		if (evdp->evwrite && evdp->evwrite != ev) {
		   
		   return(-1);
		}
		events |= POLLOUT;
	}

	if (devpoll_queue(devpollop, fd, events) != 0)
		return(-1);

	
	if (ev->ev_events & EV_READ)
		evdp->evread = ev;
	if (ev->ev_events & EV_WRITE)
		evdp->evwrite = ev;

	return (0);
}

static int
devpoll_del(void *arg, struct event *ev)
{
	struct devpollop *devpollop = arg;
	struct evdevpoll *evdp;
	int fd, events;
	int needwritedelete = 1, needreaddelete = 1;

	if (ev->ev_events & EV_SIGNAL)
		return (evsignal_del(ev));

	fd = ev->ev_fd;
	if (fd >= devpollop->nfds)
		return (0);
	evdp = &devpollop->fds[fd];

	events = 0;
	if (ev->ev_events & EV_READ)
		events |= POLLIN;
	if (ev->ev_events & EV_WRITE)
		events |= POLLOUT;

	






	if (devpoll_queue(devpollop, fd, POLLREMOVE) != 0)
		return(-1);

	if ((events & (POLLIN|POLLOUT)) != (POLLIN|POLLOUT)) {
		




		if ((events & POLLIN) && evdp->evwrite != NULL) {
			
			devpoll_queue(devpollop, fd, POLLOUT);
			needwritedelete = 0;
		} else if ((events & POLLOUT) && evdp->evread != NULL) {
			
			devpoll_queue(devpollop, fd, POLLIN);
			needreaddelete = 0;
		}
	}

	if (needreaddelete)
		evdp->evread = NULL;
	if (needwritedelete)
		evdp->evwrite = NULL;

	return (0);
}

static void
devpoll_dealloc(struct event_base *base, void *arg)
{
	struct devpollop *devpollop = arg;

	evsignal_dealloc(base);
	if (devpollop->fds)
		free(devpollop->fds);
	if (devpollop->events)
		free(devpollop->events);
	if (devpollop->changes)
		free(devpollop->changes);
	if (devpollop->dpfd >= 0)
		close(devpollop->dpfd);

	memset(devpollop, 0, sizeof(struct devpollop));
	free(devpollop);
}
