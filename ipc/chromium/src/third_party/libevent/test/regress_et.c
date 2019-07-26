
























#include "../util-internal.h"
#include "event2/event-config.h"

#ifdef WIN32
#include <winsock2.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#ifdef _EVENT_HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifndef WIN32
#include <sys/time.h>
#include <unistd.h>
#endif
#include <errno.h>

#include "event2/event.h"
#include "event2/util.h"

#include "regress.h"

static int was_et = 0;

static void
read_cb(evutil_socket_t fd, short event, void *arg)
{
	char buf;
	int len;

	len = recv(fd, &buf, sizeof(buf), 0);

	called++;
	if (event & EV_ET)
		was_et = 1;

	if (!len)
		event_del(arg);
}

#ifndef SHUT_WR
#define SHUT_WR 1
#endif

#ifdef WIN32
#define LOCAL_SOCKETPAIR_AF AF_INET
#else
#define LOCAL_SOCKETPAIR_AF AF_UNIX
#endif

static void
test_edgetriggered(void *et)
{
	struct event *ev = NULL;
	struct event_base *base = NULL;
	const char *test = "test string";
	evutil_socket_t pair[2] = {-1,-1};
	int supports_et;

	





#ifdef __linux__
	if (evutil_ersatz_socketpair(AF_INET, SOCK_STREAM, 0, pair) == -1) {
		tt_abort_perror("socketpair");
	}
#else
	if (evutil_socketpair(LOCAL_SOCKETPAIR_AF, SOCK_STREAM, 0, pair) == -1) {
		tt_abort_perror("socketpair");
	}
#endif

	called = was_et = 0;

	tt_int_op(send(pair[0], test, (int)strlen(test)+1, 0), >, 0);
	shutdown(pair[0], SHUT_WR);

	
	base = event_base_new();

	if (!strcmp(event_base_get_method(base), "epoll") ||
	    !strcmp(event_base_get_method(base), "epoll (with changelist)") ||
	    !strcmp(event_base_get_method(base), "kqueue"))
		supports_et = 1;
	else
		supports_et = 0;

	TT_BLATHER(("Checking for edge-triggered events with %s, which should %s"
				"support edge-triggering", event_base_get_method(base),
				supports_et?"":"not "));

	
	ev = event_new(base, pair[1], EV_READ|EV_ET|EV_PERSIST, read_cb, &ev);

	event_add(ev, NULL);

	






	event_base_loop(base,EVLOOP_NONBLOCK|EVLOOP_ONCE);
	event_base_loop(base,EVLOOP_NONBLOCK|EVLOOP_ONCE);

	if (supports_et) {
		tt_int_op(called, ==, 1);
		tt_assert(was_et);
	} else {
		tt_int_op(called, ==, 2);
		tt_assert(!was_et);
	}

 end:
	if (ev) {
		event_del(ev);
		event_free(ev);
	}
	if (base)
		event_base_free(base);
	evutil_closesocket(pair[0]);
	evutil_closesocket(pair[1]);
}

static void
test_edgetriggered_mix_error(void *data_)
{
	struct basic_test_data *data = data_;
	struct event_base *base = NULL;
	struct event *ev_et=NULL, *ev_lt=NULL;

#ifdef _EVENT_DISABLE_DEBUG_MODE
	if (1)
		tt_skip();
#endif

	event_enable_debug_mode();

	base = event_base_new();

	
	ev_et = event_new(base, data->pair[0], EV_READ|EV_ET, read_cb, ev_et);
	tt_assert(ev_et);
	ev_lt = event_new(base, data->pair[0], EV_READ, read_cb, ev_lt);
	tt_assert(ev_lt);

	
	tt_int_op(0, ==, event_add(ev_et, NULL));
	tt_int_op(-1, ==, event_add(ev_lt, NULL));
	tt_int_op(EV_READ, ==, event_pending(ev_et, EV_READ, NULL));
	tt_int_op(0, ==, event_pending(ev_lt, EV_READ, NULL));

	tt_int_op(0, ==, event_del(ev_et));
	
	tt_int_op(0, ==, event_add(ev_lt, NULL));
	tt_int_op(-1, ==, event_add(ev_et, NULL));
	tt_int_op(EV_READ, ==, event_pending(ev_lt, EV_READ, NULL));
	tt_int_op(0, ==, event_pending(ev_et, EV_READ, NULL));

end:
	if (ev_et)
		event_free(ev_et);
	if (ev_lt)
		event_free(ev_lt);
	if (base)
		event_base_free(base);
}

struct testcase_t edgetriggered_testcases[] = {
	{ "et", test_edgetriggered, TT_FORK, NULL, NULL },
	{ "et_mix_error", test_edgetriggered_mix_error,
	  TT_FORK|TT_NEED_SOCKETPAIR|TT_NO_LOGS, &basic_setup, NULL },
	END_OF_TESTCASES
};
