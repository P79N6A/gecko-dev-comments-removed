




































#ifndef prinit_h___
#define prinit_h___

#include "prthread.h"
#include "prtypes.h"
#include "prwin16.h"
#include <stdio.h>

PR_BEGIN_EXTERN_C









#define PR_NAME     "NSPR"









#define PR_VERSION  "4.8.1"
#define PR_VMAJOR   4
#define PR_VMINOR   8
#define PR_VPATCH   1
#define PR_BETA     PR_FALSE

















typedef PRBool (*PRVersionCheck)(const char*);









NSPR_API(PRBool) PR_VersionCheck(const char *importedVersion);












NSPR_API(void) PR_Init(
    PRThreadType type, PRThreadPriority priority, PRUintn maxPTDs);



















typedef PRIntn (PR_CALLBACK *PRPrimordialFn)(PRIntn argc, char **argv);

NSPR_API(PRIntn) PR_Initialize(
    PRPrimordialFn prmain, PRIntn argc, char **argv, PRUintn maxPTDs);




NSPR_API(PRBool) PR_Initialized(void);

















NSPR_API(PRStatus) PR_Cleanup(void);





NSPR_API(void) PR_DisableClockInterrupts(void);





NSPR_API(void) PR_EnableClockInterrupts(void);





NSPR_API(void) PR_BlockClockInterrupts(void);





NSPR_API(void) PR_UnblockClockInterrupts(void);




NSPR_API(void) PR_SetConcurrency(PRUintn numCPUs);






NSPR_API(PRStatus) PR_SetFDCacheSize(PRIntn low, PRIntn high);






NSPR_API(void) PR_ProcessExit(PRIntn status);






NSPR_API(void) PR_Abort(void);









typedef struct PRCallOnceType {
    PRIntn initialized;
    PRInt32 inProgress;
    PRStatus status;
} PRCallOnceType;

typedef PRStatus (PR_CALLBACK *PRCallOnceFN)(void);

typedef PRStatus (PR_CALLBACK *PRCallOnceWithArgFN)(void *arg);

NSPR_API(PRStatus) PR_CallOnce(
    PRCallOnceType *once,
    PRCallOnceFN    func
);

NSPR_API(PRStatus) PR_CallOnceWithArg(
    PRCallOnceType      *once,
    PRCallOnceWithArgFN  func,
    void                *arg
);


PR_END_EXTERN_C

#endif 
