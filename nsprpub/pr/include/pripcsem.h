






















































#ifndef pripcsem_h___
#define pripcsem_h___

#include "prtypes.h"
#include "prio.h"

PR_BEGIN_EXTERN_C





typedef struct PRSem PRSem;




















#define PR_SEM_CREATE 0x1  /* create if not exist */
#define PR_SEM_EXCL   0x2  /* fail if already exists */

NSPR_API(PRSem *) PR_OpenSemaphore(
    const char *name, PRIntn flags, PRIntn mode, PRUintn value);











NSPR_API(PRStatus) PR_WaitSemaphore(PRSem *sem);







NSPR_API(PRStatus) PR_PostSemaphore(PRSem *sem);







NSPR_API(PRStatus) PR_CloseSemaphore(PRSem *sem);







NSPR_API(PRStatus) PR_DeleteSemaphore(const char *name);

PR_END_EXTERN_C

#endif 
