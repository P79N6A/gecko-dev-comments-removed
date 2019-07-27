





#include "SharedDecoderManager.h"
#include "mp4_demuxer/DecoderData.h"

namespace mozilla {

class SharedDecoderCallback : public MediaDataDecoderCallback
{
public:
  explicit SharedDecoderCallback(SharedDecoderManager* aManager)
    : mManager(aManager)
  {
  }

  virtual void Output(MediaData* aData) override
  {
    if (mManager->mActiveCallback) {
      mManager->mActiveCallback->Output(aData);
    }
  }
  virtual void Error() override
  {
    if (mManager->mActiveCallback) {
      mManager->mActiveCallback->Error();
    }
  }
  virtual void InputExhausted() override
  {
    if (mManager->mActiveCallback) {
      mManager->mActiveCallback->InputExhausted();
    }
  }
  virtual void DrainComplete() override
  {
    if (mManager->mActiveCallback) {
      mManager->DrainComplete();
    }
  }
  virtual void NotifyResourcesStatusChanged() override
  {
    if (mManager->mActiveCallback) {
      mManager->mActiveCallback->NotifyResourcesStatusChanged();
    }
  }
  virtual void ReleaseMediaResources() override
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

SharedDecoderManager::~SharedDecoderManager()
{
}

already_AddRefed<MediaDataDecoder>
SharedDecoderManager::CreateVideoDecoder(
  PlatformDecoderModule* aPDM,
  const mp4_demuxer::VideoDecoderConfig& aConfig,
  layers::LayersBackend aLayersBackend,
  layers::ImageContainer* aImageContainer,
  FlushableMediaTaskQueue* aVideoTaskQueue,
  MediaDataDecoderCallback* aCallback)
{
  if (!mDecoder) {
    mLayersBackend = aLayersBackend;
    mImageContainer = aImageContainer;
    
    
    
    
    mDecoder =
      aPDM->CreateDecoder(aConfig,
                          mTaskQueue,
                          mCallback,
                          mLayersBackend,
                          mImageContainer);
    if (!mDecoder) {
      mPDM = nullptr;
      return nullptr;
    }
    mPDM = aPDM;
    nsresult rv = mDecoder->Init();
    NS_ENSURE_SUCCESS(rv, nullptr);
  }

  nsRefPtr<SharedDecoderProxy> proxy(new SharedDecoderProxy(this, aCallback));
  return proxy.forget();
}

void
SharedDecoderManager::DisableHardwareAcceleration()
{
  MOZ_ASSERT(mPDM);
  mPDM->DisableHardwareAcceleration();
}

bool
SharedDecoderManager::Recreate(const mp4_demuxer::VideoDecoderConfig& aConfig)
{
  mDecoder->Flush();
  mDecoder->Shutdown();
  mDecoder = mPDM->CreateDecoder(aConfig,
                                 mTaskQueue,
                                 mCallback,
                                 mLayersBackend,
                                 mImageContainer);
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
SharedDecoderManager::Shutdown()
{
  if (mDecoder) {
    mDecoder->Shutdown();
    mDecoder = nullptr;
  }
  mPDM = nullptr;
  if (mTaskQueue) {
    mTaskQueue->BeginShutdown();
    mTaskQueue->AwaitShutdownAndIdle();
    mTaskQueue = nullptr;
  }
}

SharedDecoderProxy::SharedDecoderProxy(SharedDecoderManager* aManager,
                                       MediaDataDecoderCallback* aCallback)
  : mManager(aManager)
  , mCallback(aCallback)
{
}

SharedDecoderProxy::~SharedDecoderProxy()
{
  Shutdown();
}

nsresult
SharedDecoderProxy::Init()
{
  return NS_OK;
}

nsresult
SharedDecoderProxy::Input(MediaRawData* aSample)
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
SharedDecoderProxy::IsHardwareAccelerated() const
{
  return mManager->mDecoder->IsHardwareAccelerated();
}

}
