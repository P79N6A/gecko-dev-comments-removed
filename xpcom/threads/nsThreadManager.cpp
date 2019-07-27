





#include "nsThreadManager.h"
#include "nsThread.h"
#include "nsThreadUtils.h"
#include "nsIClassInfoImpl.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "mozilla/ThreadLocal.h"
#include "mozilla/ReentrantMonitor.h"
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

#ifdef MOZ_NUWA_PROCESS
class NotifyAllThreadsWereIdle: public nsRunnable
{
public:

  NotifyAllThreadsWereIdle(
    nsTArray<nsRefPtr<nsThreadManager::AllThreadsWereIdleListener>>* aListeners)
    : mListeners(aListeners)
  {
  }

  virtual NS_IMETHODIMP
  Run() {
    
    nsTArray<nsRefPtr<nsThreadManager::AllThreadsWereIdleListener>> arr(*mListeners);
    for (size_t i = 0; i < arr.Length(); i++) {
      arr[i]->OnAllThreadsWereIdle();
    }
    return NS_OK;
  }

private:
  
  nsTArray<nsRefPtr<nsThreadManager::AllThreadsWereIdleListener>>* mListeners;
};

struct nsThreadManager::ThreadStatusInfo {
  Atomic<bool> mWorking;
  Atomic<bool> mWillBeWorking;
  bool mIgnored;
  ThreadStatusInfo()
    : mWorking(false)
    , mWillBeWorking(false)
    , mIgnored(false)
  {
  }
};
#endif 



static void
ReleaseObject(void* aData)
{
  static_cast<nsISupports*>(aData)->Release();
}

#ifdef MOZ_NUWA_PROCESS
void
nsThreadManager::DeleteThreadStatusInfo(void* aData)
{
  nsThreadManager* mgr = nsThreadManager::get();
  nsThreadManager::ThreadStatusInfo* thrInfo =
    static_cast<nsThreadManager::ThreadStatusInfo*>(aData);
  {
    ReentrantMonitorAutoEnter mon(*(mgr->mMonitor));
    mgr->mThreadStatusInfos.RemoveElement(thrInfo);
    if (NS_IsMainThread()) {
      mgr->mMainThreadStatusInfo = nullptr;
    }
  }
  delete thrInfo;
}
#endif

static PLDHashOperator
AppendAndRemoveThread(PRThread* aKey, nsRefPtr<nsThread>& aThread, void* aArg)
{
  nsThreadArray* threads = static_cast<nsThreadArray*>(aArg);
  threads->AppendElement(aThread);
  return PL_DHASH_REMOVE;
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

#ifdef MOZ_NUWA_PROCESS
  if (PR_NewThreadPrivateIndex(
      &mThreadStatusInfoIndex,
      nsThreadManager::DeleteThreadStatusInfo) == PR_FAILURE) {
    return NS_ERROR_FAILURE;
  }
#endif 

#ifdef MOZ_NUWA_PROCESS
  mMonitor = MakeUnique<ReentrantMonitor>("nsThreadManager.mMonitor");
#endif 

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
    mThreadsByPRThread.Enumerate(AppendAndRemoveThread, &threads);
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
#ifdef MOZ_NUWA_PROCESS
  PR_SetThreadPrivate(mThreadStatusInfoIndex, nullptr);
#endif
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
  
#ifdef MOZ_NUWA_PROCESS
  PR_SetThreadPrivate(mThreadStatusInfoIndex, nullptr);
#endif
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

#ifdef MOZ_NUWA_PROCESS
nsThreadManager::ThreadStatusInfo*
nsThreadManager::GetCurrentThreadStatusInfo()
{
  void* data = PR_GetThreadPrivate(mThreadStatusInfoIndex);
  if (!data) {
    ThreadStatusInfo *thrInfo = new ThreadStatusInfo();
    PR_SetThreadPrivate(mThreadStatusInfoIndex, thrInfo);
    data = thrInfo;

    ReentrantMonitorAutoEnter mon(*mMonitor);
    mThreadStatusInfos.AppendElement(thrInfo);
    if (NS_IsMainThread()) {
      mMainThreadStatusInfo = thrInfo;
    }
  }

  return static_cast<ThreadStatusInfo*>(data);
}
#endif

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

NS_IMETHODIMP
nsThreadManager::SetIgnoreThreadStatus()
{
#ifdef MOZ_NUWA_PROCESS
  GetCurrentThreadStatusInfo()->mIgnored = true;
  return NS_OK;
#else
  return NS_ERROR_NOT_IMPLEMENTED;
#endif
}

#ifdef MOZ_NUWA_PROCESS
void
nsThreadManager::SetThreadIdle(nsIRunnable **aReturnRunnable)
{
  SetThreadIsWorking(GetCurrentThreadStatusInfo(), false, aReturnRunnable);
}

void
nsThreadManager::SetThreadWorking()
{
  SetThreadIsWorking(GetCurrentThreadStatusInfo(), true, nullptr);
}

void
nsThreadManager::SetThreadIsWorking(ThreadStatusInfo* aInfo,
                                    bool aIsWorking,
                                    nsIRunnable **aReturnRunnable)
{
  aInfo->mWillBeWorking = aIsWorking;
  if (mThreadsIdledListeners.Length() > 0) {

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    bool hasWorkingThread = false;
    nsRefPtr<NotifyAllThreadsWereIdle> runnable;
    {
      ReentrantMonitorAutoEnter mon(*mMonitor);
      
      aInfo->mWorking = aIsWorking;
      if (aIsWorking) {
        
        return;
      }

      for (size_t i = 0; i < mThreadStatusInfos.Length(); i++) {
        ThreadStatusInfo *info = mThreadStatusInfos[i];
        if (!info->mIgnored) {
          if (info->mWorking) {
            if (info->mWillBeWorking) {
              hasWorkingThread = true;
              break;
            }
          }
        }
      }
      if (!hasWorkingThread && !mDispatchingToMainThread) {
        runnable = new NotifyAllThreadsWereIdle(&mThreadsIdledListeners);
        mDispatchingToMainThread = true;
      }
    }

    if (runnable) {
      if (NS_IsMainThread()) {
        
        
        
        
        MOZ_ASSERT(aReturnRunnable,
                   "aReturnRunnable must be provided on main thread");
        runnable.forget(aReturnRunnable);
      } else {
        NS_DispatchToMainThread(runnable);
        ResetIsDispatchingToMainThread();
      }
    }
  } else {
    
    aInfo->mWorking = aIsWorking;
  }
}

void
nsThreadManager::ResetIsDispatchingToMainThread()
{
  ReentrantMonitorAutoEnter mon(*mMonitor);
  mDispatchingToMainThread = false;
}

void
nsThreadManager::AddAllThreadsWereIdleListener(AllThreadsWereIdleListener *listener)
{
  MOZ_ASSERT(GetCurrentThreadStatusInfo()->mWorking);
  mThreadsIdledListeners.AppendElement(listener);
}

void
nsThreadManager::RemoveAllThreadsWereIdleListener(AllThreadsWereIdleListener *listener)
{
  mThreadsIdledListeners.RemoveElement(listener);
}

#endif 
