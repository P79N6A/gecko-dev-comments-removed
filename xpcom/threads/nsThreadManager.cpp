





#include "nsThreadManager.h"
#include "nsThread.h"
#include "nsThreadUtils.h"
#include "nsIClassInfoImpl.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "mozilla/ThreadLocal.h"
#ifdef MOZ_CANARY
#include <fcntl.h>
#include <unistd.h>
#endif

using namespace mozilla;

static mozilla::ThreadLocal<bool> sTLSIsMainThread;

bool
NS_IsMainThread()
{
  return sTLSIsMainThread.get();
}

void
NS_SetMainThread()
{
  if (!sTLSIsMainThread.initialized()) {
    if (!sTLSIsMainThread.init()) {
      MOZ_CRASH();
    }
    sTLSIsMainThread.set(true);
  }
  MOZ_ASSERT(NS_IsMainThread());
}

typedef nsTArray<nsRefPtr<nsThread>> nsThreadArray;



static void
ReleaseObject(void* aData)
{
  static_cast<nsISupports*>(aData)->Release();
}


NS_IMETHODIMP_(MozExternalRefCountType)
nsThreadManager::AddRef()
{
  return 2;
}
NS_IMETHODIMP_(MozExternalRefCountType)
nsThreadManager::Release()
{
  return 1;
}
NS_IMPL_CLASSINFO(nsThreadManager, nullptr,
                  nsIClassInfo::THREADSAFE | nsIClassInfo::SINGLETON,
                  NS_THREADMANAGER_CID)
NS_IMPL_QUERY_INTERFACE_CI(nsThreadManager, nsIThreadManager)
NS_IMPL_CI_INTERFACE_GETTER(nsThreadManager, nsIThreadManager)



nsresult
nsThreadManager::Init()
{
  
  
  
  if (mInitialized) {
    return NS_OK;
  }

  if (PR_NewThreadPrivateIndex(&mCurThreadIndex, ReleaseObject) == PR_FAILURE) {
    return NS_ERROR_FAILURE;
  }


#ifdef MOZ_CANARY
  const int flags = O_WRONLY | O_APPEND | O_CREAT | O_NONBLOCK;
  const mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
  char* env_var_flag = getenv("MOZ_KILL_CANARIES");
  sCanaryOutputFD =
    env_var_flag ? (env_var_flag[0] ? open(env_var_flag, flags, mode) :
                                      STDERR_FILENO) :
                   0;
#endif

  
  mMainThread = new nsThread(nsThread::MAIN_THREAD, 0);

  nsresult rv = mMainThread->InitCurrentThread();
  if (NS_FAILED(rv)) {
    mMainThread = nullptr;
    return rv;
  }

  
  
  mMainThread->GetPRThread(&mMainPRThread);

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
    OffTheBooksMutexAutoLock lock(mLock);
    for (auto iter = mThreadsByPRThread.Iter(); !iter.Done(); iter.Next()) {
      nsRefPtr<nsThread>& thread = iter.GetData();
      threads.AppendElement(thread);
      iter.Remove();
    }
  }

  
  
  
  
  
  
  
  

  
  for (uint32_t i = 0; i < threads.Length(); ++i) {
    nsThread* thread = threads[i];
    if (thread->ShutdownRequired()) {
      thread->Shutdown();
    }
  }

  
  NS_ProcessPendingEvents(mMainThread);

  

  
  {
    OffTheBooksMutexAutoLock lock(mLock);
    mThreadsByPRThread.Clear();
  }

  
  
  
  mMainThread->SetObserver(nullptr);
  mMainThread->ClearObservers();

  
  mMainThread = nullptr;

  
  PR_SetThreadPrivate(mCurThreadIndex, nullptr);
}

void
nsThreadManager::RegisterCurrentThread(nsThread* aThread)
{
  MOZ_ASSERT(aThread->GetPRThread() == PR_GetCurrentThread(), "bad aThread");

  OffTheBooksMutexAutoLock lock(mLock);

  ++mCurrentNumberOfThreads;
  if (mCurrentNumberOfThreads > mHighestNumberOfThreads) {
    mHighestNumberOfThreads = mCurrentNumberOfThreads;
  }

  mThreadsByPRThread.Put(aThread->GetPRThread(), aThread);  

  NS_ADDREF(aThread);  
  PR_SetThreadPrivate(mCurThreadIndex, aThread);
}

void
nsThreadManager::UnregisterCurrentThread(nsThread* aThread)
{
  MOZ_ASSERT(aThread->GetPRThread() == PR_GetCurrentThread(), "bad aThread");

  OffTheBooksMutexAutoLock lock(mLock);

  --mCurrentNumberOfThreads;
  mThreadsByPRThread.Remove(aThread->GetPRThread());

  PR_SetThreadPrivate(mCurThreadIndex, nullptr);
  
}

nsThread*
nsThreadManager::GetCurrentThread()
{
  
  void* data = PR_GetThreadPrivate(mCurThreadIndex);
  if (data) {
    return static_cast<nsThread*>(data);
  }

  if (!mInitialized) {
    return nullptr;
  }

  
  nsRefPtr<nsThread> thread = new nsThread(nsThread::NOT_MAIN_THREAD, 0);
  if (!thread || NS_FAILED(thread->InitCurrentThread())) {
    return nullptr;
  }

  return thread.get();  
}

NS_IMETHODIMP
nsThreadManager::NewThread(uint32_t aCreationFlags,
                           uint32_t aStackSize,
                           nsIThread** aResult)
{
  
  
  
  if (NS_WARN_IF(!mInitialized)) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  nsRefPtr<nsThread> thr = new nsThread(nsThread::NOT_MAIN_THREAD, aStackSize);
  nsresult rv = thr->Init();  
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  
  
  

  if (NS_WARN_IF(!mInitialized)) {
    if (thr->ShutdownRequired()) {
      thr->Shutdown(); 
    }
    return NS_ERROR_NOT_INITIALIZED;
  }

  thr.forget(aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsThreadManager::GetThreadFromPRThread(PRThread* aThread, nsIThread** aResult)
{
  
  if (NS_WARN_IF(!mMainThread)) {
    return NS_ERROR_NOT_INITIALIZED;
  }
  if (NS_WARN_IF(!aThread)) {
    return NS_ERROR_INVALID_ARG;
  }

  nsRefPtr<nsThread> temp;
  {
    OffTheBooksMutexAutoLock lock(mLock);
    mThreadsByPRThread.Get(aThread, getter_AddRefs(temp));
  }

  NS_IF_ADDREF(*aResult = temp);
  return NS_OK;
}

NS_IMETHODIMP
nsThreadManager::GetMainThread(nsIThread** aResult)
{
  
  if (NS_WARN_IF(!mMainThread)) {
    return NS_ERROR_NOT_INITIALIZED;
  }
  NS_ADDREF(*aResult = mMainThread);
  return NS_OK;
}

NS_IMETHODIMP
nsThreadManager::GetCurrentThread(nsIThread** aResult)
{
  
  if (NS_WARN_IF(!mMainThread)) {
    return NS_ERROR_NOT_INITIALIZED;
  }
  *aResult = GetCurrentThread();
  if (!*aResult) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  NS_ADDREF(*aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsThreadManager::GetIsMainThread(bool* aResult)
{
  

  *aResult = (PR_GetCurrentThread() == mMainPRThread);
  return NS_OK;
}

uint32_t
nsThreadManager::GetHighestNumberOfThreads()
{
  OffTheBooksMutexAutoLock lock(mLock);
  return mHighestNumberOfThreads;
}
