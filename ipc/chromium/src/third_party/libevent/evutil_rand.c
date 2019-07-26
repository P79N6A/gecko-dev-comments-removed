

































#include "event2/event-config.h"

#include <limits.h>

#include "util-internal.h"
#include "evthread-internal.h"

#ifdef _EVENT_HAVE_ARC4RANDOM
#include <stdlib.h>
#include <string.h>
int
evutil_secure_rng_init(void)
{
	
	(void) arc4random();
	return 0;
}
int
evutil_secure_rng_global_setup_locks_(const int enable_locks)
{
	return 0;
}

static void
ev_arc4random_buf(void *buf, size_t n)
{
#if defined(_EVENT_HAVE_ARC4RANDOM_BUF) && !defined(__APPLE__)
	return arc4random_buf(buf, n);
#else
	unsigned char *b = buf;

	

	if (n >= 4 && ((ev_uintptr_t)b) & 3) {
		ev_uint32_t u = arc4random();
		int n_bytes = 4 - (((ev_uintptr_t)b) & 3);
		memcpy(b, &u, n_bytes);
		b += n_bytes;
		n -= n_bytes;
	}
	while (n >= 4) {
		*(ev_uint32_t*)b = arc4random();
		b += 4;
		n -= 4;
	}
	if (n) {
		ev_uint32_t u = arc4random();
		memcpy(b, &u, n);
	}
#endif
}

#else 

#ifdef _EVENT_ssize_t
#define ssize_t _EVENT_SSIZE_t
#endif
#define ARC4RANDOM_EXPORT static
#define _ARC4_LOCK() EVLOCK_LOCK(arc4rand_lock, 0)
#define _ARC4_UNLOCK() EVLOCK_UNLOCK(arc4rand_lock, 0)
#ifndef _EVENT_DISABLE_THREAD_SUPPORT
static void *arc4rand_lock;
#endif

#define ARC4RANDOM_UINT32 ev_uint32_t
#define ARC4RANDOM_NOSTIR
#define ARC4RANDOM_NORANDOM
#define ARC4RANDOM_NOUNIFORM

#include "./arc4random.c"

#ifndef _EVENT_DISABLE_THREAD_SUPPORT
int
evutil_secure_rng_global_setup_locks_(const int enable_locks)
{
	EVTHREAD_SETUP_GLOBAL_LOCK(arc4rand_lock, 0);
	return 0;
}
#endif

int
evutil_secure_rng_init(void)
{
	int val;

	_ARC4_LOCK();
	if (!arc4_seeded_ok)
		arc4_stir();
	val = arc4_seeded_ok ? 0 : -1;
	_ARC4_UNLOCK();
	return val;
}

static void
ev_arc4random_buf(void *buf, size_t n)
{
	arc4random_buf(buf, n);
}

#endif 

void
evutil_secure_rng_get_bytes(void *buf, size_t n)
{
	ev_arc4random_buf(buf, n);
}

void
evutil_secure_rng_add_bytes(const char *buf, size_t n)
{
	arc4random_addrandom((unsigned char*)buf,
	    n>(size_t)INT_MAX ? INT_MAX : (int)n);
}

