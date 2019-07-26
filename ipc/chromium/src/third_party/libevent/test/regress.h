

























#ifndef _REGRESS_H_
#define _REGRESS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "tinytest.h"
#include "tinytest_macros.h"

extern struct testcase_t main_testcases[];
extern struct testcase_t evtag_testcases[];
extern struct testcase_t evbuffer_testcases[];
extern struct testcase_t bufferevent_testcases[];
extern struct testcase_t bufferevent_iocp_testcases[];
extern struct testcase_t util_testcases[];
extern struct testcase_t signal_testcases[];
extern struct testcase_t http_testcases[];
extern struct testcase_t dns_testcases[];
extern struct testcase_t rpc_testcases[];
extern struct testcase_t edgetriggered_testcases[];
extern struct testcase_t minheap_testcases[];
extern struct testcase_t iocp_testcases[];
extern struct testcase_t ssl_testcases[];
extern struct testcase_t listener_testcases[];
extern struct testcase_t listener_iocp_testcases[];
extern struct testcase_t thread_testcases[];

void regress_threads(void *);
void test_bufferevent_zlib(void *);


extern evutil_socket_t pair[2];
extern int test_ok;
extern int called;
extern struct event_base *global_base;
extern int in_legacy_test_wrapper;

int regress_make_tmpfile(const void *data, size_t datalen);

struct basic_test_data {
	struct event_base *base;
	evutil_socket_t pair[2];

	void (*legacy_test_fn)(void);

	void *setup_data;
};
extern const struct testcase_setup_t basic_setup;


extern const struct testcase_setup_t legacy_setup;
void run_legacy_test_fn(void *ptr);


#define TT_NEED_SOCKETPAIR	TT_FIRST_USER_FLAG
#define TT_NEED_BASE		(TT_FIRST_USER_FLAG<<1)
#define TT_NEED_DNS		(TT_FIRST_USER_FLAG<<2)
#define TT_LEGACY		(TT_FIRST_USER_FLAG<<3)
#define TT_NEED_THREADS		(TT_FIRST_USER_FLAG<<4)
#define TT_NO_LOGS		(TT_FIRST_USER_FLAG<<5)
#define TT_ENABLE_IOCP_FLAG	(TT_FIRST_USER_FLAG<<6)
#define TT_ENABLE_IOCP		(TT_ENABLE_IOCP_FLAG|TT_NEED_THREADS)


#define TT_ISOLATED TT_FORK|TT_NEED_SOCKETPAIR|TT_NEED_BASE


#define BASIC(name,flags)						\
	{ #name, test_## name, flags, &basic_setup, NULL }

#define LEGACY(name,flags)						\
	{ #name, run_legacy_test_fn, flags|TT_LEGACY, &legacy_setup,	\
	  test_## name }

struct evutil_addrinfo;
struct evutil_addrinfo *ai_find_by_family(struct evutil_addrinfo *ai, int f);
struct evutil_addrinfo *ai_find_by_protocol(struct evutil_addrinfo *ai, int p);
int _test_ai_eq(const struct evutil_addrinfo *ai, const char *sockaddr_port,
    int socktype, int protocol, int line);

#define test_ai_eq(ai, str, s, p) do {					\
		if (_test_ai_eq((ai), (str), (s), (p), __LINE__)<0)	\
			goto end;					\
	} while (0)

#define test_timeval_diff_leq(tv1, tv2, diff, tolerance)		\
	tt_int_op(abs(timeval_msec_diff((tv1), (tv2)) - diff), <=, tolerance)

#define test_timeval_diff_eq(tv1, tv2, diff)				\
	test_timeval_diff_leq((tv1), (tv2), (diff), 50)

long timeval_msec_diff(const struct timeval *start, const struct timeval *end);

#ifndef _WIN32
pid_t regress_fork(void);
#endif

#ifdef __cplusplus
}
#endif

#endif
