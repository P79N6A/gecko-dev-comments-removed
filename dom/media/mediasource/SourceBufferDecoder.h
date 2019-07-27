





#ifndef MOZILLA_SOURCEBUFFERDECODER_H_
#define MOZILLA_SOURCEBUFFERDECODER_H_

#include "AbstractMediaDecoder.h"
#include "MediaDecoderReader.h"
#include "SourceBufferResource.h"
#include "mozilla/Attributes.h"
#ifdef MOZ_EME
#include "mozilla/CDMProxy.h"
#endif
#include "mozilla/ReentrantMonitor.h"

namespace mozilla {

class MediaResource;
class MediaDecoderReader;

namespace dom {

class TimeRanges;

} 

class SourceBufferDecoder MOZ_FINAL : public AbstractMediaDecoder
{
public:
  
  
  SourceBufferDecoder(MediaResource* aResource, AbstractMediaDecoder* aParentDecoder,
                      int64_t aTimestampOffset );

  NS_DECL_THREADSAFE_ISUPPORTS

  virtual bool IsMediaSeekable() MOZ_FINAL MOZ_OVERRIDE;
  virtual bool IsShutdown() const MOZ_FINAL MOZ_OVERRIDE;
  virtual bool IsTransportSeekable() MOZ_FINAL MOZ_OVERRIDE;
  virtual bool OnDecodeThread() const MOZ_FINAL MOZ_OVERRIDE;
  virtual bool OnStateMachineThread() const MOZ_FINAL MOZ_OVERRIDE;
  virtual int64_t GetMediaDuration() MOZ_FINAL MOZ_OVERRIDE;
  virtual layers::ImageContainer* GetImageContainer() MOZ_FINAL MOZ_OVERRIDE;
  virtual MediaDecoderOwner* GetOwner() MOZ_FINAL MOZ_OVERRIDE;
  virtual SourceBufferResource* GetResource() const MOZ_FINAL MOZ_OVERRIDE;
  virtual ReentrantMonitor& GetReentrantMonitor() MOZ_FINAL MOZ_OVERRIDE;
  virtual VideoFrameContainer* GetVideoFrameContainer() MOZ_FINAL MOZ_OVERRIDE;
  virtual void MetadataLoaded(nsAutoPtr<MediaInfo> aInfo,
                              nsAutoPtr<MetadataTags> aTags,
                              MediaDecoderEventVisibility aEventVisibility) MOZ_FINAL MOZ_OVERRIDE;
  virtual void FirstFrameLoaded(nsAutoPtr<MediaInfo> aInfo,
                                MediaDecoderEventVisibility aEventVisibility) MOZ_FINAL MOZ_OVERRIDE;
  virtual void NotifyBytesConsumed(int64_t aBytes, int64_t aOffset) MOZ_FINAL MOZ_OVERRIDE;
  virtual void NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset) MOZ_FINAL MOZ_OVERRIDE;
  virtual void NotifyDecodedFrames(uint32_t aParsed, uint32_t aDecoded, uint32_t aDropped) MOZ_FINAL MOZ_OVERRIDE;
  virtual void NotifyWaitingForResourcesStatusChanged() MOZ_FINAL MOZ_OVERRIDE;
  virtual void OnReadMetadataCompleted() MOZ_FINAL MOZ_OVERRIDE;
  virtual void QueueMetadata(int64_t aTime, nsAutoPtr<MediaInfo> aInfo, nsAutoPtr<MetadataTags> aTags) MOZ_FINAL MOZ_OVERRIDE;
  virtual void RemoveMediaTracks() MOZ_FINAL MOZ_OVERRIDE;
  virtual void SetMediaDuration(int64_t aDuration) MOZ_FINAL MOZ_OVERRIDE;
  virtual void SetMediaEndTime(int64_t aTime) MOZ_FINAL MOZ_OVERRIDE;
  virtual void SetMediaSeekable(bool aMediaSeekable) MOZ_FINAL MOZ_OVERRIDE;
  virtual void UpdateEstimatedMediaDuration(int64_t aDuration) MOZ_FINAL MOZ_OVERRIDE;
  virtual void UpdatePlaybackPosition(int64_t aTime) MOZ_FINAL MOZ_OVERRIDE;
  virtual bool HasInitializationData() MOZ_FINAL MOZ_OVERRIDE;

  
  int64_t GetTimestampOffset() const { return mTimestampOffset; }
  void SetTimestampOffset(int64_t aOffset)  { mTimestampOffset = aOffset; }

  
  
  nsresult GetBuffered(dom::TimeRanges* aBuffered);

  void SetReader(MediaDecoderReader* aReader)
  {
    MOZ_ASSERT(!mReader);
    mReader = aReader;
  }

  MediaDecoderReader* GetReader() const
  {
    return mReader;
  }

  void SetTaskQueue(MediaTaskQueue* aTaskQueue)
  {
    MOZ_ASSERT((!mTaskQueue && aTaskQueue) || (mTaskQueue && !aTaskQueue));
    mTaskQueue = aTaskQueue;
  }

  void BreakCycles()
  {
    if (mReader) {
      mReader->BreakCycles();
      mReader = nullptr;
    }
    mTaskQueue = nullptr;
#ifdef MOZ_EME
    mCDMProxy = nullptr;
#endif
  }

#ifdef MOZ_EME
  virtual nsresult SetCDMProxy(CDMProxy* aProxy) MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());
    ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
    mCDMProxy = aProxy;
    return NS_OK;
  }

  virtual CDMProxy* GetCDMProxy() MOZ_OVERRIDE
  {
    MOZ_ASSERT(OnDecodeThread() || NS_IsMainThread());
    ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
    return mCDMProxy;
  }
#endif

  
  
  int64_t ConvertToByteOffset(double aTime);

  

  
  
  
  
  
  void Trim(int64_t aDuration);
  bool WasTrimmed()
  {
    return mTrimmedOffset >= 0;
  }

  
  void SetRealMediaDuration(int64_t aDuration);
  int64_t GetRealMediaDuration()
  {
    return mRealMediaDuration;
  }

private:
  virtual ~SourceBufferDecoder();

  
  RefPtr<MediaTaskQueue> mTaskQueue;

  nsRefPtr<MediaResource> mResource;

  AbstractMediaDecoder* mParentDecoder;
  nsRefPtr<MediaDecoderReader> mReader;
  
  int64_t mTimestampOffset;
  
  int64_t mMediaDuration;
  
  int64_t mRealMediaDuration;
  
  double mTrimmedOffset;

#ifdef MOZ_EME
  nsRefPtr<CDMProxy> mCDMProxy;
#endif
};

} 

#endif 
