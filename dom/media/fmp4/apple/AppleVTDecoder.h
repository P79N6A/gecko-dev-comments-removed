





#ifndef mozilla_AppleVTDecoder_h
#define mozilla_AppleVTDecoder_h

#include "AppleVDADecoder.h"

#include "VideoToolbox/VideoToolbox.h"

namespace mozilla {

class AppleVTDecoder : public AppleVDADecoder {
public:
  AppleVTDecoder(const mp4_demuxer::VideoDecoderConfig& aConfig,
                 FlushableMediaTaskQueue* aVideoTaskQueue,
                 MediaDataDecoderCallback* aCallback,
                 layers::ImageContainer* aImageContainer);
  virtual ~AppleVTDecoder();
  virtual nsresult Init() MOZ_OVERRIDE;
  virtual nsresult Input(mp4_demuxer::MP4Sample* aSample) MOZ_OVERRIDE;
  virtual nsresult Flush() MOZ_OVERRIDE;
  virtual nsresult Drain() MOZ_OVERRIDE;
  virtual nsresult Shutdown() MOZ_OVERRIDE;
  virtual bool IsHardwareAccelerated() const MOZ_OVERRIDE
  {
    return mIsHardwareAccelerated;
  }

private:
  CMVideoFormatDescriptionRef mFormat;
  VTDecompressionSessionRef mSession;

  
  nsresult SubmitFrame(mp4_demuxer::MP4Sample* aSample);
  
  nsresult InitializeSession();
  nsresult WaitForAsynchronousFrames();
  CFDictionaryRef CreateDecoderSpecification();
  CFDictionaryRef CreateDecoderExtensions();
  bool mIsHardwareAccelerated;
};

} 

#endif 
