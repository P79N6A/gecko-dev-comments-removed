





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
  {
  }

  void SetReader(MediaDecoderReader* aReader)
  {
    MOZ_ASSERT(!mReader);
    mReader = aReader;
  }

  virtual ReentrantMonitor& GetReentrantMonitor() MOZ_OVERRIDE;
  virtual bool OnStateMachineThread() const MOZ_OVERRIDE;
  virtual bool OnDecodeThread() const MOZ_OVERRIDE;
  virtual SourceBufferResource* GetResource() const MOZ_OVERRIDE;
  virtual void SetMediaDuration(int64_t aDuration) MOZ_OVERRIDE;
  virtual void UpdateEstimatedMediaDuration(int64_t aDuration) MOZ_OVERRIDE;
  virtual void SetMediaSeekable(bool aMediaSeekable) MOZ_OVERRIDE;
  virtual void SetTransportSeekable(bool aTransportSeekable) MOZ_OVERRIDE;
  virtual layers::ImageContainer* GetImageContainer() MOZ_OVERRIDE;

  void NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset)
  {
    mReader->NotifyDataArrived(aBuffer, aLength, aOffset);

    
    mParentDecoder->NotifyDataArrived(aBuffer, aLength, aOffset);
  }

  nsresult GetBuffered(TimeRanges* aBuffered)
  {
    
    return mReader->GetBuffered(aBuffered, 0);
  }

private:
  MediaSourceDecoder* mParentDecoder;
  nsAutoPtr<MediaDecoderReader> mReader;
};

} 

#endif 
