



















































#ifndef nssrwlk_h___
#define nssrwlk_h___

#include "utilrename.h"
#include "prtypes.h"
#include "nssrwlkt.h"

#define	NSS_RWLOCK_RANK_NONE	0


PR_BEGIN_EXTERN_C













extern NSSRWLock* NSSRWLock_New(PRUint32 lock_rank, const char *lock_name);

















extern NSSRWLock *
nssRWLock_AtomicCreate( NSSRWLock  ** prwlock, 
			PRUint32      lock_rank, 
			const char *  lock_name);









extern void NSSRWLock_Destroy(NSSRWLock *lock);









extern void NSSRWLock_LockRead(NSSRWLock *lock);









extern void NSSRWLock_LockWrite(NSSRWLock *lock);









extern void NSSRWLock_UnlockRead(NSSRWLock *lock);









extern void NSSRWLock_UnlockWrite(NSSRWLock *lock);










extern PRBool NSSRWLock_HaveWriteLock(NSSRWLock *rwlock);


PR_END_EXTERN_C

#endif 
