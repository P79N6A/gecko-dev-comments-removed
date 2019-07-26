





#include "MediaShutdownManager.h"
#include "nsContentUtils.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/ClearOnShutdown.h"
#include "MediaDecoder.h"

namespace mozilla {

StateMachineThread::StateMachineThread()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_COUNT_CTOR(StateMachineThread);
}

StateMachineThread::~StateMachineThread()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_COUNT_DTOR(StateMachineThread);
}

void
StateMachineThread::Shutdown()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mThread);
  if (mThread) {
    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(this, &StateMachineThread::ShutdownThread);
    NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
  }
}

void
StateMachineThread::ShutdownThread()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mThread);
  mThread->Shutdown();
  mThread = nullptr;
  MediaShutdownManager::Instance().Unregister(this);
}

nsresult
StateMachineThread::Init()
{
  MOZ_ASSERT(NS_IsMainThread());
  nsresult rv = NS_NewNamedThread("Media State", getter_AddRefs(mThread));
  NS_ENSURE_SUCCESS(rv, rv);
  MediaShutdownManager::Instance().Register(this);
  return NS_OK;
}

nsIThread*
StateMachineThread::GetThread()
{
  MOZ_ASSERT(mThread);
  return mThread;
}

void
StateMachineThread::SpinUntilShutdownComplete()
{
  MOZ_ASSERT(NS_IsMainThread());
  while (mThread) {
    bool processed = false;
    nsresult rv = NS_GetCurrentThread()->ProcessNextEvent(true, &processed);
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to spin main thread while awaiting media shutdown");
      break;
    }
  }
}

NS_IMPL_ISUPPORTS1(MediaShutdownManager, nsIObserver)

MediaShutdownManager::MediaShutdownManager()
  : mIsObservingShutdown(false),
    mIsDoingXPCOMShutDown(false)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_COUNT_CTOR(MediaShutdownManager);
}

MediaShutdownManager::~MediaShutdownManager()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_COUNT_DTOR(MediaShutdownManager);
}



StaticRefPtr<MediaShutdownManager> MediaShutdownManager::sInstance;

MediaShutdownManager&
MediaShutdownManager::Instance()
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!sInstance) {
    sInstance = new MediaShutdownManager();
  }
  return *sInstance;
}

void
MediaShutdownManager::EnsureCorrectShutdownObserverState()
{
  MOZ_ASSERT(!mIsDoingXPCOMShutDown);
  bool needShutdownObserver = (mDecoders.Count() > 0) ||
                              (mStateMachineThreads.Count() > 0);
  if (needShutdownObserver != mIsObservingShutdown) {
    mIsObservingShutdown = needShutdownObserver;
    if (mIsObservingShutdown) {
      nsContentUtils::RegisterShutdownObserver(this);
    } else {
      nsContentUtils::UnregisterShutdownObserver(this);
      
      
      sInstance = nullptr;
    }
  }
}

void
MediaShutdownManager::Register(MediaDecoder* aDecoder)
{
  MOZ_ASSERT(NS_IsMainThread());
  
  
  MOZ_ASSERT(!mDecoders.Contains(aDecoder));
  mDecoders.PutEntry(aDecoder);
  MOZ_ASSERT(mDecoders.Contains(aDecoder));
  MOZ_ASSERT(mDecoders.Count() > 0);
  EnsureCorrectShutdownObserverState();
}

void
MediaShutdownManager::Unregister(MediaDecoder* aDecoder)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mDecoders.Contains(aDecoder));
  if (!mIsDoingXPCOMShutDown) {
    mDecoders.RemoveEntry(aDecoder);
    EnsureCorrectShutdownObserverState();
  }
}

NS_IMETHODIMP
MediaShutdownManager::Observe(nsISupports *aSubjet,
                              const char *aTopic,
                              const PRUnichar *someData)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID) == 0) {
    Shutdown();
  }
  return NS_OK;
}

void
MediaShutdownManager::Register(StateMachineThread* aThread)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!mStateMachineThreads.Contains(aThread));
  mStateMachineThreads.PutEntry(aThread);
  MOZ_ASSERT(mStateMachineThreads.Contains(aThread));
  MOZ_ASSERT(mStateMachineThreads.Count() > 0);
  EnsureCorrectShutdownObserverState();
}

void
MediaShutdownManager::Unregister(StateMachineThread* aThread)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mStateMachineThreads.Contains(aThread));
  if (!mIsDoingXPCOMShutDown) {
    mStateMachineThreads.RemoveEntry(aThread);
    EnsureCorrectShutdownObserverState();
  }
}

static PLDHashOperator
ShutdownMediaDecoder(nsRefPtrHashKey<MediaDecoder>* aEntry, void*)
{
  aEntry->GetKey()->Shutdown();
  return PL_DHASH_REMOVE;
}

static PLDHashOperator
JoinStateMachineThreads(nsRefPtrHashKey<StateMachineThread>* aEntry, void*)
{
  
  
  
  RefPtr<StateMachineThread> thread = aEntry->GetKey();
  thread->SpinUntilShutdownComplete();
  return PL_DHASH_REMOVE;
}

void
MediaShutdownManager::Shutdown()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(sInstance);

  
  
  
  mIsDoingXPCOMShutDown = true;

  
  
  mDecoders.EnumerateEntries(ShutdownMediaDecoder, nullptr);
 
  
  
  
  
  
  mStateMachineThreads.EnumerateEntries(JoinStateMachineThreads, nullptr);
 
  
  
  nsContentUtils::UnregisterShutdownObserver(this);

  
  
  
  
  sInstance = nullptr;
}

} 
