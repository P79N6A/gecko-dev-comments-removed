





#ifndef mozilla_AppleVTDecoder_h
#define mozilla_AppleVTDecoder_h

#include "AppleVDADecoder.h"

#include "VideoToolbox/VideoToolbox.h"

namespace mozilla {

class AppleVTDecoder : public AppleVDADecoder {
public:
  AppleVTDecoder(const VideoInfo& aConfig,
                 FlushableMediaTaskQueue* aVideoTaskQueue,
                 MediaDataDecoderCallback* aCallback,
                 layers::ImageContainer* aImageContainer);
  virtual ~AppleVTDecoder();
  virtual nsresult Init() override;
  virtual nsresult Input(MediaRawData* aSample) override;
  virtual nsresult Flush() override;
  virtual nsresult Drain() override;
  virtual nsresult Shutdown() override;
  virtual bool IsHardwareAccelerated() const override
  {
    return mIsHardwareAccelerated;
  }

private:
  CMVideoFormatDescriptionRef mFormat;
  VTDecompressionSessionRef mSession;

  
  nsresult SubmitFrame(MediaRawData* aSample);
  
  nsresult InitializeSession();
  nsresult WaitForAsynchronousFrames();
  CFDictionaryRef CreateDecoderSpecification();
  CFDictionaryRef CreateDecoderExtensions();
  bool mIsHardwareAccelerated;
};

} 

#endif 
