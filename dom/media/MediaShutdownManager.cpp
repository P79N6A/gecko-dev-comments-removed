





#include "MediaShutdownManager.h"
#include "nsContentUtils.h"
#include "mozilla/StaticPtr.h"
#include "MediaDecoder.h"
#include "SharedThreadPool.h"
#include "mozilla/Logging.h"

namespace mozilla {

extern PRLogModuleInfo* gMediaDecoderLog;
#define DECODER_LOG(type, msg) MOZ_LOG(gMediaDecoderLog, type, msg)

NS_IMPL_ISUPPORTS(MediaShutdownManager, nsIObserver)

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
  bool needShutdownObserver = mDecoders.Count() > 0;
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
                              const char16_t *someData)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID) == 0) {
    Shutdown();
  }
  return NS_OK;
}

static PLDHashOperator
ShutdownMediaDecoder(nsRefPtrHashKey<MediaDecoder>* aEntry, void*)
{
  aEntry->GetKey()->Shutdown();
  return PL_DHASH_REMOVE;
}

void
MediaShutdownManager::Shutdown()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(sInstance);

  DECODER_LOG(LogLevel::Debug, ("MediaShutdownManager::Shutdown() start..."));

  
  
  
  mIsDoingXPCOMShutDown = true;

  
  
  mDecoders.EnumerateEntries(ShutdownMediaDecoder, nullptr);

  
  
  
  SharedThreadPool::SpinUntilEmpty();

  
  
  nsContentUtils::UnregisterShutdownObserver(this);

  
  
  
  
  sInstance = nullptr;

  DECODER_LOG(LogLevel::Debug, ("MediaShutdownManager::Shutdown() end."));
}

} 
