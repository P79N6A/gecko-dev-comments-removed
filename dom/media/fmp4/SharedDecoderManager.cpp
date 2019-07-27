





#include "SharedDecoderManager.h"
#include "mp4_demuxer/DecoderData.h"

namespace mozilla {

class SharedDecoderCallback : public MediaDataDecoderCallback
{
public:
  explicit SharedDecoderCallback(SharedDecoderManager* aManager) : mManager(aManager) {}

  virtual void Output(MediaData* aData) MOZ_OVERRIDE
  {
    if (mManager->mActiveCallback) {
      mManager->mActiveCallback->Output(aData);
    }
  }
  virtual void Error() MOZ_OVERRIDE
  {
    if (mManager->mActiveCallback) {
      mManager->mActiveCallback->Error();
    }
  }
  virtual void InputExhausted() MOZ_OVERRIDE
  {
    if (mManager->mActiveCallback) {
      mManager->mActiveCallback->InputExhausted();
    }
  }
  virtual void DrainComplete() MOZ_OVERRIDE
  {
    if (mManager->mActiveCallback) {
      mManager->DrainComplete();
    }
  }
  virtual void NotifyResourcesStatusChanged() MOZ_OVERRIDE
  {
    if (mManager->mActiveCallback) {
      mManager->mActiveCallback->NotifyResourcesStatusChanged();
    }
  }
  virtual void ReleaseMediaResources() MOZ_OVERRIDE
  {
    if (mManager->mActiveCallback) {
      mManager->mActiveCallback->ReleaseMediaResources();
    }
  }

  SharedDecoderManager* mManager;
};

SharedDecoderManager::SharedDecoderManager()
  : mTaskQueue(new FlushableMediaTaskQueue(GetMediaThreadPool()))
  , mActiveProxy(nullptr)
  , mActiveCallback(nullptr)
  , mWaitForInternalDrain(false)
  , mMonitor("SharedDecoderProxy")
  , mDecoderReleasedResources(false)
{
  MOZ_ASSERT(NS_IsMainThread()); 
  mCallback = new SharedDecoderCallback(this);
}

SharedDecoderManager::~SharedDecoderManager() {}

already_AddRefed<MediaDataDecoder>
SharedDecoderManager::CreateVideoDecoder(
  PlatformDecoderModule* aPDM,
  const mp4_demuxer::VideoDecoderConfig& aConfig,
  layers::LayersBackend aLayersBackend, layers::ImageContainer* aImageContainer,
  FlushableMediaTaskQueue* aVideoTaskQueue, MediaDataDecoderCallback* aCallback)
{
  if (!mDecoder) {
    
    
    
    
    mDecoder = aPDM->CreateVideoDecoder(
      aConfig, aLayersBackend, aImageContainer, mTaskQueue, mCallback);
    if (!mDecoder) {
      return nullptr;
    }
    nsresult rv = mDecoder->Init();
    NS_ENSURE_SUCCESS(rv, nullptr);
  }

  nsRefPtr<SharedDecoderProxy> proxy(new SharedDecoderProxy(this, aCallback));
  return proxy.forget();
}

bool
SharedDecoderManager::Recreate(PlatformDecoderModule* aPDM,
                               const mp4_demuxer::VideoDecoderConfig& aConfig,
                               layers::LayersBackend aLayersBackend,
                               layers::ImageContainer* aImageContainer)
{
  mDecoder->Flush();
  mDecoder->Shutdown();
  mDecoder = aPDM->CreateVideoDecoder(aConfig, aLayersBackend, aImageContainer, mTaskQueue, mCallback);
  if (!mDecoder) {
    return false;
  }
  nsresult rv = mDecoder->Init();
  return rv == NS_OK;
}

void
SharedDecoderManager::Select(SharedDecoderProxy* aProxy)
{
  if (mActiveProxy == aProxy) {
    return;
  }
  SetIdle(mActiveProxy);

  mActiveProxy = aProxy;
  mActiveCallback = aProxy->mCallback;

  if (mDecoderReleasedResources) {
    mDecoder->AllocateMediaResources();
    mDecoderReleasedResources = false;
  }
}

void
SharedDecoderManager::SetIdle(MediaDataDecoder* aProxy)
{
  if (aProxy && mActiveProxy == aProxy) {
    mWaitForInternalDrain = true;
    mActiveProxy->Drain();
    MonitorAutoLock mon(mMonitor);
    while (mWaitForInternalDrain) {
      mon.Wait();
    }
    mActiveProxy->Flush();
    mActiveProxy = nullptr;
  }
}

void
SharedDecoderManager::DrainComplete()
{
  if (mWaitForInternalDrain) {
    MonitorAutoLock mon(mMonitor);
    mWaitForInternalDrain = false;
    mon.NotifyAll();
  } else {
    mActiveCallback->DrainComplete();
  }
}

void
SharedDecoderManager::ReleaseMediaResources()
{
  mDecoderReleasedResources = true;
  mDecoder->ReleaseMediaResources();
  mActiveProxy = nullptr;
}

void
SharedDecoderManager::Shutdown()
{
  if (mDecoder) {
    mDecoder->Shutdown();
    mDecoder = nullptr;
  }
  if (mTaskQueue) {
    mTaskQueue->BeginShutdown();
    mTaskQueue->AwaitShutdownAndIdle();
    mTaskQueue = nullptr;
  }
}

SharedDecoderProxy::SharedDecoderProxy(
  SharedDecoderManager* aManager, MediaDataDecoderCallback* aCallback)
  : mManager(aManager), mCallback(aCallback)
{
}

SharedDecoderProxy::~SharedDecoderProxy() { Shutdown(); }

nsresult
SharedDecoderProxy::Init()
{
  return NS_OK;
}

nsresult
SharedDecoderProxy::Input(mp4_demuxer::MP4Sample* aSample)
{
  if (mManager->mActiveProxy != this) {
    mManager->Select(this);
  }
  return mManager->mDecoder->Input(aSample);
}

nsresult
SharedDecoderProxy::Flush()
{
  if (mManager->mActiveProxy == this) {
    return mManager->mDecoder->Flush();
  }
  return NS_OK;
}

nsresult
SharedDecoderProxy::Drain()
{
  if (mManager->mActiveProxy == this) {
    return mManager->mDecoder->Drain();
  } else {
    mCallback->DrainComplete();
    return NS_OK;
  }
}

nsresult
SharedDecoderProxy::Shutdown()
{
  mManager->SetIdle(this);
  return NS_OK;
}

bool
SharedDecoderProxy::IsWaitingMediaResources()
{
  if (mManager->mActiveProxy == this) {
    return mManager->mDecoder->IsWaitingMediaResources();
  }
  return mManager->mActiveProxy != nullptr;
}

bool
SharedDecoderProxy::IsDormantNeeded()
{
  return mManager->mDecoder->IsDormantNeeded();
}

void
SharedDecoderProxy::ReleaseMediaResources()
{
  if (mManager->mActiveProxy == this) {
    mManager->ReleaseMediaResources();
  }
}

bool
SharedDecoderProxy::IsHardwareAccelerated() const
{
  return mManager->mDecoder->IsHardwareAccelerated();
}

}
