





























#ifndef _USER_ATOMIC_H_
#define _USER_ATOMIC_H_









#include <stdio.h>
#include <sys/types.h>

#if defined(__Userspace_os_Darwin) || defined (__Userspace_os_Windows)
#if defined (__Userspace_os_Windows)
#define atomic_add_int(addr, val) InterlockedExchangeAdd((LPLONG)addr, (LONG)val)
#define atomic_fetchadd_int(addr, val) InterlockedExchangeAdd((LPLONG)addr, (LONG)val)
#define atomic_subtract_int(addr, val)   InterlockedExchangeAdd((LPLONG)addr,-((LONG)val))
#define atomic_cmpset_int(dst, exp, src) InterlockedCompareExchange((LPLONG)dst, src, exp)
#define SCTP_DECREMENT_AND_CHECK_REFCOUNT(addr) (InterlockedExchangeAdd((LPLONG)addr, (-1L)) == 1)
#else
#include <libkern/OSAtomic.h>
#define atomic_add_int(addr, val) OSAtomicAdd32Barrier(val, (int32_t *)addr)
#define atomic_fetchadd_int(addr, val) OSAtomicAdd32Barrier(val, (int32_t *)addr)
#define atomic_subtract_int(addr, val) OSAtomicAdd32Barrier(-val, (int32_t *)addr)
#define atomic_cmpset_int(dst, exp, src) OSAtomicCompareAndSwapIntBarrier(exp, src, (int *)dst)
#define SCTP_DECREMENT_AND_CHECK_REFCOUNT(addr) (atomic_fetchadd_int(addr, -1) == 0)
#endif

#if defined(INVARIANTS)
#define SCTP_SAVE_ATOMIC_DECREMENT(addr, val) \
{ \
	int32_t newval; \
	newval = atomic_fetchadd_int(addr, -val); \
	if (newval < 0) { \
		panic("Counter goes negative"); \
	} \
}
#else
#define SCTP_SAVE_ATOMIC_DECREMENT(addr, val) \
{ \
	int32_t newval; \
	newval = atomic_fetchadd_int(addr, -val); \
	if (newval < 0) { \
		*addr = 0; \
	} \
}
#if defined(__Userspace_os_Windows)
static void atomic_init() {} 
#else
static inline void atomic_init() {} 
#endif
#endif

#else







#define atomic_add_int(P, V)	 (void) __sync_fetch_and_add(P, V)


#define atomic_subtract_int(P, V) (void) __sync_fetch_and_sub(P, V)





#define atomic_fetchadd_int(p, v) __sync_fetch_and_add(p, v)









#define atomic_cmpset_int(dst, exp, src) __sync_bool_compare_and_swap(dst, exp, src)

#define SCTP_DECREMENT_AND_CHECK_REFCOUNT(addr) (atomic_fetchadd_int(addr, -1) == 1)
#if defined(INVARIANTS)
#define SCTP_SAVE_ATOMIC_DECREMENT(addr, val) \
{ \
	int32_t oldval; \
	oldval = atomic_fetchadd_int(addr, -val); \
	if (oldval < val) { \
		panic("Counter goes negative"); \
	} \
}
#else
#define SCTP_SAVE_ATOMIC_DECREMENT(addr, val) \
{ \
	int32_t oldval; \
	oldval = atomic_fetchadd_int(addr, -val); \
	if (oldval < val) { \
		*addr = 0; \
	} \
}
#endif
static inline void atomic_init() {} 
#endif

#if 0 
#include "user_include/atomic_ops.h"


#define atomic_add_int(P, V)	 AO_fetch_and_add((AO_t*)P, V)

#define atomic_subtract_int(P, V) AO_fetch_and_add((AO_t*)P, -(V))





#define atomic_fetchadd_int(p, v) AO_fetch_and_add((AO_t*)p, v)














#define atomic_cmpset_int(dst, exp, src) AO_compare_and_swap((AO_t*)dst, exp, src)

static inline void atomic_init() {} 
#endif 

#if 0 

#include <pthread.h>

extern userland_mutex_t atomic_mtx;

#if defined (__Userspace_os_Windows)
static inline void atomic_init() {
	InitializeCriticalSection(&atomic_mtx);
}
static inline void atomic_destroy() {
	DeleteCriticalSection(&atomic_mtx);
}
static inline void atomic_lock() {
	EnterCriticalSection(&atomic_mtx);
}
static inline void atomic_unlock() {
	LeaveCriticalSection(&atomic_mtx);
}
#else
static inline void atomic_init() {
	(void)pthread_mutex_init(&atomic_mtx, NULL);
}
static inline void atomic_destroy() {
	(void)pthread_mutex_destroy(&atomic_mtx);
}
static inline void atomic_lock() {
	(void)pthread_mutex_lock(&atomic_mtx);
}
static inline void atomic_unlock() {
	(void)pthread_mutex_unlock(&atomic_mtx);
}
#endif





#define	MPLOCKED	"lock ; "





static __inline u_int
atomic_fetchadd_int(volatile void *n, u_int v)
{
	int *p = (int *) n;
	atomic_lock();
	__asm __volatile(
	"	" MPLOCKED "		"
	"	xaddl	%0, %1 ;	"
	"# atomic_fetchadd_int"
	: "+r" (v),			
	  "=m" (*p)			
	: "m" (*p));			
	atomic_unlock();

	return (v);
}


#ifdef CPU_DISABLE_CMPXCHG

static __inline int
atomic_cmpset_int(volatile u_int *dst, u_int exp, u_int src)
{
	u_char res;

	atomic_lock();
	__asm __volatile(
	"	pushfl ;		"
	"	cli ;			"
	"	cmpl	%3,%4 ;		"
	"	jne	1f ;		"
	"	movl	%2,%1 ;		"
	"1:				"
	"       sete	%0 ;		"
	"	popfl ;			"
	"# atomic_cmpset_int"
	: "=q" (res),			
	  "=m" (*dst)			
	: "r" (src),			
	  "r" (exp),			
	  "m" (*dst)			
	: "memory");
	atomic_unlock();

	return (res);
}

#else 

static __inline int
atomic_cmpset_int(volatile u_int *dst, u_int exp, u_int src)
{
	atomic_lock();
	u_char res;

	__asm __volatile(
	"	" MPLOCKED "		"
	"	cmpxchgl %2,%1 ;	"
	"	sete	%0 ;		"
	"1:				"
	"# atomic_cmpset_int"
	: "=a" (res),			
	  "=m" (*dst)			
	: "r" (src),			
	  "a" (exp),			
	  "m" (*dst)			
	: "memory");
	atomic_unlock();

	return (res);
}

#endif 

#define atomic_add_int(P, V)	 do {   \
		atomic_lock();          \
		(*(u_int *)(P) += (V)); \
		atomic_unlock();        \
} while(0)
#define atomic_subtract_int(P, V)  do {   \
		atomic_lock();            \
		(*(u_int *)(P) -= (V));   \
		atomic_unlock();          \
} while(0)

#endif
#endif
