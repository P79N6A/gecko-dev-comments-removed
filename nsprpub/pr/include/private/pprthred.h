




































#ifndef pprthred_h___
#define pprthred_h___





#include "nspr.h"

#if defined(XP_OS2)
#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_WIN
#include <os2.h>
#endif

PR_BEGIN_EXTERN_C


















NSPR_API(PRThread*) PR_AttachThread(PRThreadType type,
                                     PRThreadPriority priority,
				     PRThreadStack *stack);












NSPR_API(void) PR_DetachThread(void);





NSPR_API(PRUint32) PR_GetThreadID(PRThread *thread);






typedef void (*PRThreadDumpProc)(PRFileDesc *fd, PRThread *t, void *arg);
NSPR_API(void) PR_SetThreadDumpProc(
    PRThread* thread, PRThreadDumpProc dump, void *arg);









NSPR_API(PRInt32) PR_GetThreadAffinityMask(PRThread *thread, PRUint32 *mask);






NSPR_API(PRInt32) PR_SetThreadAffinityMask(PRThread *thread, PRUint32 mask );





NSPR_API(PRInt32) PR_SetCPUAffinityMask(PRUint32 mask);




NSPR_API(void) PR_ShowStatus(void);




NSPR_API(void) PR_SetThreadRecycleMode(PRUint32 flag);

















NSPR_API(PRThread*) PR_CreateThreadGCAble(PRThreadType type,
				     void (*start)(void *arg),
				     void *arg,
				     PRThreadPriority priority,
				     PRThreadScope scope,
				     PRThreadState state,
				     PRUint32 stackSize);





NSPR_API(PRThread*) PR_AttachThreadGCAble(PRThreadType type,
					PRThreadPriority priority,
					PRThreadStack *stack);




NSPR_API(void) PR_SetThreadGCAble(void);




NSPR_API(void) PR_ClearThreadGCAble(void);





NSPR_API(void) PR_SuspendAll(void);





NSPR_API(void) PR_ResumeAll(void);





NSPR_API(void *) PR_GetSP(PRThread *thread);














NSPR_API(PRWord *) PR_GetGCRegisters(PRThread *t, int isCurrent, int *np);









NSPR_API(void*) GetExecutionEnvironment(PRThread *thread);
NSPR_API(void) SetExecutionEnvironment(PRThread* thread, void *environment);








typedef PRStatus (PR_CALLBACK *PREnumerator)(PRThread *t, int i, void *arg);
NSPR_API(PRStatus) PR_EnumerateThreads(PREnumerator func, void *arg);






typedef PRStatus 
(PR_CALLBACK *PRScanStackFun)(PRThread* t,
			      void** baseAddr, PRUword count, void* closure);







NSPR_API(PRStatus)
PR_ThreadScanStackPointers(PRThread* t,
                           PRScanStackFun scanFun, void* scanClosure);




NSPR_API(PRStatus)
PR_ScanStackPointers(PRScanStackFun scanFun, void* scanClosure);






NSPR_API(PRUword)
PR_GetStackSpaceLeft(PRThread* t);








NSPR_API(struct _PRCPU *) _PR_GetPrimordialCPU(void);












NSPR_API(PRMonitor*) PR_NewNamedMonitor(const char* name);






NSPR_API(PRBool) PR_TestAndLock(PRLock *lock);






NSPR_API(PRBool) PR_TestAndEnterMonitor(PRMonitor *mon);





NSPR_API(PRIntn) PR_GetMonitorEntryCount(PRMonitor *mon);





NSPR_API(PRMonitor*) PR_CTestAndEnterMonitor(void *address);




#if defined(IRIX)















NSPR_API(void) _PR_Irix_Set_Arena_Params(PRInt32 initusers, PRInt32 initsize);

#endif 

#if defined(XP_OS2)





NSPR_API(void) PR_OS2_SetFloatExcpHandler(EXCEPTIONREGISTRATIONRECORD* e);
NSPR_API(void) PR_OS2_UnsetFloatExcpHandler(EXCEPTIONREGISTRATIONRECORD* e);
#endif 


#define PR_InMonitor(m)		(PR_GetMonitorEntryCount(m) > 0)





#ifdef XP_UNIX
extern void PR_XLock(void);
extern void PR_XUnlock(void);
extern PRBool PR_XIsLocked(void);
#endif 

PR_END_EXTERN_C

#endif 
