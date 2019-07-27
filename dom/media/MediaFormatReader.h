





#if !defined(MediaFormatReader_h_)
#define MediaFormatReader_h_

#include "mozilla/Maybe.h"
#include "mozilla/Monitor.h"
#include "MediaDataDemuxer.h"
#include "MediaDecoderReader.h"
#include "MediaTaskQueue.h"
#include "PlatformDecoderModule.h"

namespace mozilla {

namespace dom {
class TimeRanges;
}

#if defined(MOZ_GONK_MEDIACODEC) || defined(XP_WIN) || defined(MOZ_APPLEMEDIA) || defined(MOZ_FFMPEG)
#define READER_DORMANT_HEURISTIC
#else
#undef READER_DORMANT_HEURISTIC
#endif

class MediaFormatReader final : public MediaDecoderReader
{
  typedef TrackInfo::TrackType TrackType;

public:
  explicit MediaFormatReader(AbstractMediaDecoder* aDecoder,
                              MediaDataDemuxer* aDemuxer);

  virtual ~MediaFormatReader();

  virtual nsresult Init(MediaDecoderReader* aCloneDonor) override;

  virtual size_t SizeOfVideoQueueInFrames() override;
  virtual size_t SizeOfAudioQueueInFrames() override;

  virtual nsRefPtr<VideoDataPromise>
  RequestVideoData(bool aSkipToNextKeyframe, int64_t aTimeThreshold) override;

  virtual nsRefPtr<AudioDataPromise> RequestAudioData() override;

  bool HasVideo() override
  {
    return mInfo.HasVideo();
  }

  bool HasAudio() override
  {
    return mInfo.HasAudio();
  }

  virtual nsRefPtr<MetadataPromise> AsyncReadMetadata() override;
  virtual nsresult ReadMetadata(MediaInfo* aInfo,
                                MetadataTags** aTags) override
  {
    
    
    return NS_OK;
  }

  virtual void ReadUpdatedMetadata(MediaInfo* aInfo) override;

  virtual nsRefPtr<SeekPromise>
  Seek(int64_t aTime, int64_t aUnused) override;

  virtual bool IsMediaSeekable() override
  {
    return mSeekable;
  }

  virtual int64_t GetEvictionOffset(double aTime) override;
  virtual void NotifyDataArrived(const char* aBuffer,
                                 uint32_t aLength,
                                 int64_t aOffset) override;

  virtual nsresult GetBuffered(dom::TimeRanges* aBuffered) override;

  
  virtual void SetIdle() override;
  virtual bool IsDormantNeeded() override;
  virtual void ReleaseMediaResources() override;
  virtual void SetSharedDecoderManager(SharedDecoderManager* aManager)
    override;

  virtual nsresult ResetDecode() override;

  virtual nsRefPtr<ShutdownPromise> Shutdown() override;

  virtual bool IsAsync() const override { return true; }

  virtual bool VideoIsHardwareAccelerated() const override;

  virtual void DisableHardwareAcceleration() override;

  virtual bool IsWaitForDataSupported() override { return true; }
  virtual nsRefPtr<WaitForDataPromise> WaitForData(MediaData::Type aType) override;

  virtual bool IsWaitingOnCDMResource() override;

private:
  bool InitDemuxer();
  void NotifyDemuxer(uint32_t aLength, int64_t aOffset);
  void ReturnOutput(MediaData* aData, TrackType aTrack);

  bool EnsureDecodersSetup();

  
  
  void ScheduleUpdate(TrackType aTrack);
  void Update(TrackType aTrack);
  
  
  bool UpdateReceivedNewData(TrackType aTrack);
  
  void RequestDemuxSamples(TrackType aTrack);
  
  void DecodeDemuxedSamples(TrackType aTrack,
                            AbstractMediaDecoder::AutoNotifyDecoded& aA);

  void ExtractCryptoInitData(nsTArray<uint8_t>& aInitData);

  
  void InitLayersBackendType();

  
  
  void Output(TrackType aType, MediaData* aSample);
  void InputExhausted(TrackType aTrack);
  void Error(TrackType aTrack);
  void Flush(TrackType aTrack);
  void DrainComplete(TrackType aTrack);
  bool IsSupportedAudioMimeType(const nsACString& aMimeType);
  bool IsSupportedVideoMimeType(const nsACString& aMimeType);
  void NotifyResourcesStatusChanged();

  bool ShouldSkip(bool aSkipToNextKeyframe, media::TimeUnit aTimeThreshold);

  size_t SizeOfQueue(TrackType aTrack);

  nsRefPtr<MediaDataDemuxer> mDemuxer;
  nsRefPtr<PlatformDecoderModule> mPlatform;

  class DecoderCallback : public MediaDataDecoderCallback {
  public:
    DecoderCallback(MediaFormatReader* aReader, TrackType aType)
      : mReader(aReader)
      , mType(aType)
    {
    }
    virtual void Output(MediaData* aSample) override {
      mReader->Output(mType, aSample);
    }
    virtual void InputExhausted() override {
      mReader->InputExhausted(mType);
    }
    virtual void Error() override {
      mReader->Error(mType);
    }
    virtual void DrainComplete() override {
      mReader->DrainComplete(mType);
    }
    virtual void NotifyResourcesStatusChanged() override {
      mReader->NotifyResourcesStatusChanged();
    }
    virtual void ReleaseMediaResources() override {
      mReader->ReleaseMediaResources();
    }
    virtual bool OnReaderTaskQueue() override {
      return mReader->OnTaskQueue();
    }

  private:
    MediaFormatReader* mReader;
    TrackType mType;
  };

  struct DecoderData {
    DecoderData(MediaFormatReader* aOwner,
                MediaData::Type aType,
                uint32_t aDecodeAhead)
      : mOwner(aOwner)
      , mType(aType)
      , mDemuxEOS(false)
      , mDemuxEOSServiced(false)
      , mWaitingForData(false)
      , mReceivedNewData(false)
      , mMonitor(aType == MediaData::AUDIO_DATA ? "audio decoder data"
                                                : "video decoder data")
      , mNumSamplesInput(0)
      , mNumSamplesOutput(0)
      , mDecodeAhead(aDecodeAhead)
      , mInputExhausted(false)
      , mError(false)
      , mIsFlushing(false)
      , mUpdateScheduled(false)
      , mDrainComplete(false)
      , mDiscontinuity(false)
    {}

    MediaFormatReader* mOwner;
    
    MediaData::Type mType;
    nsRefPtr<MediaTrackDemuxer> mTrackDemuxer;
    
    nsRefPtr<MediaDataDecoder> mDecoder;
    
    
    nsRefPtr<FlushableMediaTaskQueue> mTaskQueue;
    
    nsAutoPtr<DecoderCallback> mCallback;

    
    bool mDemuxEOS;
    bool mDemuxEOSServiced;
    bool mWaitingForData;
    bool mReceivedNewData;
    
    nsTArray<nsRefPtr<MediaRawData>> mQueuedSamples;
    MediaPromiseConsumerHolder<MediaTrackDemuxer::SamplesPromise> mDemuxRequest;
    MediaPromiseHolder<WaitForDataPromise> mWaitingPromise;

    bool HasWaitingPromise()
    {
      MOZ_ASSERT(mOwner->OnTaskQueue());
      return !mWaitingPromise.IsEmpty();
    }

    
    
    virtual bool HasPromise() = 0;
    virtual void RejectPromise(MediaDecoderReader::NotDecodedReason aReason,
                               const char* aMethodName) = 0;

    void ResetState()
    {
      MOZ_ASSERT(mOwner->OnTaskQueue());
      mDemuxEOS = false;
      mDemuxEOSServiced = false;
      mWaitingForData = false;
      mReceivedNewData = false;
      mQueuedSamples.Clear();
      MonitorAutoLock mon(mMonitor);
      mNumSamplesInput = 0;
      mNumSamplesOutput = 0;
      mOutput.Clear();
      mInputExhausted = false;
      mIsFlushing = false;
      mUpdateScheduled = false;
      mDrainComplete = false;
      mDiscontinuity = true;
    }
    
    
    Monitor mMonitor;
    uint64_t mNumSamplesInput;
    uint64_t mNumSamplesOutput;
    uint32_t mDecodeAhead;
    
    
    nsTArray<nsRefPtr<MediaData>> mOutput;
    bool mInputExhausted;
    bool mError;
    bool mIsFlushing;
    bool mUpdateScheduled;
    bool mDrainComplete;
    bool mDiscontinuity;
    media::TimeIntervals mTimeRanges;
  };

  template<typename PromiseType>
  struct DecoderDataWithPromise : public DecoderData {
    DecoderDataWithPromise(MediaFormatReader* aOwner,
                           MediaData::Type aType,
                           uint32_t aDecodeAhead) :
      DecoderData(aOwner, aType, aDecodeAhead)
    {}

    MediaPromiseHolder<PromiseType> mPromise;

    bool HasPromise() override
    {
      MOZ_ASSERT(mOwner->OnTaskQueue());
      return !mPromise.IsEmpty();
    }

    void RejectPromise(MediaDecoderReader::NotDecodedReason aReason,
                       const char* aMethodName) override
    {
      MOZ_ASSERT(mOwner->OnTaskQueue());
      mPromise.Reject(aReason, aMethodName);
    }
  };

  DecoderDataWithPromise<AudioDataPromise> mAudio;
  DecoderDataWithPromise<VideoDataPromise> mVideo;

  
  
  bool NeedInput(DecoderData& aDecoder);

  DecoderData& GetDecoderData(TrackType aTrack);

  
  void OnDemuxerInitDone(nsresult);
  void OnDemuxerInitFailed(DemuxerFailureReason aFailure);
  MediaPromiseConsumerHolder<MediaDataDemuxer::InitPromise> mDemuxerInitRequest;
  void OnDemuxFailed(TrackType aTrack, DemuxerFailureReason aFailure);

  void DoDemuxVideo();
  void OnVideoDemuxCompleted(nsRefPtr<MediaTrackDemuxer::SamplesHolder> aSamples);
  void OnVideoDemuxFailed(DemuxerFailureReason aFailure)
  {
    OnDemuxFailed(TrackType::kVideoTrack, aFailure);
  }

  void DoDemuxAudio();
  void OnAudioDemuxCompleted(nsRefPtr<MediaTrackDemuxer::SamplesHolder> aSamples);
  void OnAudioDemuxFailed(DemuxerFailureReason aFailure)
  {
    OnDemuxFailed(TrackType::kAudioTrack, aFailure);
  }

  void SkipVideoDemuxToNextKeyFrame(media::TimeUnit aTimeThreshold);
  MediaPromiseConsumerHolder<MediaTrackDemuxer::SkipAccessPointPromise> mSkipRequest;
  void OnVideoSkipCompleted(uint32_t aSkipped);
  void OnVideoSkipFailed(MediaTrackDemuxer::SkipFailureHolder aFailure);

  
  
  
  
  uint64_t mLastReportedNumDecodedFrames;

  layers::LayersBackend mLayersBackendType;

  
  
  bool mInitDone;
  MediaPromiseHolder<MetadataPromise> mMetadataPromise;
  
  
  bool mSeekable;
  bool IsEncrypted()
  {
    return mIsEncrypted;
  }
  
  
  
  
  bool mIsEncrypted;

  
  bool IsSeeking() const { return mPendingSeekTime.isSome(); }
  void AttemptSeek();
  void OnSeekFailed(TrackType aTrack, DemuxerFailureReason aFailure);
  void DoVideoSeek();
  void OnVideoSeekCompleted(media::TimeUnit aTime);
  void OnVideoSeekFailed(DemuxerFailureReason aFailure)
  {
    OnSeekFailed(TrackType::kVideoTrack, aFailure);
  }

  void DoAudioSeek();
  void OnAudioSeekCompleted(media::TimeUnit aTime);
  void OnAudioSeekFailed(DemuxerFailureReason aFailure)
  {
    OnSeekFailed(TrackType::kAudioTrack, aFailure);
  }
  
  Maybe<media::TimeUnit> mPendingSeekTime;
  MediaPromiseConsumerHolder<MediaTrackDemuxer::SeekPromise> mVideoSeekRequest;
  MediaPromiseConsumerHolder<MediaTrackDemuxer::SeekPromise> mAudioSeekRequest;
  MediaPromiseHolder<SeekPromise> mSeekPromise;

#ifdef MOZ_EME
  nsRefPtr<CDMProxy> mCDMProxy;
#endif

  nsRefPtr<SharedDecoderManager> mSharedDecoderManager;

  
  nsRefPtr<MediaDataDemuxer> mMainThreadDemuxer;
  nsRefPtr<MediaTrackDemuxer> mAudioTrackDemuxer;
  nsRefPtr<MediaTrackDemuxer> mVideoTrackDemuxer;

#if defined(READER_DORMANT_HEURISTIC)
  const bool mDormantEnabled;
#endif
};

} 

#endif
