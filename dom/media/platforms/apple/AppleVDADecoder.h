





#ifndef mozilla_AppleVDADecoder_h
#define mozilla_AppleVDADecoder_h

#include "PlatformDecoderModule.h"
#include "mozilla/ReentrantMonitor.h"
#include "MP4Reader.h"
#include "MP4Decoder.h"
#include "nsIThread.h"
#include "ReorderQueue.h"
#include "TimeUnits.h"

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
    media::TimeUnit decode_timestamp;
    media::TimeUnit composition_timestamp;
    media::TimeUnit duration;
    int64_t byte_offset;
    bool is_sync_point;

    explicit AppleFrameRef(const MediaRawData& aSample)
      : decode_timestamp(media::TimeUnit::FromMicroseconds(aSample.mTimecode))
      , composition_timestamp(media::TimeUnit::FromMicroseconds(aSample.mTime))
      , duration(media::TimeUnit::FromMicroseconds(aSample.mDuration))
      , byte_offset(aSample.mOffset)
      , is_sync_point(aSample.mKeyframe)
    {
    }

    AppleFrameRef(const media::TimeUnit& aDts,
                  const media::TimeUnit& aPts,
                  const media::TimeUnit& aDuration,
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
    const VideoInfo& aConfig,
    FlushableMediaTaskQueue* aVideoTaskQueue,
    MediaDataDecoderCallback* aCallback,
    layers::ImageContainer* aImageContainer);

  AppleVDADecoder(const VideoInfo& aConfig,
                  FlushableMediaTaskQueue* aVideoTaskQueue,
                  MediaDataDecoderCallback* aCallback,
                  layers::ImageContainer* aImageContainer);
  virtual ~AppleVDADecoder();
  virtual nsresult Init() override;
  virtual nsresult Input(MediaRawData* aSample) override;
  virtual nsresult Flush() override;
  virtual nsresult Drain() override;
  virtual nsresult Shutdown() override;
  virtual bool IsHardwareAccelerated() const override
  {
    return true;
  }

  nsresult OutputFrame(CVPixelBufferRef aImage,
                       nsAutoPtr<AppleFrameRef> aFrameRef);

 protected:
  AppleFrameRef* CreateAppleFrameRef(const MediaRawData* aSample);
  void DrainReorderedFrames();
  void ClearReorderedFrames();
  CFDictionaryRef CreateOutputConfiguration();

  nsRefPtr<MediaByteBuffer> mExtraData;
  nsRefPtr<FlushableMediaTaskQueue> mTaskQueue;
  MediaDataDecoderCallback* mCallback;
  nsRefPtr<layers::ImageContainer> mImageContainer;
  ReorderQueue mReorderQueue;
  uint32_t mPictureWidth;
  uint32_t mPictureHeight;
  uint32_t mDisplayWidth;
  uint32_t mDisplayHeight;
  uint32_t mMaxRefFrames;

private:
  VDADecoder mDecoder;
  bool mIs106;

  
  nsresult SubmitFrame(MediaRawData* aSample);
  
  nsresult InitializeSession();
  CFDictionaryRef CreateDecoderSpecification();
};

} 

#endif 
