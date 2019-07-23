











































#if !defined(PRPDCE_H)
#define PRPDCE_H

#include "prlock.h"
#include "prcvar.h"
#include "prtypes.h"
#include "prinrval.h"

PR_BEGIN_EXTERN_C

#define _PR_NAKED_CV_LOCK (PRLock*)0xdce1dce1









NSPR_API(PRStatus) PRP_TryLock(PRLock *lock);









NSPR_API(PRCondVar*) PRP_NewNakedCondVar(void);






NSPR_API(void) PRP_DestroyNakedCondVar(PRCondVar *cvar);












NSPR_API(PRStatus) PRP_NakedWait(
	PRCondVar *cvar, PRLock *lock, PRIntervalTime timeout);








NSPR_API(PRStatus) PRP_NakedNotify(PRCondVar *cvar);








NSPR_API(PRStatus) PRP_NakedBroadcast(PRCondVar *cvar);

PR_END_EXTERN_C

#endif 
