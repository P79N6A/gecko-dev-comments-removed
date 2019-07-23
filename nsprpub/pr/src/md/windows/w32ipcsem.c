









































#include "primpl.h"

#ifdef WINCE
static HANDLE OpenSemaphore(DWORD inDesiredAccess,
                            BOOL inInheritHandle,
                            const char *inName)
{
    HANDLE retval = NULL;
    HANDLE semaphore = NULL;
    PRUnichar wideName[MAX_PATH];  
    
    MultiByteToWideChar(CP_ACP, 0, inName, -1, wideName, MAX_PATH);
    
    semaphore = CreateSemaphoreW(NULL, 0, 0x7fffffff, wideName);
    if (NULL != semaphore) {
        DWORD lastErr = GetLastError();
      
        if (ERROR_ALREADY_EXISTS != lastErr)
            CloseHandle(semaphore);
        else
            retval = semaphore;
    }
    return retval;
}
#endif











static DWORD semAccessTable[] = {
    STANDARD_RIGHTS_REQUIRED|0x1, 
    STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE|SEMAPHORE_MODIFY_STATE, 
    0 
};

#ifndef _PR_GLOBAL_THREADS_ONLY












struct WaitSingleArg {
    HANDLE handle;
    DWORD timeout;
    DWORD rv;
    DWORD error;
};

static void WaitSingleThread(void *arg)
{
    struct WaitSingleArg *warg = (struct WaitSingleArg *) arg;

    warg->rv = WaitForSingleObject(warg->handle, warg->timeout);
    if (warg->rv == WAIT_FAILED) {
        warg->error = GetLastError();
    }
}

static DWORD FiberSafeWaitForSingleObject(
    HANDLE hHandle,
    DWORD dwMilliseconds
)
{
    PRThread *me = _PR_MD_CURRENT_THREAD();

    if (_PR_IS_NATIVE_THREAD(me)) {
        return WaitForSingleObject(hHandle, dwMilliseconds);
    } else {
        PRThread *waitThread;
        struct WaitSingleArg warg;
        PRStatus rv;

        warg.handle = hHandle;
        warg.timeout = dwMilliseconds;
        waitThread = PR_CreateThread(
            PR_USER_THREAD, WaitSingleThread, &warg,
            PR_PRIORITY_NORMAL, PR_GLOBAL_THREAD, PR_JOINABLE_THREAD, 0);
        if (waitThread == NULL) {
            return WAIT_FAILED;
        }

        rv = PR_JoinThread(waitThread);
        PR_ASSERT(rv == PR_SUCCESS);
        if (rv == PR_FAILURE) {
            return WAIT_FAILED;
        }
        if (warg.rv == WAIT_FAILED) {
            SetLastError(warg.error);
        }
        return warg.rv;
    }
}

#endif 

PRSem *_PR_MD_OPEN_SEMAPHORE(
    const char *osname, PRIntn flags, PRIntn mode, PRUintn value)
{
    PRSem *sem;
    SECURITY_ATTRIBUTES sa;
    LPSECURITY_ATTRIBUTES lpSA = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
    PACL pACL = NULL;

    sem = PR_NEW(PRSem);
    if (sem == NULL) {
        PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
        return NULL;
    }
    if (flags & PR_SEM_CREATE) {
        if (_PR_NT_MakeSecurityDescriptorACL(mode, semAccessTable,
                &pSD, &pACL) == PR_SUCCESS) {
            sa.nLength = sizeof(sa);
            sa.lpSecurityDescriptor = pSD;
            sa.bInheritHandle = FALSE;
            lpSA = &sa;
        }
#ifdef WINCE
        {
            
            PRUnichar wosname[MAX_PATH]; 
            MultiByteToWideChar(CP_ACP, 0, osname, -1, wosname, MAX_PATH);
            sem->sem = CreateSemaphoreW(lpSA, value, 0x7fffffff, wosname);
        }
#else
        sem->sem = CreateSemaphoreA(lpSA, value, 0x7fffffff, osname);
#endif
        if (lpSA != NULL) {
            _PR_NT_FreeSecurityDescriptorACL(pSD, pACL);
        }
        if (sem->sem == NULL) {
            _PR_MD_MAP_DEFAULT_ERROR(GetLastError());
            PR_DELETE(sem);
            return NULL;
        }
        if ((flags & PR_SEM_EXCL) && (GetLastError() == ERROR_ALREADY_EXISTS)) {
            PR_SetError(PR_FILE_EXISTS_ERROR, ERROR_ALREADY_EXISTS);
            CloseHandle(sem->sem);
            PR_DELETE(sem);
            return NULL;
        }
    } else {
        sem->sem = OpenSemaphore(
                SEMAPHORE_MODIFY_STATE|SYNCHRONIZE, FALSE, osname);
        if (sem->sem == NULL) {
            DWORD err = GetLastError();

            




            if (err == ERROR_INVALID_NAME) {
                PR_SetError(PR_FILE_NOT_FOUND_ERROR, err);
            } else {
                _PR_MD_MAP_DEFAULT_ERROR(GetLastError());
            }
            PR_DELETE(sem);
            return NULL;
        }
    }
    return sem;
}

PRStatus _PR_MD_WAIT_SEMAPHORE(PRSem *sem)
{
    DWORD rv;

#ifdef _PR_GLOBAL_THREADS_ONLY
    rv = WaitForSingleObject(sem->sem, INFINITE);
#else
    rv = FiberSafeWaitForSingleObject(sem->sem, INFINITE);
#endif
    PR_ASSERT(rv == WAIT_FAILED || rv == WAIT_OBJECT_0);
    if (rv == WAIT_FAILED) {
        _PR_MD_MAP_DEFAULT_ERROR(GetLastError());
        return PR_FAILURE;
    }
    if (rv != WAIT_OBJECT_0) {
        
        PR_SetError(PR_UNKNOWN_ERROR, 0);
        return PR_FAILURE;
    }
    return PR_SUCCESS;
}

PRStatus _PR_MD_POST_SEMAPHORE(PRSem *sem)
{
    if (ReleaseSemaphore(sem->sem, 1, NULL) == FALSE) {
        _PR_MD_MAP_DEFAULT_ERROR(GetLastError());
        return PR_FAILURE;
    }
    return PR_SUCCESS;
}

PRStatus _PR_MD_CLOSE_SEMAPHORE(PRSem *sem)
{
    if (CloseHandle(sem->sem) == FALSE) {
        _PR_MD_MAP_CLOSE_ERROR(GetLastError());
        return PR_FAILURE;
    }
    PR_DELETE(sem);
    return PR_SUCCESS;
}
