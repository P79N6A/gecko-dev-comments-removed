





#ifndef MOZILLA_SUBBUFFERDECODER_H_
#define MOZILLA_SUBBUFFERDECODER_H_

#include "BufferDecoder.h"
#include "SourceBufferResource.h"

namespace mozilla {

class MediaResource;
class MediaSourceDecoder;
class MediaDecoderReader;

namespace dom {

class TimeRanges;

} 

class SubBufferDecoder : public BufferDecoder
{
public:
  
  
  SubBufferDecoder(MediaResource* aResource, MediaSourceDecoder* aParentDecoder)
    : BufferDecoder(aResource), mParentDecoder(aParentDecoder), mReader(nullptr)
    , mMediaDuration(-1), mDiscarded(false)
  {
  }

  void SetReader(MediaDecoderReader* aReader)
  {
    MOZ_ASSERT(!mReader);
    mReader = aReader;
  }

  MediaDecoderReader* GetReader()
  {
    return mReader;
  }

  virtual ReentrantMonitor& GetReentrantMonitor() MOZ_OVERRIDE;
  virtual bool OnStateMachineThread() const MOZ_OVERRIDE;
  virtual bool OnDecodeThread() const MOZ_OVERRIDE;
  virtual SourceBufferResource* GetResource() const MOZ_OVERRIDE;
  virtual void NotifyDecodedFrames(uint32_t aParsed, uint32_t aDecoded) MOZ_OVERRIDE;
  virtual void SetMediaDuration(int64_t aDuration) MOZ_OVERRIDE;
  virtual void UpdateEstimatedMediaDuration(int64_t aDuration) MOZ_OVERRIDE;
  virtual void SetMediaSeekable(bool aMediaSeekable) MOZ_OVERRIDE;
  virtual layers::ImageContainer* GetImageContainer() MOZ_OVERRIDE;
  virtual MediaDecoderOwner* GetOwner() MOZ_OVERRIDE;

  
  
  void NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset);
  nsresult GetBuffered(dom::TimeRanges* aBuffered);

  
  
  int64_t ConvertToByteOffset(double aTime);

  int64_t GetMediaDuration() MOZ_OVERRIDE
  {
    return mMediaDuration;
  }

  bool IsDiscarded()
  {
    return mDiscarded;
  }

  void SetDiscarded()
  {
    GetResource()->Ended();
    mDiscarded = true;
  }

private:
  MediaSourceDecoder* mParentDecoder;
  nsRefPtr<MediaDecoderReader> mReader;
  int64_t mMediaDuration;
  bool mDiscarded;
};

} 

#endif 
