

















 
#include "primpl.h"








static void
AddThreadToCVWaitQueueInternal(PRThread *thred, struct _MDCVar *cv)
{
    PR_ASSERT((cv->waitTail != NULL && cv->waitHead != NULL)
            || (cv->waitTail == NULL && cv->waitHead == NULL));
    cv->nwait += 1;
    thred->md.inCVWaitQueue = PR_TRUE;
    thred->md.next = NULL;
    thred->md.prev = cv->waitTail;
    if (cv->waitHead == NULL) {
        cv->waitHead = thred;
    } else {
        cv->waitTail->md.next = thred;
    }
    cv->waitTail = thred;
}











static void
md_UnlockAndPostNotifies(
    _MDLock *lock,
    PRThread *waitThred,
    _MDCVar *waitCV)
{
    PRIntn index;
    _MDNotified post;
    _MDNotified *notified, *prev = NULL;

    





    post = lock->notified;  

#if defined(DEBUG)
    ZeroMemory(&lock->notified, sizeof(_MDNotified));  
#else
    lock->notified.length = 0;  
    lock->notified.link = NULL;
#endif

    


    notified = &post;  
    do {
        for (index = 0; index < notified->length; ++index) {
            _MDCVar *cv = notified->cv[index].cv;
            PRThread *thred;
            int i;
            
            
            if (cv->waitHead == NULL) {
                notified->cv[index].notifyHead = NULL;
                continue;
            }

            
            if (-1 == notified->cv[index].times) {
                
                thred = cv->waitHead;
                while (thred != NULL) {
                    thred->md.inCVWaitQueue = PR_FALSE;
                    thred = thred->md.next;
                }
                notified->cv[index].notifyHead = cv->waitHead;
                cv->waitHead = cv->waitTail = NULL;
                cv->nwait = 0;
            } else {
                thred = cv->waitHead;
                i = notified->cv[index].times;
                while (thred != NULL && i > 0) {
                    thred->md.inCVWaitQueue = PR_FALSE;
                    thred = thred->md.next;
                    i--;
                }
                notified->cv[index].notifyHead = cv->waitHead;
                cv->waitHead = thred;
                if (cv->waitHead == NULL) {
                    cv->waitTail = NULL;
                } else {
                    if (cv->waitHead->md.prev != NULL) {
                        cv->waitHead->md.prev->md.next = NULL;
                        cv->waitHead->md.prev = NULL;
                    }
                }
                cv->nwait -= notified->cv[index].times - i;
            }
        }
        notified = notified->link;
    } while (NULL != notified);

    if (waitThred) {
        AddThreadToCVWaitQueueInternal(waitThred, waitCV);
    }

    
        LeaveCriticalSection(&lock->mutex);

    notified = &post;  
    do {
        for (index = 0; index < notified->length; ++index) {
            PRThread *thred;
            PRThread *next;

            thred = notified->cv[index].notifyHead;
            while (thred != NULL) {
                BOOL rv;

                next = thred->md.next;
                thred->md.prev = thred->md.next = NULL;

                rv = ReleaseSemaphore(thred->md.blocked_sema, 1, NULL);
                PR_ASSERT(rv != 0);
                thred = next;
            }
        }
        prev = notified;
        notified = notified->link;
        if (&post != prev) PR_DELETE(prev);
    } while (NULL != notified);
}






static void md_PostNotifyToCvar(_MDCVar *cvar, _MDLock *lock,
        PRBool broadcast)
{
    PRIntn index = 0;
    _MDNotified *notified = &lock->notified;

    while (1) {
        for (index = 0; index < notified->length; ++index) {
            if (notified->cv[index].cv == cvar) {
                if (broadcast) {
                    notified->cv[index].times = -1;
                } else if (-1 != notified->cv[index].times) {
                    notified->cv[index].times += 1;
                }
                return;
            }
        }
        
        if (notified->length < _MD_CV_NOTIFIED_LENGTH) break;

        
        if (NULL == notified->link) {
            notified->link = PR_NEWZAP(_MDNotified);
        }

        notified = notified->link;
    }

    
    notified->cv[index].times = (broadcast) ? -1 : 1;
    notified->cv[index].cv = cvar;
    notified->length += 1;
}









PRInt32 
_PR_MD_NEW_CV(_MDCVar *cv)
{
    cv->magic = _MD_MAGIC_CV;
    



    return 0;
} 

void _PR_MD_FREE_CV(_MDCVar *cv)
{
    cv->magic = (PRUint32)-1;
    return;
}




void _PR_MD_WAIT_CV(_MDCVar *cv, _MDLock *lock, PRIntervalTime timeout )
{
    PRThread *thred = _PR_MD_CURRENT_THREAD();
    DWORD rv;
    DWORD msecs = (timeout == PR_INTERVAL_NO_TIMEOUT) ?
            INFINITE : PR_IntervalToMilliseconds(timeout);

    


    if (0 != lock->notified.length) {
        md_UnlockAndPostNotifies(lock, thred, cv);
    } else {
        AddThreadToCVWaitQueueInternal(thred, cv);
        LeaveCriticalSection(&lock->mutex);
    }

    
    rv = WaitForSingleObject(thred->md.blocked_sema, msecs);

    EnterCriticalSection(&(lock->mutex));

    PR_ASSERT(rv != WAIT_ABANDONED);
    PR_ASSERT(rv != WAIT_FAILED);
    PR_ASSERT(rv != WAIT_OBJECT_0 || thred->md.inCVWaitQueue == PR_FALSE);

    if (rv == WAIT_TIMEOUT) {
        if (thred->md.inCVWaitQueue) {
            PR_ASSERT((cv->waitTail != NULL && cv->waitHead != NULL)
                    || (cv->waitTail == NULL && cv->waitHead == NULL));
            cv->nwait -= 1;
            thred->md.inCVWaitQueue = PR_FALSE;
            if (cv->waitHead == thred) {
                cv->waitHead = thred->md.next;
                if (cv->waitHead == NULL) {
                    cv->waitTail = NULL;
                } else {
                    cv->waitHead->md.prev = NULL;
                }
            } else {
                PR_ASSERT(thred->md.prev != NULL);
                thred->md.prev->md.next = thred->md.next;
                if (thred->md.next != NULL) {
                    thred->md.next->md.prev = thred->md.prev;
                } else {
                    PR_ASSERT(cv->waitTail == thred);
                    cv->waitTail = thred->md.prev;
                }
            }
            thred->md.next = thred->md.prev = NULL;
        } else {
            





            rv = WaitForSingleObject(thred->md.blocked_sema, INFINITE);
            PR_ASSERT(rv == WAIT_OBJECT_0);
        }
    }
    PR_ASSERT(thred->md.inCVWaitQueue == PR_FALSE);
    return;
} 

void _PR_MD_NOTIFY_CV(_MDCVar *cv, _MDLock *lock)
{
    md_PostNotifyToCvar(cv, lock, PR_FALSE);
    return;
}

void _PR_MD_NOTIFYALL_CV(_MDCVar *cv, _MDLock *lock)
{
    md_PostNotifyToCvar(cv, lock, PR_TRUE);
    return;
}

typedef BOOL (WINAPI *INITIALIZECRITICALSECTIONEX)(
    CRITICAL_SECTION *lpCriticalSection,
    DWORD dwSpinCount,
    DWORD Flags);

static INITIALIZECRITICALSECTIONEX sInitializeCriticalSectionEx;

void _PR_MD_INIT_LOCKS(void)
{
    







    HMODULE hKernel32 = GetModuleHandle("kernel32.dll");
    PR_ASSERT(hKernel32);
    PR_ASSERT(!sInitializeCriticalSectionEx);
    sInitializeCriticalSectionEx = (INITIALIZECRITICALSECTIONEX)
            GetProcAddress(hKernel32, "InitializeCriticalSectionEx");
}







#define LOCK_SPIN_COUNT 1500

PRStatus _PR_MD_NEW_LOCK(_MDLock *lock)
{
    CRITICAL_SECTION *cs = &lock->mutex;
    BOOL ok;

    if (sInitializeCriticalSectionEx) {
        ok = sInitializeCriticalSectionEx(cs, LOCK_SPIN_COUNT,
                                          CRITICAL_SECTION_NO_DEBUG_INFO);
    } else {
        ok = InitializeCriticalSectionAndSpinCount(cs, LOCK_SPIN_COUNT);
    }
    if (!ok) {
        _PR_MD_MAP_DEFAULT_ERROR(GetLastError());
        return PR_FAILURE;
    }

    lock->notified.length = 0;
    lock->notified.link = NULL;
    return PR_SUCCESS;
}

void _PR_MD_UNLOCK(_MDLock *lock)
{
    if (0 != lock->notified.length) {
        md_UnlockAndPostNotifies(lock, NULL, NULL);
    } else {
        LeaveCriticalSection(&lock->mutex);
    }
}
