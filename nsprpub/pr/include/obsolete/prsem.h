




































#ifndef prsem_h___
#define prsem_h___






#include "prtypes.h"

PR_BEGIN_EXTERN_C

typedef struct PRSemaphore PRSemaphore;




NSPR_API(PRSemaphore*) PR_NewSem(PRUintn value);





NSPR_API(void) PR_DestroySem(PRSemaphore *sem);















NSPR_API(PRStatus) PR_WaitSem(PRSemaphore *sem);






NSPR_API(void) PR_PostSem(PRSemaphore *sem);







NSPR_API(PRUintn) PR_GetValueSem(PRSemaphore *sem);

PR_END_EXTERN_C

#endif 
