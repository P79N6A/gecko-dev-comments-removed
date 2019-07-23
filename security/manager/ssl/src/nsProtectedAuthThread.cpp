



































#include "pk11func.h"
#include "nsCOMPtr.h"
#include "nsProxiedService.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsProtectedAuthThread.h"

NS_IMPL_THREADSAFE_ISUPPORTS1(nsProtectedAuthThread, nsIProtectedAuthThread)

static void PR_CALLBACK nsProtectedAuthThreadRunner(void *arg)
{
    nsProtectedAuthThread *self = static_cast<nsProtectedAuthThread *>(arg);
    self->Run();
}

nsProtectedAuthThread::nsProtectedAuthThread()
: mMutex(nsnull)
, mStatusDialogPtr(nsnull)
, mIAmRunning(PR_FALSE)
, mStatusDialogClosed(PR_FALSE)
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

    if (mStatusDialogPtr)
    {
        NS_RELEASE(mStatusDialogPtr);
    }
}

NS_IMETHODIMP nsProtectedAuthThread::Login(nsIDOMWindowInternal *statusDialog)
{
    if (!mMutex)
        return NS_ERROR_FAILURE;
    
    if (!statusDialog )
        return NS_ERROR_FAILURE;

    if (!mSlot)
        
        return NS_ERROR_FAILURE;
    
    nsCOMPtr<nsIDOMWindowInternal> wi;
    NS_GetProxyForObject( NS_PROXY_TO_MAIN_THREAD,
                          nsIDOMWindowInternal::GetIID(),
                          statusDialog,
                          NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                          getter_AddRefs(wi));

    PR_Lock(mMutex);
    
    if (mIAmRunning || mLoginReady) {
        PR_Unlock(mMutex);
        return NS_OK;
    }

    mStatusDialogPtr = wi;
    NS_ADDREF(mStatusDialogPtr);
    wi = 0;
    
    mIAmRunning = PR_TRUE;
    
    mThreadHandle = PR_CreateThread(PR_USER_THREAD, nsProtectedAuthThreadRunner, static_cast<void*>(this), 
        PR_PRIORITY_NORMAL, PR_LOCAL_THREAD, PR_JOINABLE_THREAD, 0);
    
    
    
    NS_ASSERTION(mThreadHandle, "Could not create nsProtectedAuthThreadRunner thread\n");
    
    PR_Unlock(mMutex);
    
    return NS_OK;
}

NS_IMETHODIMP nsProtectedAuthThread::GetTokenName(PRUnichar **_retval)
{
    PR_Lock(mMutex);

    
    *_retval = UTF8ToNewUnicode(nsDependentCString(PK11_GetTokenName(mSlot)));

    PR_Unlock(mMutex);

    return NS_OK;
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
    
    nsIDOMWindowInternal *windowToClose = 0;
    
    PR_Lock(mMutex);
    
    mLoginReady = PR_TRUE;
    mIAmRunning = PR_FALSE;

    
    if (mSlot)
    {
        PK11_FreeSlot(mSlot);
        mSlot = 0;
    }
    
    if (!mStatusDialogClosed)
    {
        windowToClose = mStatusDialogPtr;
    }
        
    mStatusDialogPtr = 0;
    mStatusDialogClosed = PR_TRUE;
    
    PR_Unlock(mMutex);
    
    if (windowToClose)
        windowToClose->Close();
}

void nsProtectedAuthThread::Join()
{
    if (!mThreadHandle)
        return;
    
    PR_JoinThread(mThreadHandle);
    mThreadHandle = nsnull;
}
