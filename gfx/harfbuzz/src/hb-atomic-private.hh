






























#ifndef HB_ATOMIC_PRIVATE_HH
#define HB_ATOMIC_PRIVATE_HH

#include "hb-private.hh"






#if 0


#elif !defined(HB_NO_MT) && defined(_MSC_VER) && _MSC_VER >= 1600

#include <intrin.h>
typedef long hb_atomic_int_t;
#define hb_atomic_int_add(AI, V)	_InterlockedExchangeAdd (&(AI), (V))
#define hb_atomic_int_set(AI, V)	((AI) = (V), MemoryBarrier ())
#define hb_atomic_int_get(AI)		(MemoryBarrier (), (AI))


#elif !defined(HB_NO_MT) && defined(__APPLE__)

#include <libkern/OSAtomic.h>
typedef int32_t hb_atomic_int_t;
#define hb_atomic_int_add(AI, V)	(OSAtomicAdd32Barrier ((V), &(AI)) - (V))
#define hb_atomic_int_set(AI, V)	((AI) = (V), OSMemoryBarrier ())
#define hb_atomic_int_get(AI)		(OSMemoryBarrier (), (AI))


#elif !defined(HB_NO_MT) && defined(HAVE_GLIB)

#include <glib.h>
typedef volatile int hb_atomic_int_t;
#if GLIB_CHECK_VERSION(2,29,5)
#define hb_atomic_int_add(AI, V)	g_atomic_int_add (&(AI), (V))
#else
#define hb_atomic_int_add(AI, V)	g_atomic_int_exchange_and_add (&(AI), (V))
#endif
#define hb_atomic_int_set(AI, V)	g_atomic_int_set (&(AI), (V))
#define hb_atomic_int_get(AI)		g_atomic_int_get (&(AI))


#else

#define HB_ATOMIC_INT_NIL 1
typedef volatile int hb_atomic_int_t;
#define hb_atomic_int_add(AI, V)	(((AI) += (V)) - (V))
#define hb_atomic_int_set(AI, V)	((void) ((AI) = (V)))
#define hb_atomic_int_get(AI)		(AI)

#endif


#endif 
