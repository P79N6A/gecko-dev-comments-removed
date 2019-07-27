





#ifndef mozilla_AppleVTDecoder_h
#define mozilla_AppleVTDecoder_h

#include "PlatformDecoderModule.h"
#include "mozilla/RefPtr.h"
#include "mozilla/ReentrantMonitor.h"
#include "nsIThread.h"
#include "ReorderQueue.h"

#include "VideoToolbox/VideoToolbox.h"

namespace mozilla {

class MediaTaskQueue;
class MediaDataDecoderCallback;
namespace layers {
  class ImageContainer;
}
class FrameRef;

class AppleVTDecoder : public MediaDataDecoder {
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
  
  nsresult OutputFrame(CVPixelBufferRef aImage,
                       nsAutoPtr<FrameRef> frameRef);
private:
  const mp4_demuxer::VideoDecoderConfig& mConfig;
  RefPtr<MediaTaskQueue> mTaskQueue;
  MediaDataDecoderCallback* mCallback;
  layers::ImageContainer* mImageContainer;
  CMVideoFormatDescriptionRef mFormat;
  VTDecompressionSessionRef mSession;
  ReorderQueue mReorderQueue;

  
  nsresult SubmitFrame(mp4_demuxer::MP4Sample* aSample);
  
  nsresult InitializeSession();
  nsresult WaitForAsynchronousFrames();
  void DrainReorderedFrames();
  void ClearReorderedFrames();

  CFDictionaryRef CreateDecoderSpecification();
};

} 

#endif 
