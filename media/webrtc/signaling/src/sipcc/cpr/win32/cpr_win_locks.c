






































#include "cpr_types.h"
#include "cpr_locks.h"
#include "cpr_win_locks.h" 
#include "cpr_debug.h"
#include "cpr_memory.h"
#include "cpr_stdio.h"
#include "cpr_stdlib.h"
#include <windows.h>


extern CRITICAL_SECTION criticalSection;












void
cprDisableSwap (void)
{
    EnterCriticalSection(&criticalSection);
}











void
cprEnableSwap (void)
{
    LeaveCriticalSection(&criticalSection);
}











cprMutex_t
cprCreateMutex (const char *name)
{
    cpr_mutex_t *cprMutexPtr;
    static char fname[] = "cprCreateMutex";
	WCHAR* wname;

    




    cprMutexPtr = (cpr_mutex_t *) cpr_malloc(sizeof(cpr_mutex_t));
    if (cprMutexPtr != NULL) {
        
        if (name != NULL) {
            cprMutexPtr->name = name;
        }

		wname = cpr_malloc((strlen(name) + 1) * sizeof(WCHAR));
		mbstowcs(wname, name, strlen(name));
        cprMutexPtr->u.handlePtr = CreateMutex(NULL, FALSE, wname);
		cpr_free(wname);

        if (cprMutexPtr->u.handlePtr == NULL) {
            CPR_ERROR("%s - Mutex init failure: %d\n", fname, GetLastError());
            cpr_free(cprMutexPtr);
            cprMutexPtr = NULL;
        }
    }
    return cprMutexPtr;

}











cprRC_t
cprDestroyMutex (cprMutex_t mutex)
{
    cpr_mutex_t *cprMutexPtr;
    const static char fname[] = "cprDestroyMutex";

    cprMutexPtr = (cpr_mutex_t *) mutex;
    if (cprMutexPtr != NULL) {
        CloseHandle(cprMutexPtr->u.handlePtr);
        cpr_free(cprMutexPtr);
        return (CPR_SUCCESS);
        
    } else {
        CPR_ERROR("%s - NULL pointer passed in.\n", fname);
        return (CPR_FAILURE);
    }
}











cprRC_t
cprGetMutex (cprMutex_t mutex)
{
    int32_t rc;
    static const char fname[] = "cprGetMutex";
    cpr_mutex_t *cprMutexPtr;

    cprMutexPtr = (cpr_mutex_t *) mutex;
    if (cprMutexPtr != NULL) {
        




        rc = WaitForSingleObject((HANDLE) cprMutexPtr->u.handlePtr, INFINITE);
        if (rc != WAIT_OBJECT_0) {
            CPR_ERROR("%s - Error acquiring mutex: %d\n", fname, rc);
            return (CPR_FAILURE);
        }

        return (CPR_SUCCESS);

        
    } else {
        CPR_ERROR("%s - NULL pointer passed in.\n", fname);
        return (CPR_FAILURE);
    }
}











cprRC_t
cprReleaseMutex (cprMutex_t mutex)
{
    static const char fname[] = "cprReleaseMutex";
    cpr_mutex_t *cprMutexPtr;

    cprMutexPtr = (cpr_mutex_t *) mutex;
    if (cprMutexPtr != NULL) {
        if (ReleaseMutex(cprMutexPtr->u.handlePtr) == 0) {
            CPR_ERROR("%s - Error releasing mutex: %d\n",
                      fname, GetLastError());
            return (CPR_FAILURE);
        }
        return (CPR_SUCCESS);

        
    } else {
        CPR_ERROR("%s - NULL pointer passed in.\n", fname);
        return (CPR_FAILURE);
    }
}

