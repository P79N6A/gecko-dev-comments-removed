





#if !defined(MP4Reader_h_)
#define MP4Reader_h_

#include "MediaDecoderReader.h"
#include "nsAutoPtr.h"
#include "PlatformDecoderModule.h"
#include "mp4_demuxer/mp4_demuxer.h"
#include "demuxer/TrackDemuxer.h"
#include "MediaTaskQueue.h"

#include <deque>
#include "mozilla/Monitor.h"

namespace mozilla {

typedef std::deque<nsRefPtr<MediaRawData>> MediaSampleQueue;

class MP4Stream;

class MP4Reader final : public MediaDecoderReader
{
  typedef TrackInfo::TrackType TrackType;

public:
  explicit MP4Reader(AbstractMediaDecoder* aDecoder, MediaTaskQueue* aBorrowedTaskQueue = nullptr);

  virtual ~MP4Reader();

  virtual nsresult Init(MediaDecoderReader* aCloneDonor) override;

  virtual size_t SizeOfVideoQueueInFrames() override;
  virtual size_t SizeOfAudioQueueInFrames() override;

  virtual nsRefPtr<VideoDataPromise>
  RequestVideoData(bool aSkipToNextKeyframe, int64_t aTimeThreshold) override;

  virtual nsRefPtr<AudioDataPromise> RequestAudioData() override;

  virtual bool HasAudio() override;
  virtual bool HasVideo() override;

  virtual nsresult ReadMetadata(MediaInfo* aInfo,
                                MetadataTags** aTags) override;

  virtual void ReadUpdatedMetadata(MediaInfo* aInfo) override;

  virtual nsRefPtr<SeekPromise>
  Seek(int64_t aTime, int64_t aEndTime) override;

  virtual bool IsMediaSeekable() override;

  virtual int64_t GetEvictionOffset(double aTime) override;

protected:
  virtual void NotifyDataArrivedInternal(uint32_t aLength, int64_t aOffset) override;
public:

  virtual media::TimeIntervals GetBuffered() override;

  
  virtual void SetIdle() override;
  virtual void ReleaseMediaResources() override;
  virtual void SetSharedDecoderManager(SharedDecoderManager* aManager)
    override;

  virtual nsresult ResetDecode() override;

  virtual nsRefPtr<ShutdownPromise> Shutdown() override;

  virtual bool IsAsync() const override { return true; }

  virtual bool VideoIsHardwareAccelerated() const override;

  virtual void DisableHardwareAcceleration() override;

private:

  bool InitDemuxer();
  void ReturnOutput(MediaData* aData, TrackType aTrack);

  bool EnsureDecodersSetup();

  
  
  void Update(TrackType aTrack);

  
  
  void ScheduleUpdate(TrackType aTrack);

  void ExtractCryptoInitData(nsTArray<uint8_t>& aInitData);

  
  void InitLayersBackendType();

  
  
  already_AddRefed<MediaRawData> PopSample(TrackType aTrack);
  already_AddRefed<MediaRawData> PopSampleLocked(TrackType aTrack);

  bool SkipVideoDemuxToNextKeyFrame(int64_t aTimeThreshold, uint32_t& parsed);

  
  
  void Output(TrackType aType, MediaData* aSample);
  void InputExhausted(TrackType aTrack);
  void Error(TrackType aTrack);
  void Flush(TrackType aTrack);
  void DrainComplete(TrackType aTrack);
  void UpdateIndex();
  bool IsSupportedAudioMimeType(const nsACString& aMimeType);
  bool IsSupportedVideoMimeType(const nsACString& aMimeType);
  virtual bool IsWaitingOnCDMResource() override;

  Microseconds GetNextKeyframeTime();
  bool ShouldSkip(bool aSkipToNextKeyframe, int64_t aTimeThreshold);

  size_t SizeOfQueue(TrackType aTrack);

  nsRefPtr<MP4Stream> mStream;
  nsRefPtr<mp4_demuxer::MP4Demuxer> mDemuxer;
  nsRefPtr<PlatformDecoderModule> mPlatform;
  mp4_demuxer::CryptoFile mCrypto;

  class DecoderCallback : public MediaDataDecoderCallback {
  public:
    DecoderCallback(MP4Reader* aReader, TrackType aType)
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
    virtual void ReleaseMediaResources() override {
      mReader->ReleaseMediaResources();
    }
    virtual bool OnReaderTaskQueue() override {
      return mReader->OnTaskQueue();
    }
  private:
    MP4Reader* mReader;
    TrackType mType;
  };

  struct DecoderData {
    DecoderData(MediaData::Type aType,
                uint32_t aDecodeAhead)
      : mType(aType)
      , mMonitor(aType == MediaData::AUDIO_DATA ? "MP4 audio decoder data"
                                                : "MP4 video decoder data")
      , mNumSamplesInput(0)
      , mNumSamplesOutput(0)
      , mDecodeAhead(aDecodeAhead)
      , mActive(false)
      , mInputExhausted(false)
      , mError(false)
      , mIsFlushing(false)
      , mUpdateScheduled(false)
      , mDemuxEOS(false)
      , mDrainComplete(false)
      , mDiscontinuity(false)
    {
    }

    nsAutoPtr<TrackDemuxer> mTrackDemuxer;
    
    nsRefPtr<MediaDataDecoder> mDecoder;
    
    
    nsRefPtr<FlushableMediaTaskQueue> mTaskQueue;
    
    nsAutoPtr<DecoderCallback> mCallback;
    
    
    nsTArray<nsRefPtr<MediaData> > mOutput;
    
    MediaData::Type mType;

    
    virtual bool HasPromise() = 0;
    virtual void RejectPromise(MediaDecoderReader::NotDecodedReason aReason,
                               const char* aMethodName) = 0;

    
    
    Monitor mMonitor;
    uint64_t mNumSamplesInput;
    uint64_t mNumSamplesOutput;
    uint32_t mDecodeAhead;
    
    bool mActive;
    bool mInputExhausted;
    bool mError;
    bool mIsFlushing;
    bool mUpdateScheduled;
    bool mDemuxEOS;
    bool mDrainComplete;
    bool mDiscontinuity;
  };

  template<typename PromiseType>
  struct DecoderDataWithPromise : public DecoderData {
    DecoderDataWithPromise(MediaData::Type aType, uint32_t aDecodeAhead) :
      DecoderData(aType, aDecodeAhead)
    {
      mPromise.SetMonitor(&mMonitor);
    }

    MediaPromiseHolder<PromiseType> mPromise;

    bool HasPromise() override { return !mPromise.IsEmpty(); }
    void RejectPromise(MediaDecoderReader::NotDecodedReason aReason,
                       const char* aMethodName) override
    {
      mPromise.Reject(aReason, aMethodName);
    }
  };

  DecoderDataWithPromise<AudioDataPromise> mAudio;
  DecoderDataWithPromise<VideoDataPromise> mVideo;

  
  
  nsRefPtr<MediaRawData> mQueuedVideoSample;

  
  
  bool NeedInput(DecoderData& aDecoder);

  
  
  
  
  uint64_t mLastReportedNumDecodedFrames;

  DecoderData& GetDecoderData(TrackType aTrack);

  layers::LayersBackend mLayersBackendType;

  
  nsRefPtr<MediaRawData> DemuxVideoSample();
  nsRefPtr<MediaRawData> DemuxAudioSample();

  
  bool mDemuxerInitialized;

  
  bool mFoundSPSForTelemetry;

  
  bool mIsEncrypted;

  bool mIndexReady;
  int64_t mLastSeenEnd;
  Monitor mDemuxerMonitor;
  nsRefPtr<SharedDecoderManager> mSharedDecoderManager;
};

} 

#endif
