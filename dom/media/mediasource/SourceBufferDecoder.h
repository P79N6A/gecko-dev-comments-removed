





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

class SourceBufferDecoder final : public AbstractMediaDecoder
{
public:
  
  
  SourceBufferDecoder(MediaResource* aResource, AbstractMediaDecoder* aParentDecoder,
                      int64_t aTimestampOffset );

  NS_DECL_THREADSAFE_ISUPPORTS

  virtual bool IsMediaSeekable() final override;
  virtual bool IsShutdown() const final override;
  virtual bool IsTransportSeekable() final override;
  virtual bool OnDecodeTaskQueue() const final override;
  virtual bool OnStateMachineTaskQueue() const final override;
  virtual int64_t GetMediaDuration() final override { MOZ_ASSERT_UNREACHABLE(""); return -1; };
  virtual layers::ImageContainer* GetImageContainer() final override;
  virtual MediaDecoderOwner* GetOwner() final override;
  virtual SourceBufferResource* GetResource() const final override;
  virtual ReentrantMonitor& GetReentrantMonitor() final override;
  virtual VideoFrameContainer* GetVideoFrameContainer() final override;
  virtual void MetadataLoaded(nsAutoPtr<MediaInfo> aInfo,
                              nsAutoPtr<MetadataTags> aTags,
                              MediaDecoderEventVisibility aEventVisibility) final override;
  virtual void FirstFrameLoaded(nsAutoPtr<MediaInfo> aInfo,
                                MediaDecoderEventVisibility aEventVisibility) final override;
  virtual void NotifyBytesConsumed(int64_t aBytes, int64_t aOffset) final override;
  virtual void NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset) final override;
  virtual void NotifyDecodedFrames(uint32_t aParsed, uint32_t aDecoded, uint32_t aDropped) final override;
  virtual void NotifyWaitingForResourcesStatusChanged() final override;
  virtual void OnReadMetadataCompleted() final override;
  virtual void QueueMetadata(int64_t aTime, nsAutoPtr<MediaInfo> aInfo, nsAutoPtr<MetadataTags> aTags) final override;
  virtual void RemoveMediaTracks() final override;
  virtual void SetMediaSeekable(bool aMediaSeekable) final override;
  virtual void UpdateEstimatedMediaDuration(int64_t aDuration) final override;
  virtual bool HasInitializationData() final override;

  
  int64_t GetTimestampOffset() const { return mTimestampOffset; }
  void SetTimestampOffset(int64_t aOffset)  { mTimestampOffset = aOffset; }

  
  
  media::TimeIntervals GetBuffered();

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
  virtual nsresult SetCDMProxy(CDMProxy* aProxy) override
  {
    MOZ_ASSERT(NS_IsMainThread());
    ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
    mCDMProxy = aProxy;
    return NS_OK;
  }

  virtual CDMProxy* GetCDMProxy() override
  {
    MOZ_ASSERT(OnDecodeTaskQueue() || NS_IsMainThread());
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
  
  int64_t mRealMediaDuration;
  
  double mTrimmedOffset;

#ifdef MOZ_EME
  nsRefPtr<CDMProxy> mCDMProxy;
#endif
};

} 

#endif 
