



























#include "event2/event-config.h"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#endif
#include <sys/types.h>
#ifdef _EVENT_HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <sys/queue.h>
#ifdef _EVENT_HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _EVENT_HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>
#ifdef _EVENT_HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "event2/event.h"
#include "event2/event_struct.h"
#include "event-internal.h"
#include "event2/util.h"
#include "evsignal-internal.h"
#include "log-internal.h"
#include "evmap-internal.h"
#include "evthread-internal.h"



















#ifndef WIN32


#define __cdecl
#endif

static int evsig_add(struct event_base *, evutil_socket_t, short, short, void *);
static int evsig_del(struct event_base *, evutil_socket_t, short, short, void *);

static const struct eventop evsigops = {
	"signal",
	NULL,
	evsig_add,
	evsig_del,
	NULL,
	NULL,
	0, 0, 0
};

#ifndef _EVENT_DISABLE_THREAD_SUPPORT

static void *evsig_base_lock = NULL;
#endif

static struct event_base *evsig_base = NULL;

static int evsig_base_n_signals_added = 0;
static evutil_socket_t evsig_base_fd = -1;

static void __cdecl evsig_handler(int sig);

#define EVSIGBASE_LOCK() EVLOCK_LOCK(evsig_base_lock, 0)
#define EVSIGBASE_UNLOCK() EVLOCK_UNLOCK(evsig_base_lock, 0)

void
evsig_set_base(struct event_base *base)
{
	EVSIGBASE_LOCK();
	evsig_base = base;
	evsig_base_n_signals_added = base->sig.ev_n_signals_added;
	evsig_base_fd = base->sig.ev_signal_pair[0];
	EVSIGBASE_UNLOCK();
}


static void
evsig_cb(evutil_socket_t fd, short what, void *arg)
{
	static char signals[1024];
	ev_ssize_t n;
	int i;
	int ncaught[NSIG];
	struct event_base *base;

	base = arg;

	memset(&ncaught, 0, sizeof(ncaught));

	while (1) {
		n = recv(fd, signals, sizeof(signals), 0);
		if (n == -1) {
			int err = evutil_socket_geterror(fd);
			if (! EVUTIL_ERR_RW_RETRIABLE(err))
				event_sock_err(1, fd, "%s: recv", __func__);
			break;
		} else if (n == 0) {
			
			break;
		}
		for (i = 0; i < n; ++i) {
			ev_uint8_t sig = signals[i];
			if (sig < NSIG)
				ncaught[sig]++;
		}
	}

	EVBASE_ACQUIRE_LOCK(base, th_base_lock);
	for (i = 0; i < NSIG; ++i) {
		if (ncaught[i])
			evmap_signal_active(base, i, ncaught[i]);
	}
	EVBASE_RELEASE_LOCK(base, th_base_lock);
}

int
evsig_init(struct event_base *base)
{
	




	if (evutil_socketpair(
		    AF_UNIX, SOCK_STREAM, 0, base->sig.ev_signal_pair) == -1) {
#ifdef WIN32
		

		event_sock_warn(-1, "%s: socketpair", __func__);
#else
		event_sock_err(1, -1, "%s: socketpair", __func__);
#endif
		return -1;
	}

	evutil_make_socket_closeonexec(base->sig.ev_signal_pair[0]);
	evutil_make_socket_closeonexec(base->sig.ev_signal_pair[1]);
	base->sig.sh_old = NULL;
	base->sig.sh_old_max = 0;

	evutil_make_socket_nonblocking(base->sig.ev_signal_pair[0]);
	evutil_make_socket_nonblocking(base->sig.ev_signal_pair[1]);

	event_assign(&base->sig.ev_signal, base, base->sig.ev_signal_pair[1],
		EV_READ | EV_PERSIST, evsig_cb, base);

	base->sig.ev_signal.ev_flags |= EVLIST_INTERNAL;
	event_priority_set(&base->sig.ev_signal, 0);

	base->evsigsel = &evsigops;

	return 0;
}



int
_evsig_set_handler(struct event_base *base,
    int evsignal, void (__cdecl *handler)(int))
{
#ifdef _EVENT_HAVE_SIGACTION
	struct sigaction sa;
#else
	ev_sighandler_t sh;
#endif
	struct evsig_info *sig = &base->sig;
	void *p;

	



	if (evsignal >= sig->sh_old_max) {
		int new_max = evsignal + 1;
		event_debug(("%s: evsignal (%d) >= sh_old_max (%d), resizing",
			    __func__, evsignal, sig->sh_old_max));
		p = mm_realloc(sig->sh_old, new_max * sizeof(*sig->sh_old));
		if (p == NULL) {
			event_warn("realloc");
			return (-1);
		}

		memset((char *)p + sig->sh_old_max * sizeof(*sig->sh_old),
		    0, (new_max - sig->sh_old_max) * sizeof(*sig->sh_old));

		sig->sh_old_max = new_max;
		sig->sh_old = p;
	}

	
	sig->sh_old[evsignal] = mm_malloc(sizeof *sig->sh_old[evsignal]);
	if (sig->sh_old[evsignal] == NULL) {
		event_warn("malloc");
		return (-1);
	}

	
#ifdef _EVENT_HAVE_SIGACTION
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handler;
	sa.sa_flags |= SA_RESTART;
	sigfillset(&sa.sa_mask);

	if (sigaction(evsignal, &sa, sig->sh_old[evsignal]) == -1) {
		event_warn("sigaction");
		mm_free(sig->sh_old[evsignal]);
		sig->sh_old[evsignal] = NULL;
		return (-1);
	}
#else
	if ((sh = signal(evsignal, handler)) == SIG_ERR) {
		event_warn("signal");
		mm_free(sig->sh_old[evsignal]);
		sig->sh_old[evsignal] = NULL;
		return (-1);
	}
	*sig->sh_old[evsignal] = sh;
#endif

	return (0);
}

static int
evsig_add(struct event_base *base, evutil_socket_t evsignal, short old, short events, void *p)
{
	struct evsig_info *sig = &base->sig;
	(void)p;

	EVUTIL_ASSERT(evsignal >= 0 && evsignal < NSIG);

	
	EVSIGBASE_LOCK();
	if (evsig_base != base && evsig_base_n_signals_added) {
		event_warnx("Added a signal to event base %p with signals "
		    "already added to event_base %p.  Only one can have "
		    "signals at a time with the %s backend.  The base with "
		    "the most recently added signal or the most recent "
		    "event_base_loop() call gets preference; do "
		    "not rely on this behavior in future Libevent versions.",
		    base, evsig_base, base->evsel->name);
	}
	evsig_base = base;
	evsig_base_n_signals_added = ++sig->ev_n_signals_added;
	evsig_base_fd = base->sig.ev_signal_pair[0];
	EVSIGBASE_UNLOCK();

	event_debug(("%s: %d: changing signal handler", __func__, (int)evsignal));
	if (_evsig_set_handler(base, (int)evsignal, evsig_handler) == -1) {
		goto err;
	}


	if (!sig->ev_signal_added) {
		if (event_add(&sig->ev_signal, NULL))
			goto err;
		sig->ev_signal_added = 1;
	}

	return (0);

err:
	EVSIGBASE_LOCK();
	--evsig_base_n_signals_added;
	--sig->ev_n_signals_added;
	EVSIGBASE_UNLOCK();
	return (-1);
}

int
_evsig_restore_handler(struct event_base *base, int evsignal)
{
	int ret = 0;
	struct evsig_info *sig = &base->sig;
#ifdef _EVENT_HAVE_SIGACTION
	struct sigaction *sh;
#else
	ev_sighandler_t *sh;
#endif

	
	sh = sig->sh_old[evsignal];
	sig->sh_old[evsignal] = NULL;
#ifdef _EVENT_HAVE_SIGACTION
	if (sigaction(evsignal, sh, NULL) == -1) {
		event_warn("sigaction");
		ret = -1;
	}
#else
	if (signal(evsignal, *sh) == SIG_ERR) {
		event_warn("signal");
		ret = -1;
	}
#endif

	mm_free(sh);

	return ret;
}

static int
evsig_del(struct event_base *base, evutil_socket_t evsignal, short old, short events, void *p)
{
	EVUTIL_ASSERT(evsignal >= 0 && evsignal < NSIG);

	event_debug(("%s: "EV_SOCK_FMT": restoring signal handler",
		__func__, EV_SOCK_ARG(evsignal)));

	EVSIGBASE_LOCK();
	--evsig_base_n_signals_added;
	--base->sig.ev_n_signals_added;
	EVSIGBASE_UNLOCK();

	return (_evsig_restore_handler(base, (int)evsignal));
}

static void __cdecl
evsig_handler(int sig)
{
	int save_errno = errno;
#ifdef WIN32
	int socket_errno = EVUTIL_SOCKET_ERROR();
#endif
	ev_uint8_t msg;

	if (evsig_base == NULL) {
		event_warnx(
			"%s: received signal %d, but have no base configured",
			__func__, sig);
		return;
	}

#ifndef _EVENT_HAVE_SIGACTION
	signal(sig, evsig_handler);
#endif

	
	msg = sig;
	send(evsig_base_fd, (char*)&msg, 1, 0);
	errno = save_errno;
#ifdef WIN32
	EVUTIL_SET_SOCKET_ERROR(socket_errno);
#endif
}

void
evsig_dealloc(struct event_base *base)
{
	int i = 0;
	if (base->sig.ev_signal_added) {
		event_del(&base->sig.ev_signal);
		base->sig.ev_signal_added = 0;
	}
	

	event_debug_unassign(&base->sig.ev_signal);

	for (i = 0; i < NSIG; ++i) {
		if (i < base->sig.sh_old_max && base->sig.sh_old[i] != NULL)
			_evsig_restore_handler(base, i);
	}
	EVSIGBASE_LOCK();
	if (base == evsig_base) {
		evsig_base = NULL;
		evsig_base_n_signals_added = 0;
		evsig_base_fd = -1;
	}
	EVSIGBASE_UNLOCK();

	if (base->sig.ev_signal_pair[0] != -1) {
		evutil_closesocket(base->sig.ev_signal_pair[0]);
		base->sig.ev_signal_pair[0] = -1;
	}
	if (base->sig.ev_signal_pair[1] != -1) {
		evutil_closesocket(base->sig.ev_signal_pair[1]);
		base->sig.ev_signal_pair[1] = -1;
	}
	base->sig.sh_old_max = 0;

	
	if (base->sig.sh_old) {
		mm_free(base->sig.sh_old);
		base->sig.sh_old = NULL;
	}
}

#ifndef _EVENT_DISABLE_THREAD_SUPPORT
int
evsig_global_setup_locks_(const int enable_locks)
{
	EVTHREAD_SETUP_GLOBAL_LOCK(evsig_base_lock, 0);
	return 0;
}
#endif
