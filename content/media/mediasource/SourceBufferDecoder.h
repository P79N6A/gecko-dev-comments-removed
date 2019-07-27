





#ifndef MOZILLA_SOURCEBUFFERDECODER_H_
#define MOZILLA_SOURCEBUFFERDECODER_H_

#include "AbstractMediaDecoder.h"
#include "MediaDecoderReader.h"
#include "SourceBufferResource.h"
#include "mozilla/Attributes.h"
#include "mozilla/ReentrantMonitor.h"

namespace mozilla {

class MediaResource;
class MediaDecoderReader;

namespace dom {

class TimeRanges;

} 

class SourceBufferDecoder : public AbstractMediaDecoder
{
public:
  
  
  SourceBufferDecoder(MediaResource* aResource, AbstractMediaDecoder* aParentDecoder);

  NS_DECL_THREADSAFE_ISUPPORTS

  virtual bool IsMediaSeekable() MOZ_FINAL MOZ_OVERRIDE;
  virtual bool IsShutdown() const MOZ_FINAL MOZ_OVERRIDE;
  virtual bool IsTransportSeekable() MOZ_FINAL MOZ_OVERRIDE;
  virtual bool OnDecodeThread() const MOZ_FINAL MOZ_OVERRIDE;
  virtual bool OnStateMachineThread() const MOZ_FINAL MOZ_OVERRIDE;
  virtual int64_t GetEndMediaTime() const MOZ_FINAL MOZ_OVERRIDE;
  virtual int64_t GetMediaDuration() MOZ_FINAL MOZ_OVERRIDE;
  virtual layers::ImageContainer* GetImageContainer() MOZ_FINAL MOZ_OVERRIDE;
  virtual MediaDecoderOwner* GetOwner() MOZ_FINAL MOZ_OVERRIDE;
  virtual SourceBufferResource* GetResource() const MOZ_FINAL MOZ_OVERRIDE;
  virtual ReentrantMonitor& GetReentrantMonitor() MOZ_FINAL MOZ_OVERRIDE;
  virtual VideoFrameContainer* GetVideoFrameContainer() MOZ_FINAL MOZ_OVERRIDE;
  virtual void MetadataLoaded(MediaInfo* aInfo, MetadataTags* aTags) MOZ_FINAL MOZ_OVERRIDE;
  virtual void NotifyBytesConsumed(int64_t aBytes, int64_t aOffset) MOZ_FINAL MOZ_OVERRIDE;
  virtual void NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset) MOZ_FINAL MOZ_OVERRIDE;
  virtual void NotifyDecodedFrames(uint32_t aParsed, uint32_t aDecoded) MOZ_FINAL MOZ_OVERRIDE;
  virtual void NotifyWaitingForResourcesStatusChanged() MOZ_FINAL MOZ_OVERRIDE;
  virtual void OnReadMetadataCompleted() MOZ_FINAL MOZ_OVERRIDE;
  virtual void QueueMetadata(int64_t aTime, MediaInfo* aInfo, MetadataTags* aTags) MOZ_FINAL MOZ_OVERRIDE;
  virtual void RemoveMediaTracks() MOZ_FINAL MOZ_OVERRIDE;
  virtual void SetMediaDuration(int64_t aDuration) MOZ_FINAL MOZ_OVERRIDE;
  virtual void SetMediaEndTime(int64_t aTime) MOZ_FINAL MOZ_OVERRIDE;
  virtual void SetMediaSeekable(bool aMediaSeekable) MOZ_FINAL MOZ_OVERRIDE;
  virtual void UpdateEstimatedMediaDuration(int64_t aDuration) MOZ_FINAL MOZ_OVERRIDE;
  virtual void UpdatePlaybackPosition(int64_t aTime) MOZ_FINAL MOZ_OVERRIDE;

  

  
  
  nsresult GetBuffered(dom::TimeRanges* aBuffered);

  void SetReader(MediaDecoderReader* aReader)
  {
    MOZ_ASSERT(!mReader);
    mReader = aReader;
  }

  MediaDecoderReader* GetReader()
  {
    return mReader;
  }

  void SetTaskQueue(MediaTaskQueue* aTaskQueue)
  {
    MOZ_ASSERT((!mTaskQueue && aTaskQueue) || (mTaskQueue && !aTaskQueue));
    mTaskQueue = aTaskQueue;
  }

  
  
  int64_t ConvertToByteOffset(double aTime);

  
  bool ContainsTime(double aTime);

private:
  virtual ~SourceBufferDecoder();

  
  RefPtr<MediaTaskQueue> mTaskQueue;

  nsRefPtr<MediaResource> mResource;

  AbstractMediaDecoder* mParentDecoder;
  nsRefPtr<MediaDecoderReader> mReader;
  int64_t mMediaDuration;
};

} 

#endif 
