





#if !defined(MediaFormatReader_h_)
#define MediaFormatReader_h_

#include "mozilla/Atomics.h"
#include "mozilla/Maybe.h"
#include "mozilla/Monitor.h"
#include "MediaDataDemuxer.h"
#include "MediaDecoderReader.h"
#include "MediaTaskQueue.h"
#include "PlatformDecoderModule.h"

namespace mozilla {

#if defined(MOZ_GONK_MEDIACODEC) || defined(XP_WIN) || defined(MOZ_APPLEMEDIA) || defined(MOZ_FFMPEG)
#define READER_DORMANT_HEURISTIC
#else
#undef READER_DORMANT_HEURISTIC
#endif

class MediaFormatReader final : public MediaDecoderReader
{
  typedef TrackInfo::TrackType TrackType;
  typedef media::Interval<int64_t> ByteInterval;

public:
  explicit MediaFormatReader(AbstractMediaDecoder* aDecoder,
                             MediaDataDemuxer* aDemuxer,
                             MediaTaskQueue* aBorrowedTaskQueue = nullptr);

  virtual ~MediaFormatReader();

  nsresult Init(MediaDecoderReader* aCloneDonor) override;

  size_t SizeOfVideoQueueInFrames() override;
  size_t SizeOfAudioQueueInFrames() override;

  nsRefPtr<VideoDataPromise>
  RequestVideoData(bool aSkipToNextKeyframe, int64_t aTimeThreshold) override;

  nsRefPtr<AudioDataPromise> RequestAudioData() override;

  bool HasVideo() override
  {
    return mInfo.HasVideo();
  }

  bool HasAudio() override
  {
    return mInfo.HasAudio();
  }

  nsRefPtr<MetadataPromise> AsyncReadMetadata() override;

  void ReadUpdatedMetadata(MediaInfo* aInfo) override;

  nsRefPtr<SeekPromise>
  Seek(int64_t aTime, int64_t aUnused) override;

  bool IsMediaSeekable() override
  {
    return mSeekable;
  }

  int64_t GetEvictionOffset(double aTime) override;
protected:
  void NotifyDataArrivedInternal(uint32_t aLength, int64_t aOffset) override;
public:
  void NotifyDataRemoved() override;

  media::TimeIntervals GetBuffered() override;

  virtual bool ForceZeroStartTime() const override;

  
  void SetIdle() override;
  bool IsDormantNeeded() override;
  void ReleaseMediaResources() override;
  void SetSharedDecoderManager(SharedDecoderManager* aManager)
    override;

  nsresult ResetDecode() override;

  nsRefPtr<ShutdownPromise> Shutdown() override;

  bool IsAsync() const override { return true; }

  bool VideoIsHardwareAccelerated() const override;

  void DisableHardwareAcceleration() override;

  bool IsWaitForDataSupported() override { return true; }
  nsRefPtr<WaitForDataPromise> WaitForData(MediaData::Type aType) override;

  bool IsWaitingOnCDMResource() override;

  bool UseBufferingHeuristics() override
  {
    return mTrackDemuxersMayBlock;
  }

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
  void NotifyNewOutput(TrackType aTrack, MediaData* aSample);
  void NotifyInputExhausted(TrackType aTrack);
  void NotifyDrainComplete(TrackType aTrack);
  void NotifyError(TrackType aTrack);
  void NotifyWaitingForData(TrackType aTrack);
  void NotifyEndOfStream(TrackType aTrack);

  void ExtractCryptoInitData(nsTArray<uint8_t>& aInitData);

  
  void InitLayersBackendType();

  
  
  void Output(TrackType aType, MediaData* aSample);
  void InputExhausted(TrackType aTrack);
  void Error(TrackType aTrack);
  void Flush(TrackType aTrack);
  void DrainComplete(TrackType aTrack);
  bool IsSupportedAudioMimeType(const nsACString& aMimeType);
  bool IsSupportedVideoMimeType(const nsACString& aMimeType);

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
    void Output(MediaData* aSample) override {
      mReader->Output(mType, aSample);
    }
    void InputExhausted() override {
      mReader->InputExhausted(mType);
    }
    void Error() override {
      mReader->Error(mType);
    }
    void DrainComplete() override {
      mReader->DrainComplete(mType);
    }
    void ReleaseMediaResources() override {
      mReader->ReleaseMediaResources();
    }
    bool OnReaderTaskQueue() override {
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
      , mDecodeAhead(aDecodeAhead)
      , mUpdateScheduled(false)
      , mDemuxEOS(false)
      , mDemuxEOSServiced(false)
      , mWaitingForData(false)
      , mReceivedNewData(false)
      , mDiscontinuity(true)
      , mOutputRequested(false)
      , mInputExhausted(false)
      , mError(false)
      , mDrainComplete(false)
      , mNumSamplesInput(0)
      , mNumSamplesOutput(0)
      , mSizeOfQueue(0)
      , mLastStreamSourceID(UINT32_MAX)
      , mMonitor(aType == MediaData::AUDIO_DATA ? "audio decoder data"
                                                : "video decoder data")
    {}

    MediaFormatReader* mOwner;
    
    MediaData::Type mType;
    nsRefPtr<MediaTrackDemuxer> mTrackDemuxer;
    
    nsRefPtr<MediaDataDecoder> mDecoder;
    
    
    nsRefPtr<FlushableMediaTaskQueue> mTaskQueue;
    
    nsAutoPtr<DecoderCallback> mCallback;

    
    uint32_t mDecodeAhead;
    bool mUpdateScheduled;
    bool mDemuxEOS;
    bool mDemuxEOSServiced;
    bool mWaitingForData;
    bool mReceivedNewData;
    bool mDiscontinuity;

    
    MediaPromiseRequestHolder<MediaTrackDemuxer::SeekPromise> mSeekRequest;

    
    nsTArray<nsRefPtr<MediaRawData>> mQueuedSamples;
    MediaPromiseRequestHolder<MediaTrackDemuxer::SamplesPromise> mDemuxRequest;
    MediaPromiseHolder<WaitForDataPromise> mWaitingPromise;
    bool HasWaitingPromise()
    {
      MOZ_ASSERT(mOwner->OnTaskQueue());
      return !mWaitingPromise.IsEmpty();
    }

    
    bool mOutputRequested;
    bool mInputExhausted;
    bool mError;
    bool mDrainComplete;
    
    
    Maybe<media::TimeUnit> mTimeThreshold;

    
    
    nsTArray<nsRefPtr<MediaData>> mOutput;
    uint64_t mNumSamplesInput;
    uint64_t mNumSamplesOutput;

    
    
    virtual bool HasPromise() = 0;
    virtual void RejectPromise(MediaDecoderReader::NotDecodedReason aReason,
                               const char* aMethodName) = 0;

    void ResetDemuxer()
    {
      
      mDemuxRequest.DisconnectIfExists();
      mTrackDemuxer->Reset();
    }

    void ResetState()
    {
      MOZ_ASSERT(mOwner->OnTaskQueue());
      mDemuxEOS = false;
      mDemuxEOSServiced = false;
      mWaitingForData = false;
      mReceivedNewData = false;
      mDiscontinuity = true;
      mQueuedSamples.Clear();
      mOutputRequested = false;
      mInputExhausted = false;
      mDrainComplete = false;
      mTimeThreshold.reset();
      mOutput.Clear();
      mNumSamplesInput = 0;
      mNumSamplesOutput = 0;
    }

    
    Atomic<size_t> mSizeOfQueue;
    
    uint32_t mLastStreamSourceID;
    
    
    Monitor mMonitor;
    media::TimeIntervals mTimeRanges;
    nsRefPtr<SharedTrackInfo> mInfo;
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
  MediaPromiseRequestHolder<MediaDataDemuxer::InitPromise> mDemuxerInitRequest;
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
  MediaPromiseRequestHolder<MediaTrackDemuxer::SkipAccessPointPromise> mSkipRequest;
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

  
  bool mTrackDemuxersMayBlock;

  
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
  MediaPromiseHolder<SeekPromise> mSeekPromise;

#ifdef MOZ_EME
  nsRefPtr<CDMProxy> mCDMProxy;
#endif

  nsRefPtr<SharedDecoderManager> mSharedDecoderManager;

  
  
  
  nsRefPtr<MediaDataDemuxer> mMainThreadDemuxer;
  nsRefPtr<MediaTrackDemuxer> mAudioTrackDemuxer;
  nsRefPtr<MediaTrackDemuxer> mVideoTrackDemuxer;
  ByteInterval mDataRange;
  media::TimeIntervals mCachedTimeRanges;
  bool mCachedTimeRangesStale;

#if defined(READER_DORMANT_HEURISTIC)
  const bool mDormantEnabled;
#endif
};

} 

#endif
