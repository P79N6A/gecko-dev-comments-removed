



#include <afx.h>
#include <afxwin.h>
#include <afxmt.h>
#include "cpr_types.h"
#include "cpr_threads.h"
#include "cpr_stdio.h"
#include "cpr_stdlib.h"
#include "cpr_debug.h"
#include "cpr_memory.h"











cprRC_t
cprSuspendThread(cprThread_t thread)
{
    int32_t returnCode;
    static const char fname[] = "cprSuspendThread";
    cpr_thread_t *cprThreadPtr;
	
    cprThreadPtr = (cpr_thread_t*)thread;
    if (cprThreadPtr != NULL) {
		
		CWinThread *pCWinThread;
		pCWinThread = (CWinThread *)cprThreadPtr->u.handlePtr;
		if (pCWinThread != NULL) {
			returnCode = pCWinThread->SuspendThread();
			if (returnCode == -1) {
				CPR_ERROR("%s - Suspend thread failed: %d\n",
					fname,GetLastError());
				return(CPR_FAILURE);
			}
			return(CPR_SUCCESS);
			
			
		}
	}
	CPR_ERROR("%s - NULL pointer passed in.\n", fname);
	return(CPR_FAILURE);
};











cprRC_t
cprResumeThread(cprThread_t thread)
{
    int32_t returnCode;
    static const char fname[] = "cprResumeThread";
    cpr_thread_t *cprThreadPtr;
	
    cprThreadPtr = (cpr_thread_t*)thread;
    if (cprThreadPtr != NULL) {
		CWinThread *pCWinThread;
		pCWinThread = (CWinThread *)cprThreadPtr->u.handlePtr;
		if (pCWinThread != NULL) {
			
			returnCode = pCWinThread->ResumeThread();
			if (returnCode == -1) {
				CPR_ERROR("%s - Resume thread failed: %d\n",
					fname, GetLastError());
				return(CPR_FAILURE);
			}
			return(CPR_SUCCESS);
		}
		
    }
	CPR_ERROR("%s - NULL pointer passed in.\n", fname);
    return(CPR_FAILURE);
};

















cprThread_t
cprCreateThread(const char* name,
                cprThreadStartRoutine startRoutine,
                uint16_t stackSize,
                uint16_t priority,
                void* data)
{
    cpr_thread_t* threadPtr;
    static char fname[] = "cprCreateThread";
	unsigned long result;
	CEvent serialize_lock;

    
    threadPtr = (cpr_thread_t *)cpr_malloc(sizeof(cpr_thread_t));
    if (threadPtr != NULL) {
		
        
        if (name != NULL) {
            threadPtr->name = name;
        }

		threadPtr->u.handlePtr = AfxBeginThread((AFX_THREADPROC)startRoutine, data, priority, stackSize);
		  
        if (threadPtr->u.handlePtr != NULL) {
			PostThreadMessage(((CWinThread *)(threadPtr->u.handlePtr))->m_nThreadID, MSG_ECHO_EVENT, (unsigned long)&serialize_lock, 0);
			result = WaitForSingleObject(serialize_lock, 1000);
			serialize_lock.ResetEvent();
		}
		else
		{
			CPR_ERROR("%s - Thread creation failure: %d\n", fname, GetLastError());
			cpr_free(threadPtr);
            threadPtr = NULL;
			
        }
    } else {
        
        CPR_ERROR("%s - Malloc for new thread failed.\n", fname);
    }
	return(threadPtr);
};














cprRC_t
cprDestroyThread(cprThread_t thread)
{
	cprRC_t retCode = CPR_FAILURE;
    static const char fname[] = "cprDestroyThread";
    cpr_thread_t *cprThreadPtr;

    cprThreadPtr = (cpr_thread_t*)thread;
    if (cprThreadPtr != NULL) {
		CWinThread * pCWinThread;
		uint32_t result = 0;
		uint32_t waitrc = WAIT_FAILED;
		pCWinThread = (CWinThread *)((cpr_thread_t *)thread)->u.handlePtr;
		if (pCWinThread !=NULL) {
			result = pCWinThread->PostThreadMessage(WM_CLOSE, 0, 0);
			if(result) {
				waitrc = WaitForSingleObject(pCWinThread->m_hThread, 60000);
			}
		}
		if (result == 0) {
			CPR_ERROR("%s - Thread exit failure %d\n", fname, GetLastError());
            retCode = CPR_FAILURE;
		}
        retCode = CPR_SUCCESS;
    
    } else {
        CPR_ERROR("%s - NULL pointer passed in.\n", fname);
        retCode = CPR_FAILURE;
    }
	cpr_free(cprThreadPtr);
	return (retCode);
};





















cprRC_t
cprAdjustRelativeThreadPriority (int relPri)
{
    
    return (CPR_FAILURE);
}
