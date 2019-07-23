





































#include "nsThreadManager.h"
#include "nsThread.h"
#include "nsThreadUtils.h"
#include "nsIClassInfoImpl.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "nsAutoLock.h"

#ifdef NS_TLS
NS_TLS bool gTLSIsMainThread = false;
#endif

typedef nsTArray< nsRefPtr<nsThread> > nsThreadArray;



static void
ReleaseObject(void *data)
{
  static_cast<nsISupports *>(data)->Release();
}

static PLDHashOperator
AppendAndRemoveThread(const void *key, nsRefPtr<nsThread> &thread, void *arg)
{
  nsThreadArray *threads = static_cast<nsThreadArray *>(arg);
  threads->AppendElement(thread);
  return PL_DHASH_REMOVE;
}



nsThreadManager nsThreadManager::sInstance;


NS_IMETHODIMP_(nsrefcnt) nsThreadManager::AddRef() { return 2; }
NS_IMETHODIMP_(nsrefcnt) nsThreadManager::Release() { return 1; }
NS_IMPL_QUERY_INTERFACE1_CI(nsThreadManager, nsIThreadManager)
NS_IMPL_CI_INTERFACE_GETTER1(nsThreadManager, nsIThreadManager)



nsresult
nsThreadManager::Init()
{
  mLock = PR_NewLock();
  if (!mLock)
    return NS_ERROR_OUT_OF_MEMORY;

  if (!mThreadsByPRThread.Init())
    return NS_ERROR_OUT_OF_MEMORY;

  if (PR_NewThreadPrivateIndex(&mCurThreadIndex, ReleaseObject) == PR_FAILURE)
    return NS_ERROR_FAILURE;

  
  mMainThread = new nsThread();
  if (!mMainThread)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv = mMainThread->InitCurrentThread();
  if (NS_FAILED(rv)) {
    mMainThread = nsnull;
    return rv;
  }

  
  
  mMainThread->GetPRThread(&mMainPRThread);

#ifdef NS_TLS
  gTLSIsMainThread = true;
#endif

  mInitialized = PR_TRUE;
  return NS_OK;
}

void
nsThreadManager::Shutdown()
{
  NS_ASSERTION(NS_IsMainThread(), "shutdown not called from main thread");

  
  
  
  
  
  mInitialized = PR_FALSE;

  
  NS_ProcessPendingEvents(mMainThread);

  
  
  nsThreadArray threads;
  {
    nsAutoLock lock(mLock);
    mThreadsByPRThread.Enumerate(AppendAndRemoveThread, &threads);
  }

  
  
  
  
  
  
  
  

  
  for (PRUint32 i = 0; i < threads.Length(); ++i) {
    nsThread *thread = threads[i];
    if (thread->ShutdownRequired())
      thread->Shutdown();
  }

  
  NS_ProcessPendingEvents(mMainThread);

  

  
  {
    nsAutoLock lock(mLock);
    mThreadsByPRThread.Clear();
  }

  
  
  
  mMainThread->SetObserver(nsnull);

  
  mMainThread = nsnull;

  
  PR_SetThreadPrivate(mCurThreadIndex, nsnull);

  
  PR_DestroyLock(mLock);
  mLock = nsnull;
}

void
nsThreadManager::RegisterCurrentThread(nsThread *thread)
{
  NS_ASSERTION(thread->GetPRThread() == PR_GetCurrentThread(), "bad thread");

  nsAutoLock lock(mLock);

  mThreadsByPRThread.Put(thread->GetPRThread(), thread);  

  NS_ADDREF(thread);  
  PR_SetThreadPrivate(mCurThreadIndex, thread);
}

void
nsThreadManager::UnregisterCurrentThread(nsThread *thread)
{
  NS_ASSERTION(thread->GetPRThread() == PR_GetCurrentThread(), "bad thread");

  nsAutoLock lock(mLock);

  mThreadsByPRThread.Remove(thread->GetPRThread());

  PR_SetThreadPrivate(mCurThreadIndex, nsnull);
  
}

nsThread *
nsThreadManager::GetCurrentThread()
{
  
  void *data = PR_GetThreadPrivate(mCurThreadIndex);
  if (data)
    return static_cast<nsThread *>(data);

  if (!mInitialized) {
    return nsnull;
  }

  
  nsRefPtr<nsThread> thread = new nsThread();
  if (!thread || NS_FAILED(thread->InitCurrentThread()))
    return nsnull;

  return thread.get();  
}

NS_IMETHODIMP
nsThreadManager::NewThread(PRUint32 creationFlags, nsIThread **result)
{
  
  NS_ENSURE_TRUE(mInitialized, NS_ERROR_NOT_INITIALIZED);

  nsThread *thr = new nsThread();
  if (!thr)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(thr);

  nsresult rv = thr->Init();
  if (NS_FAILED(rv)) {
    NS_RELEASE(thr);
    return rv;
  }

  
  
  

  *result = thr;
  return NS_OK;
}

NS_IMETHODIMP
nsThreadManager::GetThreadFromPRThread(PRThread *thread, nsIThread **result)
{
  
  NS_ENSURE_TRUE(mMainThread, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(thread);

  nsRefPtr<nsThread> temp;
  {
    nsAutoLock lock(mLock);
    mThreadsByPRThread.Get(thread, getter_AddRefs(temp));
  }

  NS_IF_ADDREF(*result = temp);
  return NS_OK;
}

NS_IMETHODIMP
nsThreadManager::GetMainThread(nsIThread **result)
{
  
  NS_ENSURE_TRUE(mMainThread, NS_ERROR_NOT_INITIALIZED);
  NS_ADDREF(*result = mMainThread);
  return NS_OK;
}

NS_IMETHODIMP
nsThreadManager::GetCurrentThread(nsIThread **result)
{
  
  NS_ENSURE_TRUE(mMainThread, NS_ERROR_NOT_INITIALIZED);
  *result = GetCurrentThread();
  if (!*result)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*result);
  return NS_OK;
}

NS_IMETHODIMP
nsThreadManager::GetIsMainThread(PRBool *result)
{
  

  *result = (PR_GetCurrentThread() == mMainPRThread);
  return NS_OK;
}
