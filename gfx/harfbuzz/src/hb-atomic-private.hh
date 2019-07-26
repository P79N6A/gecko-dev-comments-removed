






























#ifndef HB_ATOMIC_PRIVATE_HH
#define HB_ATOMIC_PRIVATE_HH

#include "hb-private.hh"






#if 0


#elif !defined(HB_NO_MT) && defined(_MSC_VER) || defined(__MINGW32__)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>




#ifdef MemoryBarrier
#define HBMemoryBarrier MemoryBarrier
#else
static inline void HBMemoryBarrier (void) {
  long dummy = 0;
  InterlockedExchange (&dummy, 1);
}
#endif

typedef LONG hb_atomic_int_t;
#define hb_atomic_int_add(AI, V)	InterlockedExchangeAdd (&(AI), (V))

#define hb_atomic_ptr_get(P)		(HBMemoryBarrier (), (void *) *(P))
#define hb_atomic_ptr_cmpexch(P,O,N)	(InterlockedCompareExchangePointer ((void **) (P), (void *) (N), (void *) (O)) == (void *) (O))


#elif !defined(HB_NO_MT) && defined(__APPLE__)

#include <libkern/OSAtomic.h>
#ifdef __MAC_OS_X_MIN_REQUIRED
#include <AvailabilityMacros.h>
#elif defined(__IPHONE_OS_MIN_REQUIRED)
#include <Availability.h>
#endif

typedef int32_t hb_atomic_int_t;
#define hb_atomic_int_add(AI, V)	(OSAtomicAdd32Barrier ((V), &(AI)) - (V))

#define hb_atomic_ptr_get(P)		(OSMemoryBarrier (), (void *) *(P))
#if (MAC_OS_X_VERSION_MIN_REQUIRED > MAC_OS_X_VERSION_10_4 || __IPHONE_VERSION_MIN_REQUIRED >= 20100)
#define hb_atomic_ptr_cmpexch(P,O,N)	OSAtomicCompareAndSwapPtrBarrier ((void *) (O), (void *) (N), (void **) (P))
#else
#if __ppc64__ || __x86_64__
#define hb_atomic_ptr_cmpexch(P,O,N)    OSAtomicCompareAndSwap64Barrier ((int64_t) (O), (int64_t) (N), (int64_t*) (P))
#else
#define hb_atomic_ptr_cmpexch(P,O,N)    OSAtomicCompareAndSwap32Barrier ((int32_t) (O), (int32_t) (N), (int32_t*) (P))
#endif
#endif


#elif !defined(HB_NO_MT) && defined(HAVE_INTEL_ATOMIC_PRIMITIVES)

typedef int hb_atomic_int_t;
#define hb_atomic_int_add(AI, V)	__sync_fetch_and_add (&(AI), (V))

#define hb_atomic_ptr_get(P)		(void *) (__sync_synchronize (), *(P))
#define hb_atomic_ptr_cmpexch(P,O,N)	__sync_bool_compare_and_swap ((P), (O), (N))


#elif !defined(HB_NO_MT)

#define HB_ATOMIC_INT_NIL 1 /* Warn that fallback implementation is in use. */
typedef volatile int hb_atomic_int_t;
#define hb_atomic_int_add(AI, V)	(((AI) += (V)) - (V))

#define hb_atomic_ptr_get(P)		((void *) *(P))
#define hb_atomic_ptr_cmpexch(P,O,N)	(* (void * volatile *) (P) == (void *) (O) ? (* (void * volatile *) (P) = (void *) (N), true) : false)


#else 

typedef int hb_atomic_int_t;
#define hb_atomic_int_add(AI, V)	(((AI) += (V)) - (V))

#define hb_atomic_ptr_get(P)		((void *) *(P))
#define hb_atomic_ptr_cmpexch(P,O,N)	(* (void **) (P) == (void *) (O) ? (* (void **) (P) = (void *) (N), true) : false)

#endif



#endif 
