





#ifndef mozilla_AppleVTDecoder_h
#define mozilla_AppleVTDecoder_h

#include "AppleVDADecoder.h"

#include "VideoToolbox/VideoToolbox.h"

namespace mozilla {

class AppleVTDecoder : public AppleVDADecoder {
public:
  AppleVTDecoder(const mp4_demuxer::VideoDecoderConfig& aConfig,
                 MediaTaskQueue* aVideoTaskQueue,
                 MediaDataDecoderCallback* aCallback,
                 layers::ImageContainer* aImageContainer);
  ~AppleVTDecoder();
  virtual nsresult Init() MOZ_OVERRIDE;
  virtual nsresult Input(mp4_demuxer::MP4Sample* aSample) MOZ_OVERRIDE;
  virtual nsresult Flush() MOZ_OVERRIDE;
  virtual nsresult Drain() MOZ_OVERRIDE;
  virtual nsresult Shutdown() MOZ_OVERRIDE;

private:
  CMVideoFormatDescriptionRef mFormat;
  VTDecompressionSessionRef mSession;

  
  nsresult SubmitFrame(mp4_demuxer::MP4Sample* aSample);
  
  nsresult InitializeSession();
  nsresult WaitForAsynchronousFrames();
  CFDictionaryRef CreateDecoderSpecification();
  CFDictionaryRef CreateDecoderExtensions();
};

} 

#endif 
