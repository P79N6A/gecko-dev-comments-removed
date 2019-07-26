



#include "cpr_types.h"
#include "cpr_threads.h"
#include "cpr_stdio.h"
#include "cpr_stdlib.h"
#include "cpr_debug.h"
#include "cpr_memory.h"
#include "prtypes.h"
#include "mozilla/Assertions.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <process.h>

void CSFLogRegisterThread(const cprThread_t thread);

typedef struct {
    cprThreadStartRoutine startRoutine;
	void *data;
	HANDLE event;
} startThreadData;

unsigned __stdcall
cprStartThread (void *arg)
{
    startThreadData *startThreadDataPtr = (startThreadData *) arg;

    
    MSG msg;
    PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);

    
    SetEvent( startThreadDataPtr->event );

    
    (*startThreadDataPtr->startRoutine)(startThreadDataPtr->data);

    cpr_free(startThreadDataPtr);

    return CPR_SUCCESS;
}














cprRC_t
cprSuspendThread(cprThread_t thread)
{
    int32_t returnCode;
    static const char fname[] = "cprSuspendThread";
    cpr_thread_t *cprThreadPtr;

    cprThreadPtr = (cpr_thread_t*)thread;
    if (cprThreadPtr != NULL) {

		HANDLE *hThread;
		hThread = (HANDLE *)cprThreadPtr->u.handlePtr;
		if (hThread != NULL) {

			returnCode = SuspendThread( hThread );

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
		HANDLE *hThread;
		hThread = (HANDLE *)cprThreadPtr->u.handlePtr;

		if (hThread != NULL) {

			returnCode = ResumeThread(hThread);
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
	HANDLE serialize_lock;
	startThreadData* startThreadDataPtr = 0;
	unsigned int ThreadId;

	
    threadPtr = (cpr_thread_t *)cpr_malloc(sizeof(cpr_thread_t));

	
	startThreadDataPtr = (startThreadData *) cpr_malloc(sizeof(startThreadData));

    if (threadPtr != NULL && startThreadDataPtr != NULL) {

        
        if (name != NULL) {
            threadPtr->name = name;
        }

        startThreadDataPtr->startRoutine = startRoutine;
        startThreadDataPtr->data = data;

		serialize_lock = CreateEvent (NULL, FALSE, FALSE, NULL);
		if (serialize_lock == NULL)	{
			
			CPR_ERROR("%s - Event creation failure: %d\n", fname, GetLastError());
			cpr_free(threadPtr);
			threadPtr = NULL;
		}
		else
		{

			startThreadDataPtr->event = serialize_lock;

			threadPtr->u.handlePtr = (void*)_beginthreadex(NULL, 0, cprStartThread, (void*)startThreadDataPtr, 0, &ThreadId);

			if (threadPtr->u.handlePtr != NULL) {
				threadPtr->threadId = ThreadId;
				result = WaitForSingleObject(serialize_lock, 1000);
				ResetEvent( serialize_lock );
			}
			else
			{
				CPR_ERROR("%s - Thread creation failure: %d\n", fname, GetLastError());
				cpr_free(threadPtr);
				threadPtr = NULL;
			}
			CloseHandle( serialize_lock );
		}
    } else {
        
        CPR_ERROR("%s - Malloc for new thread failed.\n", fname);
    }

    CSFLogRegisterThread(threadPtr);

    return(threadPtr);
};







void cprJoinThread(cprThread_t thread)
{
    cpr_thread_t *cprThreadPtr;

    cprThreadPtr = (cpr_thread_t *) thread;
    MOZ_ASSERT(cprThreadPtr);
    WaitForSingleObject(cprThreadPtr->u.handlePtr, INFINITE);
}













cprRC_t
cprDestroyThread(cprThread_t thread)
{
    cpr_thread_t *cprThreadPtr;

    cprThreadPtr = (cpr_thread_t*)thread;
    if (cprThreadPtr) {
        


        if (cprThreadPtr->threadId == GetCurrentThreadId()) {
            CPR_INFO("%s: Destroying Thread %d", __FUNCTION__, cprThreadPtr->threadId);
            ExitThread(0);
            return CPR_SUCCESS;
        }

        CPR_ERROR("%s: Thread attempted to destroy another thread, not itself.",
            __FUNCTION__);
        MOZ_ASSERT(PR_FALSE);
        errno = EINVAL;
        return CPR_FAILURE;
    }

    CPR_ERROR("%s - NULL pointer passed in.", __FUNCTION__);
    MOZ_ASSERT(PR_FALSE);
    errno = EINVAL;
    return CPR_FAILURE;
};





















cprRC_t
cprAdjustRelativeThreadPriority (int relPri)
{
    
    return (CPR_FAILURE);
}
