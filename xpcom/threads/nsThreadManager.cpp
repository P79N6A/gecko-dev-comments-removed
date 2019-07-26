





#include "nsThreadManager.h"
#include "nsThread.h"
#include "nsThreadUtils.h"
#include "nsIClassInfoImpl.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectorUtils.h"

using namespace mozilla;

#ifdef XP_WIN
#include <windows.h>
DWORD gTLSThreadIDIndex = TlsAlloc();
#elif defined(NS_TLS)
NS_TLS mozilla::threads::ID gTLSThreadID = mozilla::threads::Generic;
#endif

typedef nsTArray< nsRefPtr<nsThread> > nsThreadArray;



static void
ReleaseObject(void *data)
{
  static_cast<nsISupports *>(data)->Release();
}

static PLDHashOperator
AppendAndRemoveThread(PRThread *key, nsRefPtr<nsThread> &thread, void *arg)
{
  nsThreadArray *threads = static_cast<nsThreadArray *>(arg);
  threads->AppendElement(thread);
  return PL_DHASH_REMOVE;
}


NS_IMETHODIMP_(nsrefcnt) nsThreadManager::AddRef() { return 2; }
NS_IMETHODIMP_(nsrefcnt) nsThreadManager::Release() { return 1; }
NS_IMPL_CLASSINFO(nsThreadManager, NULL,
                  nsIClassInfo::THREADSAFE | nsIClassInfo::SINGLETON,
                  NS_THREADMANAGER_CID)
NS_IMPL_QUERY_INTERFACE1_CI(nsThreadManager, nsIThreadManager)
NS_IMPL_CI_INTERFACE_GETTER1(nsThreadManager, nsIThreadManager)



nsresult
nsThreadManager::Init()
{
  mThreadsByPRThread.Init();

  if (PR_NewThreadPrivateIndex(&mCurThreadIndex, ReleaseObject) == PR_FAILURE)
    return NS_ERROR_FAILURE;

  mLock = new Mutex("nsThreadManager.mLock");

  
  mMainThread = new nsThread(nsThread::MAIN_THREAD, 0);
  if (!mMainThread)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv = mMainThread->InitCurrentThread();
  if (NS_FAILED(rv)) {
    mMainThread = nullptr;
    return rv;
  }

  
  
  mMainThread->GetPRThread(&mMainPRThread);

#ifdef XP_WIN
  TlsSetValue(gTLSThreadIDIndex, (void*) mozilla::threads::Main);
#elif defined(NS_TLS)
  gTLSThreadID = mozilla::threads::Main;
#endif

  mInitialized = true;
  return NS_OK;
}

void
nsThreadManager::Shutdown()
{
  MOZ_ASSERT(NS_IsMainThread(), "shutdown not called from main thread");

  
  
  
  
  
  mInitialized = false;

  
  NS_ProcessPendingEvents(mMainThread);

  
  
  nsThreadArray threads;
  {
    MutexAutoLock lock(*mLock);
    mThreadsByPRThread.Enumerate(AppendAndRemoveThread, &threads);
  }

  
  
  
  
  
  
  
  

  
  for (uint32_t i = 0; i < threads.Length(); ++i) {
    nsThread *thread = threads[i];
    if (thread->ShutdownRequired())
      thread->Shutdown();
  }

  
  NS_ProcessPendingEvents(mMainThread);

  

  
  {
    MutexAutoLock lock(*mLock);
    mThreadsByPRThread.Clear();
  }

  
  
  
  mMainThread->SetObserver(nullptr);
  mMainThread->ClearObservers();

  
  mMainThread = nullptr;
  mLock = nullptr;

  
  PR_SetThreadPrivate(mCurThreadIndex, nullptr);
}

void
nsThreadManager::RegisterCurrentThread(nsThread *thread)
{
  MOZ_ASSERT(thread->GetPRThread() == PR_GetCurrentThread(), "bad thread");

  MutexAutoLock lock(*mLock);

  ++mCurrentNumberOfThreads;
  if (mCurrentNumberOfThreads > mHighestNumberOfThreads) {
    mHighestNumberOfThreads = mCurrentNumberOfThreads;
  }

  mThreadsByPRThread.Put(thread->GetPRThread(), thread);  

  NS_ADDREF(thread);  
  PR_SetThreadPrivate(mCurThreadIndex, thread);
}

void
nsThreadManager::UnregisterCurrentThread(nsThread *thread)
{
  MOZ_ASSERT(thread->GetPRThread() == PR_GetCurrentThread(), "bad thread");

  MutexAutoLock lock(*mLock);

  --mCurrentNumberOfThreads;
  mThreadsByPRThread.Remove(thread->GetPRThread());

  PR_SetThreadPrivate(mCurThreadIndex, nullptr);
  
}

nsThread *
nsThreadManager::GetCurrentThread()
{
  
  void *data = PR_GetThreadPrivate(mCurThreadIndex);
  if (data)
    return static_cast<nsThread *>(data);

  if (!mInitialized) {
    return nullptr;
  }

  
  nsRefPtr<nsThread> thread = new nsThread(nsThread::NOT_MAIN_THREAD, 0);
  if (!thread || NS_FAILED(thread->InitCurrentThread()))
    return nullptr;

  return thread.get();  
}

NS_IMETHODIMP
nsThreadManager::NewThread(uint32_t creationFlags,
                           uint32_t stackSize,
                           nsIThread **result)
{
  
  NS_ENSURE_TRUE(mInitialized, NS_ERROR_NOT_INITIALIZED);

  nsThread *thr = new nsThread(nsThread::NOT_MAIN_THREAD, stackSize);
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
    MutexAutoLock lock(*mLock);
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
nsThreadManager::GetIsMainThread(bool *result)
{
  

  *result = (PR_GetCurrentThread() == mMainPRThread);
  return NS_OK;
}

NS_IMETHODIMP
nsThreadManager::GetIsCycleCollectorThread(bool *result)
{
  *result = bool(NS_IsCycleCollectorThread());
  return NS_OK;
}

uint32_t
nsThreadManager::GetHighestNumberOfThreads()
{
  MutexAutoLock lock(*mLock);
  return mHighestNumberOfThreads;
}
