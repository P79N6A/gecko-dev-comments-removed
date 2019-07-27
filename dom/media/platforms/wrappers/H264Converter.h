





#ifndef mozilla_H264Converter_h
#define mozilla_H264Converter_h

#include "PlatformDecoderModule.h"

namespace mozilla {








class H264Converter : public MediaDataDecoder {
public:

  H264Converter(PlatformDecoderModule* aPDM,
                const VideoInfo& aConfig,
                layers::LayersBackend aLayersBackend,
                layers::ImageContainer* aImageContainer,
                FlushableMediaTaskQueue* aVideoTaskQueue,
                MediaDataDecoderCallback* aCallback);
  virtual ~H264Converter();

  virtual nsresult Init() override;
  virtual nsresult Input(MediaRawData* aSample) override;
  virtual nsresult Flush() override;
  virtual nsresult Drain() override;
  virtual nsresult Shutdown() override;
  virtual bool IsHardwareAccelerated() const override;

  
  static bool IsH264(const TrackInfo& aConfig);

private:
  
  
  
  nsresult CreateDecoder();
  nsresult CreateDecoderAndInit(MediaRawData* aSample);
  nsresult CheckForSPSChange(MediaRawData* aSample);
  void UpdateConfigFromExtraData(MediaByteBuffer* aExtraData);

  nsRefPtr<PlatformDecoderModule> mPDM;
  VideoInfo mCurrentConfig;
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
