




































#ifndef prolock_h___
#define prolock_h___

#include "prtypes.h"

PR_BEGIN_EXTERN_C








































#if defined(DEBUG) || defined(FORCE_NSPR_ORDERED_LOCKS)
typedef void * PROrderedLock;
#else





#include "prlock.h"

typedef PRLock PROrderedLock;
#endif

















#if defined(DEBUG) || defined(FORCE_NSPR_ORDERED_LOCKS)
#define PR_CREATE_ORDERED_LOCK(order,name)\
    PR_CreateOrderedLock((order),(name))
#else
#define PR_CREATE_ORDERED_LOCK(order) PR_NewLock()
#endif

NSPR_API(PROrderedLock *) 
    PR_CreateOrderedLock( 
        PRInt32 order,
        const char *name
);
















#if defined(DEBUG) || defined(FORCE_NSPR_ORDERED_LOCKS)
#define PR_DESTROY_ORDERED_LOCK(lock) PR_DestroyOrderedLock((lock))
#else
#define PR_DESTROY_ORDERED_LOCK(lock) PR_DestroyLock((lock))
#endif

NSPR_API(void) 
    PR_DestroyOrderedLock( 
        PROrderedLock *lock 
);


















#if defined(DEBUG) || defined(FORCE_NSPR_ORDERED_LOCKS)
#define PR_LOCK_ORDERED_LOCK(lock) PR_LockOrderedLock((lock))
#else
#define PR_LOCK_ORDERED_LOCK(lock) PR_Lock((lock))
#endif

NSPR_API(void) 
    PR_LockOrderedLock( 
        PROrderedLock *lock 
);


















#if defined(DEBUG) || defined(FORCE_NSPR_ORDERED_LOCKS)
#define PR_UNLOCK_ORDERED_LOCK(lock) PR_UnlockOrderedLock((lock))
#else
#define PR_UNLOCK_ORDERED_LOCK(lock) PR_Unlock((lock))
#endif

NSPR_API(PRStatus) 
    PR_UnlockOrderedLock( 
        PROrderedLock *lock 
);

PR_END_EXTERN_C

#endif 
