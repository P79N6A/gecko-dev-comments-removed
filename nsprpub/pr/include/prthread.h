




#ifndef prthread_h___
#define prthread_h___






































#include "prtypes.h"
#include "prinrval.h"

PR_BEGIN_EXTERN_C

typedef struct PRThread PRThread;
typedef struct PRThreadStack PRThreadStack;

typedef enum PRThreadType {
    PR_USER_THREAD,
    PR_SYSTEM_THREAD
} PRThreadType;

typedef enum PRThreadScope {
    PR_LOCAL_THREAD,
    PR_GLOBAL_THREAD,
    PR_GLOBAL_BOUND_THREAD
} PRThreadScope;

typedef enum PRThreadState {
    PR_JOINABLE_THREAD,
    PR_UNJOINABLE_THREAD
} PRThreadState;

typedef enum PRThreadPriority
{
    PR_PRIORITY_FIRST = 0,      
    PR_PRIORITY_LOW = 0,        
    PR_PRIORITY_NORMAL = 1,     
    PR_PRIORITY_HIGH = 2,       
    PR_PRIORITY_URGENT = 3,     
    PR_PRIORITY_LAST = 3        
} PRThreadPriority;


























NSPR_API(PRThread*) PR_CreateThread(PRThreadType type,
                     void (PR_CALLBACK *start)(void *arg),
                     void *arg,
                     PRThreadPriority priority,
                     PRThreadScope scope,
                     PRThreadState state,
                     PRUint32 stackSize);














NSPR_API(PRStatus) PR_JoinThread(PRThread *thread);





NSPR_API(PRThread*) PR_GetCurrentThread(void);
#ifndef NO_NSPR_10_SUPPORT
#define PR_CurrentThread() PR_GetCurrentThread() /* for nspr1.0 compat. */
#endif 




NSPR_API(PRThreadPriority) PR_GetThreadPriority(const PRThread *thread);











NSPR_API(void) PR_SetThreadPriority(PRThread *thread, PRThreadPriority priority);





NSPR_API(PRStatus) PR_SetCurrentThreadName(const char *name);




NSPR_API(const char *) PR_GetThreadName(const PRThread *thread);

























typedef void (PR_CALLBACK *PRThreadPrivateDTOR)(void *priv);

NSPR_API(PRStatus) PR_NewThreadPrivateIndex(
    PRUintn *newIndex, PRThreadPrivateDTOR destructor);












NSPR_API(PRStatus) PR_SetThreadPrivate(PRUintn tpdIndex, void *priv);










NSPR_API(void*) PR_GetThreadPrivate(PRUintn tpdIndex);











NSPR_API(PRStatus) PR_Interrupt(PRThread *thread);





NSPR_API(void) PR_ClearInterrupt(void);




NSPR_API(void) PR_BlockInterrupt(void);




NSPR_API(void) PR_UnblockInterrupt(void);








NSPR_API(PRStatus) PR_Sleep(PRIntervalTime ticks);




NSPR_API(PRThreadScope) PR_GetThreadScope(const PRThread *thread);




NSPR_API(PRThreadType) PR_GetThreadType(const PRThread *thread);




NSPR_API(PRThreadState) PR_GetThreadState(const PRThread *thread);

PR_END_EXTERN_C

#endif 
