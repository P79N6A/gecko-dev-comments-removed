





#ifndef mozilla_H264Converter_h
#define mozilla_H264Converter_h

#include "PlatformDecoderModule.h"

namespace mozilla {








class H264Converter : public MediaDataDecoder {
public:

  H264Converter(PlatformDecoderModule* aPDM,
                const mp4_demuxer::VideoDecoderConfig& aConfig,
                layers::LayersBackend aLayersBackend,
                layers::ImageContainer* aImageContainer,
                FlushableMediaTaskQueue* aVideoTaskQueue,
                MediaDataDecoderCallback* aCallback);
  virtual ~H264Converter();

  virtual nsresult Init() override;
  virtual nsresult Input(mp4_demuxer::MP4Sample* aSample) override;
  virtual nsresult Flush() override;
  virtual nsresult Drain() override;
  virtual nsresult Shutdown() override;
  virtual bool IsWaitingMediaResources() override;
  virtual bool IsDormantNeeded() override;
  virtual void AllocateMediaResources() override;
  virtual void ReleaseMediaResources() override;
  virtual bool IsHardwareAccelerated() const override;

  
  static bool IsH264(const mp4_demuxer::TrackConfig& aConfig);

private:
  
  
  
  nsresult CreateDecoder();
  nsresult CreateDecoderAndInit(mp4_demuxer::MP4Sample* aSample);
  nsresult CheckForSPSChange(mp4_demuxer::MP4Sample* aSample);
  void UpdateConfigFromExtraData(mp4_demuxer::ByteBuffer* aExtraData);

  nsRefPtr<PlatformDecoderModule> mPDM;
  mp4_demuxer::VideoDecoderConfig mCurrentConfig;
  layers::LayersBackend mLayersBackend;
  nsRefPtr<layers::ImageContainer> mImageContainer;
  nsRefPtr<FlushableMediaTaskQueue> mVideoTaskQueue;
  MediaDataDecoderCallback* mCallback;
  nsRefPtr<MediaDataDecoder> mDecoder;
  bool mNeedAVCC;
  nsresult mLastError;
};

} 

#endif 
