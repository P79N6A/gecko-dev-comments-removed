





#ifndef MOZILLA_SUBBUFFERDECODER_H_
#define MOZILLA_SUBBUFFERDECODER_H_

#include "BufferDecoder.h"

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

  ReentrantMonitor& GetReentrantMonitor() MOZ_OVERRIDE;
  bool OnStateMachineThread() const MOZ_OVERRIDE;
  bool OnDecodeThread() const MOZ_OVERRIDE;
  void SetMediaDuration(int64_t aDuration) MOZ_OVERRIDE;
  void UpdateEstimatedMediaDuration(int64_t aDuration) MOZ_OVERRIDE;
  void SetMediaSeekable(bool aMediaSeekable) MOZ_OVERRIDE;
  void SetTransportSeekable(bool aTransportSeekable) MOZ_OVERRIDE;
  layers::ImageContainer* GetImageContainer() MOZ_OVERRIDE;

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
