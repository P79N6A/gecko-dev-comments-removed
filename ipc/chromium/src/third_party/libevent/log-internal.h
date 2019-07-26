

























#ifndef _LOG_H_
#define _LOG_H_

#include "event2/util.h"

#ifdef __GNUC__
#define EV_CHECK_FMT(a,b) __attribute__((format(printf, a, b)))
#define EV_NORETURN __attribute__((noreturn))
#else
#define EV_CHECK_FMT(a,b)
#define EV_NORETURN
#endif

#define _EVENT_ERR_ABORT ((int)0xdeaddead)

void event_err(int eval, const char *fmt, ...) EV_CHECK_FMT(2,3) EV_NORETURN;
void event_warn(const char *fmt, ...) EV_CHECK_FMT(1,2);
void event_sock_err(int eval, evutil_socket_t sock, const char *fmt, ...) EV_CHECK_FMT(3,4) EV_NORETURN;
void event_sock_warn(evutil_socket_t sock, const char *fmt, ...) EV_CHECK_FMT(2,3);
void event_errx(int eval, const char *fmt, ...) EV_CHECK_FMT(2,3) EV_NORETURN;
void event_warnx(const char *fmt, ...) EV_CHECK_FMT(1,2);
void event_msgx(const char *fmt, ...) EV_CHECK_FMT(1,2);
void _event_debugx(const char *fmt, ...) EV_CHECK_FMT(1,2);

#ifdef USE_DEBUG
#define event_debug(x) _event_debugx x
#else
#define event_debug(x) do {;} while (0)
#endif

#undef EV_CHECK_FMT

#endif
