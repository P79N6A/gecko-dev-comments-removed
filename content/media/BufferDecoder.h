





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
  virtual ~BufferDecoder();

  NS_DECL_THREADSAFE_ISUPPORTS

  
  void BeginDecoding(nsIThread* aDecodeThread);

  ReentrantMonitor& GetReentrantMonitor() MOZ_OVERRIDE;

  bool IsShutdown() const MOZ_FINAL MOZ_OVERRIDE;

  bool OnStateMachineThread() const MOZ_OVERRIDE;

  bool OnDecodeThread() const MOZ_OVERRIDE;

  MediaResource* GetResource() const MOZ_OVERRIDE;

  void NotifyBytesConsumed(int64_t aBytes, int64_t aOffset) MOZ_FINAL MOZ_OVERRIDE;

  void NotifyDecodedFrames(uint32_t aParsed, uint32_t aDecoded) MOZ_FINAL MOZ_OVERRIDE;

  int64_t GetEndMediaTime() const MOZ_FINAL MOZ_OVERRIDE;

  int64_t GetMediaDuration() MOZ_FINAL MOZ_OVERRIDE;

  void SetMediaDuration(int64_t aDuration) MOZ_OVERRIDE;

  void UpdateEstimatedMediaDuration(int64_t aDuration) MOZ_OVERRIDE;

  void SetMediaSeekable(bool aMediaSeekable) MOZ_OVERRIDE;

  void SetTransportSeekable(bool aTransportSeekable) MOZ_OVERRIDE;

  VideoFrameContainer* GetVideoFrameContainer() MOZ_FINAL MOZ_OVERRIDE;
  layers::ImageContainer* GetImageContainer() MOZ_OVERRIDE;

  bool IsTransportSeekable() MOZ_FINAL MOZ_OVERRIDE;

  bool IsMediaSeekable() MOZ_FINAL MOZ_OVERRIDE;

  void MetadataLoaded(int aChannels, int aRate, bool aHasAudio, bool aHasVideo, MetadataTags* aTags) MOZ_FINAL MOZ_OVERRIDE;
  void QueueMetadata(int64_t aTime, int aChannels, int aRate, bool aHasAudio, bool aHasVideo, MetadataTags* aTags) MOZ_FINAL MOZ_OVERRIDE;

  void SetMediaEndTime(int64_t aTime) MOZ_FINAL MOZ_OVERRIDE;

  void UpdatePlaybackPosition(int64_t aTime) MOZ_FINAL MOZ_OVERRIDE;

  void OnReadMetadataCompleted() MOZ_FINAL MOZ_OVERRIDE;

  MediaDecoderOwner* GetOwner() MOZ_FINAL MOZ_OVERRIDE;

protected:
  
  
  
  ReentrantMonitor mReentrantMonitor;
  nsCOMPtr<nsIThread> mDecodeThread;
  nsRefPtr<MediaResource> mResource;
};

} 

#endif 
