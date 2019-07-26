






















































#include "event2/event-config.h"

#include <sys/time.h>
#include <sys/queue.h>
#include <errno.h>
#include <poll.h>
#include <port.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "event2/thread.h"

#include "evthread-internal.h"
#include "event-internal.h"
#include "log-internal.h"
#include "evsignal-internal.h"
#include "evmap-internal.h"






#define DEFAULT_NFDS	16







#define EVENTS_PER_GETN 8






struct fd_info {
	short fdi_what;		
};

#define FDI_HAS_READ(fdi)  ((fdi)->fdi_what & EV_READ)
#define FDI_HAS_WRITE(fdi) ((fdi)->fdi_what & EV_WRITE)
#define FDI_HAS_EVENTS(fdi) (FDI_HAS_READ(fdi) || FDI_HAS_WRITE(fdi))
#define FDI_TO_SYSEVENTS(fdi) (FDI_HAS_READ(fdi) ? POLLIN : 0) | \
    (FDI_HAS_WRITE(fdi) ? POLLOUT : 0)

struct evport_data {
	int		ed_port;	
	int		ed_nevents;	
	struct fd_info *ed_fds;		
	
	int ed_pending[EVENTS_PER_GETN]; 
};

static void*	evport_init(struct event_base *);
static int evport_add(struct event_base *, int fd, short old, short events, void *);
static int evport_del(struct event_base *, int fd, short old, short events, void *);
static int	evport_dispatch(struct event_base *, struct timeval *);
static void	evport_dealloc(struct event_base *);

const struct eventop evportops = {
	"evport",
	evport_init,
	evport_add,
	evport_del,
	evport_dispatch,
	evport_dealloc,
	1, 
	0, 
	0, 
};





static void*
evport_init(struct event_base *base)
{
	struct evport_data *evpd;
	int i;

	if (!(evpd = mm_calloc(1, sizeof(struct evport_data))))
		return (NULL);

	if ((evpd->ed_port = port_create()) == -1) {
		mm_free(evpd);
		return (NULL);
	}

	


	evpd->ed_fds = mm_calloc(DEFAULT_NFDS, sizeof(struct fd_info));
	if (evpd->ed_fds == NULL) {
		close(evpd->ed_port);
		mm_free(evpd);
		return (NULL);
	}
	evpd->ed_nevents = DEFAULT_NFDS;
	for (i = 0; i < EVENTS_PER_GETN; i++)
		evpd->ed_pending[i] = -1;

	evsig_init(base);

	return (evpd);
}

#ifdef CHECK_INVARIANTS






static void
check_evportop(struct evport_data *evpd)
{
	EVUTIL_ASSERT(evpd);
	EVUTIL_ASSERT(evpd->ed_nevents > 0);
	EVUTIL_ASSERT(evpd->ed_port > 0);
	EVUTIL_ASSERT(evpd->ed_fds > 0);
}




static void
check_event(port_event_t* pevt)
{
	





	EVUTIL_ASSERT(pevt->portev_source == PORT_SOURCE_FD);
	EVUTIL_ASSERT(pevt->portev_user == NULL);
}

#else
#define check_evportop(epop)
#define check_event(pevt)
#endif 




static int
grow(struct evport_data *epdp, int factor)
{
	struct fd_info *tmp;
	int oldsize = epdp->ed_nevents;
	int newsize = factor * oldsize;
	EVUTIL_ASSERT(factor > 1);

	check_evportop(epdp);

	tmp = mm_realloc(epdp->ed_fds, sizeof(struct fd_info) * newsize);
	if (NULL == tmp)
		return -1;
	epdp->ed_fds = tmp;
	memset((char*) (epdp->ed_fds + oldsize), 0,
	    (newsize - oldsize)*sizeof(struct fd_info));
	epdp->ed_nevents = newsize;

	check_evportop(epdp);

	return 0;
}






static int
reassociate(struct evport_data *epdp, struct fd_info *fdip, int fd)
{
	int sysevents = FDI_TO_SYSEVENTS(fdip);

	if (sysevents != 0) {
		if (port_associate(epdp->ed_port, PORT_SOURCE_FD,
				   fd, sysevents, NULL) == -1) {
			event_warn("port_associate");
			return (-1);
		}
	}

	check_evportop(epdp);

	return (0);
}






static int
evport_dispatch(struct event_base *base, struct timeval *tv)
{
	int i, res;
	struct evport_data *epdp = base->evbase;
	port_event_t pevtlist[EVENTS_PER_GETN];

	





	int nevents = 1;

	




	struct timespec ts;
	struct timespec *ts_p = NULL;
	if (tv != NULL) {
		ts.tv_sec = tv->tv_sec;
		ts.tv_nsec = tv->tv_usec * 1000;
		ts_p = &ts;
	}

	




	for (i = 0; i < EVENTS_PER_GETN; ++i) {
		struct fd_info *fdi = NULL;
		if (epdp->ed_pending[i] != -1) {
			fdi = &(epdp->ed_fds[epdp->ed_pending[i]]);
		}

		if (fdi != NULL && FDI_HAS_EVENTS(fdi)) {
			int fd = epdp->ed_pending[i];
			reassociate(epdp, fdi, fd);
			epdp->ed_pending[i] = -1;
		}
	}

	EVBASE_RELEASE_LOCK(base, th_base_lock);

	res = port_getn(epdp->ed_port, pevtlist, EVENTS_PER_GETN,
	    (unsigned int *) &nevents, ts_p);

	EVBASE_ACQUIRE_LOCK(base, th_base_lock);

	if (res == -1) {
		if (errno == EINTR || errno == EAGAIN) {
			return (0);
		} else if (errno == ETIME) {
			if (nevents == 0)
				return (0);
		} else {
			event_warn("port_getn");
			return (-1);
		}
	}

	event_debug(("%s: port_getn reports %d events", __func__, nevents));

	for (i = 0; i < nevents; ++i) {
		struct fd_info *fdi;
		port_event_t *pevt = &pevtlist[i];
		int fd = (int) pevt->portev_object;

		check_evportop(epdp);
		check_event(pevt);
		epdp->ed_pending[i] = fd;

		



		res = 0;
		if (pevt->portev_events & (POLLERR|POLLHUP)) {
			res = EV_READ | EV_WRITE;
		} else {
			if (pevt->portev_events & POLLIN)
				res |= EV_READ;
			if (pevt->portev_events & POLLOUT)
				res |= EV_WRITE;
		}

		


		if (pevt->portev_events & (POLLERR|POLLHUP|POLLNVAL))
			res |= EV_READ|EV_WRITE;

		EVUTIL_ASSERT(epdp->ed_nevents > fd);
		fdi = &(epdp->ed_fds[fd]);

		evmap_io_active(base, fd, res);
	} 

	check_evportop(epdp);

	return (0);
}







static int
evport_add(struct event_base *base, int fd, short old, short events, void *p)
{
	struct evport_data *evpd = base->evbase;
	struct fd_info *fdi;
	int factor;
	(void)p;

	check_evportop(evpd);

	



	factor = 1;
	while (fd >= factor * evpd->ed_nevents)
		factor *= 2;

	if (factor > 1) {
		if (-1 == grow(evpd, factor)) {
			return (-1);
		}
	}

	fdi = &evpd->ed_fds[fd];
	fdi->fdi_what |= events;

	return reassociate(evpd, fdi, fd);
}





static int
evport_del(struct event_base *base, int fd, short old, short events, void *p)
{
	struct evport_data *evpd = base->evbase;
	struct fd_info *fdi;
	int i;
	int associated = 1;
	(void)p;

	check_evportop(evpd);

	if (evpd->ed_nevents < fd) {
		return (-1);
	}

	for (i = 0; i < EVENTS_PER_GETN; ++i) {
		if (evpd->ed_pending[i] == fd) {
			associated = 0;
			break;
		}
	}

	fdi = &evpd->ed_fds[fd];
	if (events & EV_READ)
		fdi->fdi_what &= ~EV_READ;
	if (events & EV_WRITE)
		fdi->fdi_what &= ~EV_WRITE;

	if (associated) {
		if (!FDI_HAS_EVENTS(fdi) &&
		    port_dissociate(evpd->ed_port, PORT_SOURCE_FD, fd) == -1) {
			



			if (errno != EBADFD) {
				event_warn("port_dissociate");
				return (-1);
			}
		} else {
			if (FDI_HAS_EVENTS(fdi)) {
				return (reassociate(evpd, fdi, fd));
			}
		}
	} else {
		if ((fdi->fdi_what & (EV_READ|EV_WRITE)) == 0) {
			evpd->ed_pending[i] = -1;
		}
	}
	return 0;
}


static void
evport_dealloc(struct event_base *base)
{
	struct evport_data *evpd = base->evbase;

	evsig_dealloc(base);

	close(evpd->ed_port);

	if (evpd->ed_fds)
		mm_free(evpd->ed_fds);
	mm_free(evpd);
}
