





#ifndef mozilla_AppleVDADecoder_h
#define mozilla_AppleVDADecoder_h

#include "PlatformDecoderModule.h"
#include "mozilla/ReentrantMonitor.h"
#include "MP4Reader.h"
#include "MP4Decoder.h"
#include "nsIThread.h"
#include "ReorderQueue.h"

#include "VideoDecodeAcceleration/VDADecoder.h"

namespace mozilla {

class FlushableMediaTaskQueue;
class MediaDataDecoderCallback;
namespace layers {
  class ImageContainer;
}

class AppleVDADecoder : public MediaDataDecoder {
public:
  class AppleFrameRef {
  public:
    Microseconds decode_timestamp;
    Microseconds composition_timestamp;
    Microseconds duration;
    int64_t byte_offset;
    bool is_sync_point;

    explicit AppleFrameRef(const mp4_demuxer::MP4Sample& aSample)
    : decode_timestamp(aSample.decode_timestamp)
    , composition_timestamp(aSample.composition_timestamp)
    , duration(aSample.duration)
    , byte_offset(aSample.byte_offset)
    , is_sync_point(aSample.is_sync_point)
    {
    }

    AppleFrameRef(Microseconds aDts,
                  Microseconds aPts,
                  Microseconds aDuration,
                  int64_t aByte_offset,
                  bool aIs_sync_point)
    : decode_timestamp(aDts)
    , composition_timestamp(aPts)
    , duration(aDuration)
    , byte_offset(aByte_offset)
    , is_sync_point(aIs_sync_point)
    {
    }
  };

  
  
  static already_AddRefed<AppleVDADecoder> CreateVDADecoder(
    const mp4_demuxer::VideoDecoderConfig& aConfig,
    FlushableMediaTaskQueue* aVideoTaskQueue,
    MediaDataDecoderCallback* aCallback,
    layers::ImageContainer* aImageContainer);

  AppleVDADecoder(const mp4_demuxer::VideoDecoderConfig& aConfig,
                  FlushableMediaTaskQueue* aVideoTaskQueue,
                  MediaDataDecoderCallback* aCallback,
                  layers::ImageContainer* aImageContainer);
  virtual ~AppleVDADecoder();
  virtual nsresult Init() MOZ_OVERRIDE;
  virtual nsresult Input(mp4_demuxer::MP4Sample* aSample) MOZ_OVERRIDE;
  virtual nsresult Flush() MOZ_OVERRIDE;
  virtual nsresult Drain() MOZ_OVERRIDE;
  virtual nsresult Shutdown() MOZ_OVERRIDE;
  virtual bool IsHardwareAccelerated() const MOZ_OVERRIDE
  {
    return true;
  }

  nsresult OutputFrame(CVPixelBufferRef aImage,
                       nsAutoPtr<AppleFrameRef> aFrameRef);

 protected:
  AppleFrameRef* CreateAppleFrameRef(const mp4_demuxer::MP4Sample* aSample);
  void DrainReorderedFrames();
  void ClearReorderedFrames();
  CFDictionaryRef CreateOutputConfiguration();

  const mp4_demuxer::VideoDecoderConfig& mConfig;
  nsRefPtr<FlushableMediaTaskQueue> mTaskQueue;
  MediaDataDecoderCallback* mCallback;
  nsRefPtr<layers::ImageContainer> mImageContainer;
  ReorderQueue mReorderQueue;
  uint32_t mPictureWidth;
  uint32_t mPictureHeight;
  uint32_t mMaxRefFrames;

private:
  VDADecoder mDecoder;
  bool mIs106;

  
  nsresult SubmitFrame(mp4_demuxer::MP4Sample* aSample);
  
  nsresult InitializeSession();
  CFDictionaryRef CreateDecoderSpecification();
};

} 

#endif 
