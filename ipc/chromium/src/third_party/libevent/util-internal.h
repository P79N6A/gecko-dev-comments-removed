
























#ifndef _EVENT_UTIL_INTERNAL_H
#define _EVENT_UTIL_INTERNAL_H

#include "event2/event-config.h"
#include <errno.h>


#include "log-internal.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef _EVENT_HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include "event2/util.h"

#include "ipv6-internal.h"

#ifdef __cplusplus
extern "C" {
#endif


#ifdef _EVENT_inline
#define inline _EVENT_inline
#endif
#ifdef _EVENT___func__
#define __func__ _EVENT___func__
#endif


#define _EVUTIL_NIL_STMT ((void)0)





#define _EVUTIL_NIL_CONDITION(condition) do { \
	(void)sizeof(!(condition));  \
} while(0)









#ifndef WIN32


#define EVUTIL_ERR_RW_RETRIABLE(e)				\
	((e) == EINTR || (e) == EAGAIN)

#define EVUTIL_ERR_CONNECT_RETRIABLE(e)			\
	((e) == EINTR || (e) == EINPROGRESS)

#define EVUTIL_ERR_ACCEPT_RETRIABLE(e)			\
	((e) == EINTR || (e) == EAGAIN || (e) == ECONNABORTED)


#define EVUTIL_ERR_CONNECT_REFUSED(e)					\
	((e) == ECONNREFUSED)

#else

#define EVUTIL_ERR_RW_RETRIABLE(e)					\
	((e) == WSAEWOULDBLOCK ||					\
	    (e) == WSAEINTR)

#define EVUTIL_ERR_CONNECT_RETRIABLE(e)					\
	((e) == WSAEWOULDBLOCK ||					\
	    (e) == WSAEINTR ||						\
	    (e) == WSAEINPROGRESS ||					\
	    (e) == WSAEINVAL)

#define EVUTIL_ERR_ACCEPT_RETRIABLE(e)			\
	EVUTIL_ERR_RW_RETRIABLE(e)

#define EVUTIL_ERR_CONNECT_REFUSED(e)					\
	((e) == WSAECONNREFUSED)

#endif

#ifdef _EVENT_socklen_t
#define socklen_t _EVENT_socklen_t
#endif


#ifdef SHUT_RD
#define EVUTIL_SHUT_RD SHUT_RD
#else
#define EVUTIL_SHUT_RD 0
#endif
#ifdef SHUT_WR
#define EVUTIL_SHUT_WR SHUT_WR
#else
#define EVUTIL_SHUT_WR 1
#endif
#ifdef SHUT_BOTH
#define EVUTIL_SHUT_BOTH SHUT_BOTH
#else
#define EVUTIL_SHUT_BOTH 2
#endif





int EVUTIL_ISALPHA(char c);
int EVUTIL_ISALNUM(char c);
int EVUTIL_ISSPACE(char c);
int EVUTIL_ISDIGIT(char c);
int EVUTIL_ISXDIGIT(char c);
int EVUTIL_ISPRINT(char c);
int EVUTIL_ISLOWER(char c);
int EVUTIL_ISUPPER(char c);
char EVUTIL_TOUPPER(char c);
char EVUTIL_TOLOWER(char c);
















#define EVUTIL_UPCAST(ptr, type, field)				\
	((type *)(((char*)(ptr)) - evutil_offsetof(type, field)))




int evutil_open_closeonexec(const char *pathname, int flags, unsigned mode);

int evutil_read_file(const char *filename, char **content_out, size_t *len_out,
    int is_binary);

int evutil_socket_connect(evutil_socket_t *fd_ptr, struct sockaddr *sa, int socklen);

int evutil_socket_finished_connecting(evutil_socket_t fd);

int evutil_ersatz_socketpair(int, int , int, evutil_socket_t[]);

int evutil_resolve(int family, const char *hostname, struct sockaddr *sa,
    ev_socklen_t *socklen, int port);

const char *evutil_getenv(const char *name);

long _evutil_weakrand(void);



#if defined(__GNUC__) && __GNUC__ >= 3         
#define EVUTIL_UNLIKELY(p) __builtin_expect(!!(p),0)
#else
#define EVUTIL_UNLIKELY(p) (p)
#endif


#ifdef NDEBUG
#define EVUTIL_ASSERT(cond) _EVUTIL_NIL_CONDITION(cond)
#define EVUTIL_FAILURE_CHECK(cond) 0
#else
#define EVUTIL_ASSERT(cond)						\
	do {								\
		if (EVUTIL_UNLIKELY(!(cond))) {				\
			event_errx(_EVENT_ERR_ABORT,			\
			    "%s:%d: Assertion %s failed in %s",		\
			    __FILE__,__LINE__,#cond,__func__);		\
			/* In case a user-supplied handler tries to */	\
			/* return control to us, log and abort here. */	\
			(void)fprintf(stderr,				\
			    "%s:%d: Assertion %s failed in %s",		\
			    __FILE__,__LINE__,#cond,__func__);		\
			abort();					\
		}							\
	} while (0)
#define EVUTIL_FAILURE_CHECK(cond) EVUTIL_UNLIKELY(cond)
#endif

#ifndef _EVENT_HAVE_STRUCT_SOCKADDR_STORAGE



struct sockaddr_storage {
	union {
		struct sockaddr ss_sa;
		struct sockaddr_in ss_sin;
		struct sockaddr_in6 ss_sin6;
		char ss_padding[128];
	} ss_union;
};
#define ss_family ss_union.ss_sa.sa_family
#endif




#define EVUTIL_EAI_NEED_RESOLVE      -90002

struct evdns_base;
struct evdns_getaddrinfo_request;
typedef struct evdns_getaddrinfo_request* (*evdns_getaddrinfo_fn)(
    struct evdns_base *base,
    const char *nodename, const char *servname,
    const struct evutil_addrinfo *hints_in,
    void (*cb)(int, struct evutil_addrinfo *, void *), void *arg);

void evutil_set_evdns_getaddrinfo_fn(evdns_getaddrinfo_fn fn);

struct evutil_addrinfo *evutil_new_addrinfo(struct sockaddr *sa,
    ev_socklen_t socklen, const struct evutil_addrinfo *hints);
struct evutil_addrinfo *evutil_addrinfo_append(struct evutil_addrinfo *first,
    struct evutil_addrinfo *append);
void evutil_adjust_hints_for_addrconfig(struct evutil_addrinfo *hints);
int evutil_getaddrinfo_common(const char *nodename, const char *servname,
    struct evutil_addrinfo *hints, struct evutil_addrinfo **res, int *portnum);

int evutil_getaddrinfo_async(struct evdns_base *dns_base,
    const char *nodename, const char *servname,
    const struct evutil_addrinfo *hints_in,
    void (*cb)(int, struct evutil_addrinfo *, void *), void *arg);



int evutil_sockaddr_is_loopback(const struct sockaddr *sa);







const char *evutil_format_sockaddr_port(const struct sockaddr *sa, char *out, size_t outlen);

long evutil_tv_to_msec(const struct timeval *tv);

int evutil_hex_char_to_int(char c);

#ifdef WIN32
HANDLE evutil_load_windows_system_library(const TCHAR *library_name);
#endif

#ifndef EV_SIZE_FMT
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
#define EV_U64_FMT "%I64u"
#define EV_I64_FMT "%I64d"
#define EV_I64_ARG(x) ((__int64)(x))
#define EV_U64_ARG(x) ((unsigned __int64)(x))
#else
#define EV_U64_FMT "%llu"
#define EV_I64_FMT "%lld"
#define EV_I64_ARG(x) ((long long)(x))
#define EV_U64_ARG(x) ((unsigned long long)(x))
#endif
#endif

#ifdef _WIN32
#define EV_SOCK_FMT EV_I64_FMT
#define EV_SOCK_ARG(x) EV_I64_ARG((x))
#else
#define EV_SOCK_FMT "%d"
#define EV_SOCK_ARG(x) (x)
#endif

#if defined(__STDC__) && defined(__STDC_VERSION__)
#if (__STDC_VERSION__ >= 199901L)
#define EV_SIZE_FMT "%zu"
#define EV_SSIZE_FMT "%zd"
#define EV_SIZE_ARG(x) (x)
#define EV_SSIZE_ARG(x) (x)
#endif
#endif

#ifndef EV_SIZE_FMT
#if (_EVENT_SIZEOF_SIZE_T <= _EVENT_SIZEOF_LONG)
#define EV_SIZE_FMT "%lu"
#define EV_SSIZE_FMT "%ld"
#define EV_SIZE_ARG(x) ((unsigned long)(x))
#define EV_SSIZE_ARG(x) ((long)(x))
#else
#define EV_SIZE_FMT EV_U64_FMT
#define EV_SSIZE_FMT EV_I64_FMT
#define EV_SIZE_ARG(x) EV_U64_ARG(x)
#define EV_SSIZE_ARG(x) EV_I64_ARG(x)
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif
