





#ifndef MOZILLA_SUBBUFFERDECODER_H_
#define MOZILLA_SUBBUFFERDECODER_H_

#include "BufferDecoder.h"
#include "SourceBufferResource.h"

namespace mozilla {

class MediaSourceDecoder;

class SubBufferDecoder : public BufferDecoder
{
public:
  
  
  SubBufferDecoder(MediaResource* aResource, MediaSourceDecoder* aParentDecoder)
    : BufferDecoder(aResource), mParentDecoder(aParentDecoder), mReader(nullptr)
    , mMediaDuration(-1), mMediaStartTime(0)
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
  virtual void SetTransportSeekable(bool aTransportSeekable) MOZ_OVERRIDE;
  virtual layers::ImageContainer* GetImageContainer() MOZ_OVERRIDE;
  virtual MediaDecoderOwner* GetOwner() MOZ_OVERRIDE;

  void NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset)
  {
    mReader->NotifyDataArrived(aBuffer, aLength, aOffset);

    
    
    
    
    mParentDecoder->NotifyDataArrived(nullptr, 0, 0);
  }

  nsresult GetBuffered(dom::TimeRanges* aBuffered)
  {
    
    return mReader->GetBuffered(aBuffered, 0);
  }

  
  
  int64_t ConvertToByteOffset(double aTime);

  int64_t GetMediaDuration() MOZ_OVERRIDE
  {
    return mMediaDuration;
  }

  int64_t GetMediaStartTime()
  {
    return mMediaStartTime;
  }

  void SetMediaStartTime(int64_t aMediaStartTime)
  {
    mMediaStartTime = aMediaStartTime;
  }

private:
  MediaSourceDecoder* mParentDecoder;
  nsRefPtr<MediaDecoderReader> mReader;
  int64_t mMediaDuration;
  int64_t mMediaStartTime;
};

} 

#endif 
