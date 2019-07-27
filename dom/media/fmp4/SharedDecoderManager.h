





#ifndef SHARED_DECODER_MANAGER_H_
#define SHARED_DECODER_MANAGER_H_

#include "PlatformDecoderModule.h"
#include "mozilla/Monitor.h"

namespace mozilla
{

class MediaDataDecoder;
class SharedDecoderProxy;
class SharedDecoderCallback;

class SharedDecoderManager
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(SharedDecoderManager)

  SharedDecoderManager();

  already_AddRefed<MediaDataDecoder> CreateVideoDecoder(
    PlatformDecoderModule* aPDM,
    const mp4_demuxer::VideoDecoderConfig& aConfig,
    layers::LayersBackend aLayersBackend,
    layers::ImageContainer* aImageContainer,
    FlushableMediaTaskQueue* aVideoTaskQueue,
    MediaDataDecoderCallback* aCallback);

  void SetReader(MediaDecoderReader* aReader);
  void Select(SharedDecoderProxy* aProxy);
  void SetIdle(MediaDataDecoder* aProxy);
  void ReleaseMediaResources();
  void Shutdown();

  friend class SharedDecoderProxy;
  friend class SharedDecoderCallback;

  void DisableHardwareAcceleration();
  bool Recreate(const mp4_demuxer::VideoDecoderConfig& aConfig);

private:
  virtual ~SharedDecoderManager();
  void DrainComplete();

  nsRefPtr<PlatformDecoderModule> mPDM;
  nsRefPtr<MediaDataDecoder> mDecoder;
  layers::LayersBackend mLayersBackend;
  nsRefPtr<layers::ImageContainer> mImageContainer;
  nsRefPtr<FlushableMediaTaskQueue> mTaskQueue;
  SharedDecoderProxy* mActiveProxy;
  MediaDataDecoderCallback* mActiveCallback;
  nsAutoPtr<MediaDataDecoderCallback> mCallback;
  bool mWaitForInternalDrain;
  Monitor mMonitor;
  bool mDecoderReleasedResources;
};

class SharedDecoderProxy : public MediaDataDecoder
{
public:
  SharedDecoderProxy(SharedDecoderManager* aManager,
                     MediaDataDecoderCallback* aCallback);
  virtual ~SharedDecoderProxy();

  virtual nsresult Init() override;
  virtual nsresult Input(MediaRawData* aSample) override;
  virtual nsresult Flush() override;
  virtual nsresult Drain() override;
  virtual nsresult Shutdown() override;
  virtual bool IsWaitingMediaResources() override;
  virtual bool IsDormantNeeded() override;
  virtual void ReleaseMediaResources() override;
  virtual bool IsHardwareAccelerated() const override;

  friend class SharedDecoderManager;

private:
  nsRefPtr<SharedDecoderManager> mManager;
  MediaDataDecoderCallback* mCallback;
};
}

#endif
