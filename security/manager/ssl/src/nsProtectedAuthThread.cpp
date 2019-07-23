



































#include "pk11func.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsProxiedService.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsPKCS11Slot.h"
#include "nsProtectedAuthThread.h"

NS_IMPL_THREADSAFE_ISUPPORTS1(nsProtectedAuthThread, nsIProtectedAuthThread)

static void PR_CALLBACK nsProtectedAuthThreadRunner(void *arg)
{
    nsProtectedAuthThread *self = static_cast<nsProtectedAuthThread *>(arg);
    self->Run();
}

nsProtectedAuthThread::nsProtectedAuthThread()
: mMutex(nsnull)
, mIAmRunning(PR_FALSE)
, mStatusObserverNotified(PR_FALSE)
, mLoginReady(PR_FALSE)
, mThreadHandle(nsnull)
, mSlot(0)
, mLoginResult(SECFailure)
{
    NS_INIT_ISUPPORTS();
    mMutex = PR_NewLock();
}

nsProtectedAuthThread::~nsProtectedAuthThread()
{
    if (mMutex)
        PR_DestroyLock(mMutex);
}

NS_IMETHODIMP nsProtectedAuthThread::Login(nsIObserver *aObserver)
{
    NS_ENSURE_ARG(aObserver);

    if (!mMutex)
        return NS_ERROR_FAILURE;
    
    if (!mSlot)
        
        return NS_ERROR_FAILURE;

    nsCOMPtr<nsIObserver> observerProxy;
    nsresult rv = NS_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                                       NS_GET_IID(nsIObserver),
                                       aObserver,
                                       NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                                       getter_AddRefs(observerProxy));
    if (NS_FAILED(rv))
        return rv;

    PR_Lock(mMutex);
    
    if (mIAmRunning || mLoginReady) {
        PR_Unlock(mMutex);
        return NS_OK;
    }

    observerProxy.swap(mStatusObserver);
    mIAmRunning = PR_TRUE;
    
    mThreadHandle = PR_CreateThread(PR_USER_THREAD, nsProtectedAuthThreadRunner, static_cast<void*>(this), 
        PR_PRIORITY_NORMAL, PR_LOCAL_THREAD, PR_JOINABLE_THREAD, 0);
    
    
    
    NS_ASSERTION(mThreadHandle, "Could not create nsProtectedAuthThreadRunner thread\n");
    
    PR_Unlock(mMutex);
    
    return NS_OK;
}

NS_IMETHODIMP nsProtectedAuthThread::GetTokenName(nsAString &_retval)
{
    PR_Lock(mMutex);

    
    CopyUTF8toUTF16(nsDependentCString(PK11_GetTokenName(mSlot)), _retval);

    PR_Unlock(mMutex);

    return NS_OK;
}

NS_IMETHODIMP nsProtectedAuthThread::GetSlot(nsIPKCS11Slot **_retval)
{
    PR_Lock(mMutex);

    nsRefPtr<nsPKCS11Slot> slot = new nsPKCS11Slot(mSlot);

    PR_Unlock(mMutex);

    if (!slot)
      return NS_ERROR_OUT_OF_MEMORY;

    return CallQueryInterface (slot.get(), _retval);
}

void nsProtectedAuthThread::SetParams(PK11SlotInfo* aSlot)
{
    PR_Lock(mMutex);

    mSlot = (aSlot) ? PK11_ReferenceSlot(aSlot) : 0;
    
    PR_Unlock(mMutex);
}

SECStatus nsProtectedAuthThread::GetResult()
{
    return mLoginResult;
}

void nsProtectedAuthThread::Run(void)
{
    
    
    mLoginResult = PK11_CheckUserPassword(mSlot, 0);

    nsCOMPtr<nsIObserver> observer;
    
    PR_Lock(mMutex);
    
    mLoginReady = PR_TRUE;
    mIAmRunning = PR_FALSE;

    
    if (mSlot)
    {
        PK11_FreeSlot(mSlot);
        mSlot = 0;
    }
    
    if (!mStatusObserverNotified)
    {
        observer = mStatusObserver;
    }

    mStatusObserver = nsnull;
    mStatusObserverNotified = PR_TRUE;
    
    PR_Unlock(mMutex);
    
    if (observer)
        observer->Observe(nsnull, "operation-completed", nsnull);
}

void nsProtectedAuthThread::Join()
{
    if (!mThreadHandle)
        return;
    
    PR_JoinThread(mThreadHandle);
    mThreadHandle = nsnull;
}
