




































#ifndef prcvar_h___
#define prcvar_h___

#include "prlock.h"
#include "prinrval.h"

PR_BEGIN_EXTERN_C

typedef struct PRCondVar PRCondVar;












NSPR_API(PRCondVar*) PR_NewCondVar(PRLock *lock);







NSPR_API(void) PR_DestroyCondVar(PRCondVar *cvar);





























NSPR_API(PRStatus) PR_WaitCondVar(PRCondVar *cvar, PRIntervalTime timeout);














NSPR_API(PRStatus) PR_NotifyCondVar(PRCondVar *cvar);









NSPR_API(PRStatus) PR_NotifyAllCondVar(PRCondVar *cvar);

PR_END_EXTERN_C

#endif 
