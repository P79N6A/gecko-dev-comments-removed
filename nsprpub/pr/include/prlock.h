














































#ifndef prlock_h___
#define prlock_h___

#include "prtypes.h"

PR_BEGIN_EXTERN_C












typedef struct PRLock PRLock;
















NSPR_API(PRLock*) PR_NewLock(void);










NSPR_API(void) PR_DestroyLock(PRLock *lock);










NSPR_API(void) PR_Lock(PRLock *lock);











NSPR_API(PRStatus) PR_Unlock(PRLock *lock);











#if defined(DEBUG) || defined(FORCE_PR_ASSERT)
#define PR_ASSERT_CURRENT_THREAD_OWNS_LOCK( lock) \
    PR_AssertCurrentThreadOwnsLock(lock)
#else
#define PR_ASSERT_CURRENT_THREAD_OWNS_LOCK( lock)
#endif


NSPR_API(void) PR_AssertCurrentThreadOwnsLock(PRLock *lock);

PR_END_EXTERN_C

#endif 
