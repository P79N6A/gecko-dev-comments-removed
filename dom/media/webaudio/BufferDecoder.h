





#ifndef BUFFER_DECODER_H_
#define BUFFER_DECODER_H_

#include "AbstractMediaDecoder.h"
#include "TaskQueue.h"
#include "mozilla/Attributes.h"
#include "mozilla/ReentrantMonitor.h"

namespace mozilla {





class BufferDecoder final : public AbstractMediaDecoder
{
public:
  
  
  explicit BufferDecoder(MediaResource* aResource);

  NS_DECL_THREADSAFE_ISUPPORTS

  
  void BeginDecoding(TaskQueue* aTaskQueueIdentity);

  virtual ReentrantMonitor& GetReentrantMonitor() final override;

  virtual bool IsShutdown() const final override;

  virtual bool OnStateMachineTaskQueue() const final override;

  virtual bool OnDecodeTaskQueue() const final override;

  virtual MediaResource* GetResource() const final override;

  virtual void NotifyBytesConsumed(int64_t aBytes, int64_t aOffset) final override;

  virtual void NotifyDecodedFrames(uint32_t aParsed, uint32_t aDecoded,
                                   uint32_t aDropped) final override;

  virtual void SetMediaSeekable(bool aMediaSeekable) final override;

  virtual VideoFrameContainer* GetVideoFrameContainer() final override;
  virtual layers::ImageContainer* GetImageContainer() final override;

  virtual bool IsTransportSeekable() final override;

  virtual bool IsMediaSeekable() final override;

  virtual void MetadataLoaded(nsAutoPtr<MediaInfo> aInfo,
                              nsAutoPtr<MetadataTags> aTags,
                              MediaDecoderEventVisibility aEventVisibility) final override;
  virtual void QueueMetadata(int64_t aTime, nsAutoPtr<MediaInfo> aInfo, nsAutoPtr<MetadataTags> aTags) final override;
  virtual void FirstFrameLoaded(nsAutoPtr<MediaInfo> aInfo,
                                MediaDecoderEventVisibility aEventVisibility) final override;

  virtual void RemoveMediaTracks() final override;

  virtual void OnReadMetadataCompleted() final override;

  virtual MediaDecoderOwner* GetOwner() final override;

  virtual void NotifyWaitingForResourcesStatusChanged() final override;

  virtual void NotifyDataArrived(uint32_t, int64_t, bool) final override {};

private:
  virtual ~BufferDecoder();

  
  
  
  ReentrantMonitor mReentrantMonitor;
  nsRefPtr<TaskQueue> mTaskQueueIdentity;
  nsRefPtr<MediaResource> mResource;
};

} 

#endif 
