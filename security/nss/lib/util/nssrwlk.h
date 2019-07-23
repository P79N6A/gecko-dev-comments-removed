



















































#ifndef nssrwlk_h___
#define nssrwlk_h___

#include "utilrename.h"
#include "prtypes.h"
#include "nssrwlkt.h"

#define	NSS_RWLOCK_RANK_NONE	0


PR_BEGIN_EXTERN_C













PR_EXTERN(NSSRWLock*) NSSRWLock_New(PRUint32 lock_rank, const char *lock_name);

















PR_EXTERN(NSSRWLock *)
nssRWLock_AtomicCreate( NSSRWLock  ** prwlock, 
			PRUint32      lock_rank, 
			const char *  lock_name);









PR_EXTERN(void) NSSRWLock_Destroy(NSSRWLock *lock);









PR_EXTERN(void) NSSRWLock_LockRead(NSSRWLock *lock);









PR_EXTERN(void) NSSRWLock_LockWrite(NSSRWLock *lock);









PR_EXTERN(void) NSSRWLock_UnlockRead(NSSRWLock *lock);









PR_EXTERN(void) NSSRWLock_UnlockWrite(NSSRWLock *lock);










PR_EXTERN(PRBool) NSSRWLock_HaveWriteLock(NSSRWLock *rwlock);


PR_END_EXTERN_C

#endif 
