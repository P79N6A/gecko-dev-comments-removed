





#ifndef BUFFER_DECODER_H_
#define BUFFER_DECODER_H_

#include "AbstractMediaDecoder.h"
#include "mozilla/Attributes.h"
#include "mozilla/ReentrantMonitor.h"

namespace mozilla {





class BufferDecoder : public AbstractMediaDecoder
{
public:
  
  
  explicit BufferDecoder(MediaResource* aResource);

  NS_DECL_THREADSAFE_ISUPPORTS

  
  void BeginDecoding(nsIThread* aDecodeThread);

  virtual ReentrantMonitor& GetReentrantMonitor() MOZ_FINAL MOZ_OVERRIDE;

  virtual bool IsShutdown() const MOZ_FINAL MOZ_OVERRIDE;

  virtual bool OnStateMachineThread() const MOZ_FINAL MOZ_OVERRIDE;

  virtual bool OnDecodeThread() const MOZ_FINAL MOZ_OVERRIDE;

  virtual MediaResource* GetResource() const MOZ_FINAL MOZ_OVERRIDE;

  virtual void NotifyBytesConsumed(int64_t aBytes, int64_t aOffset) MOZ_FINAL MOZ_OVERRIDE;

  virtual void NotifyDecodedFrames(uint32_t aParsed, uint32_t aDecoded) MOZ_FINAL MOZ_OVERRIDE;

  virtual int64_t GetMediaDuration() MOZ_FINAL MOZ_OVERRIDE;

  virtual void SetMediaDuration(int64_t aDuration) MOZ_FINAL MOZ_OVERRIDE;

  virtual void UpdateEstimatedMediaDuration(int64_t aDuration) MOZ_FINAL MOZ_OVERRIDE;

  virtual void SetMediaSeekable(bool aMediaSeekable) MOZ_FINAL MOZ_OVERRIDE;

  virtual VideoFrameContainer* GetVideoFrameContainer() MOZ_FINAL MOZ_OVERRIDE;
  virtual layers::ImageContainer* GetImageContainer() MOZ_FINAL MOZ_OVERRIDE;

  virtual bool IsTransportSeekable() MOZ_FINAL MOZ_OVERRIDE;

  virtual bool IsMediaSeekable() MOZ_FINAL MOZ_OVERRIDE;

  virtual void MetadataLoaded(nsAutoPtr<MediaInfo> aInfo, nsAutoPtr<MetadataTags> aTags) MOZ_FINAL MOZ_OVERRIDE;
  virtual void QueueMetadata(int64_t aTime, nsAutoPtr<MediaInfo> aInfo, nsAutoPtr<MetadataTags> aTags) MOZ_FINAL MOZ_OVERRIDE;
  virtual void FirstFrameLoaded(nsAutoPtr<MediaInfo> aInfo) MOZ_FINAL MOZ_OVERRIDE;

  virtual void RemoveMediaTracks() MOZ_FINAL MOZ_OVERRIDE;

  virtual void SetMediaEndTime(int64_t aTime) MOZ_FINAL MOZ_OVERRIDE;

  virtual void UpdatePlaybackPosition(int64_t aTime) MOZ_FINAL MOZ_OVERRIDE;

  virtual void OnReadMetadataCompleted() MOZ_FINAL MOZ_OVERRIDE;

  virtual MediaDecoderOwner* GetOwner() MOZ_FINAL MOZ_OVERRIDE;

  virtual void NotifyWaitingForResourcesStatusChanged() MOZ_FINAL MOZ_OVERRIDE;

  virtual void NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset) MOZ_FINAL MOZ_OVERRIDE;

private:
  virtual ~BufferDecoder();

  
  
  
  ReentrantMonitor mReentrantMonitor;
  nsCOMPtr<nsIThread> mDecodeThread;
  nsRefPtr<MediaResource> mResource;
};

} 

#endif 
