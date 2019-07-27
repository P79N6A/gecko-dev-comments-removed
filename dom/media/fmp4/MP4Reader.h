





#if !defined(MP4Reader_h_)
#define MP4Reader_h_

#include "MediaDecoderReader.h"
#include "nsAutoPtr.h"
#include "PlatformDecoderModule.h"
#include "mp4_demuxer/mp4_demuxer.h"
#include "MediaTaskQueue.h"

#include <deque>
#include "mozilla/Monitor.h"

namespace mozilla {

namespace dom {
class TimeRanges;
}

typedef std::deque<mp4_demuxer::MP4Sample*> MP4SampleQueue;

class MP4Stream;

class MP4Reader MOZ_FINAL : public MediaDecoderReader
{
  typedef mp4_demuxer::TrackType TrackType;

public:
  explicit MP4Reader(AbstractMediaDecoder* aDecoder);

  virtual ~MP4Reader();

  virtual nsresult Init(MediaDecoderReader* aCloneDonor) MOZ_OVERRIDE;

  virtual size_t SizeOfVideoQueueInFrames() MOZ_OVERRIDE;
  virtual size_t SizeOfAudioQueueInFrames() MOZ_OVERRIDE;

  virtual nsRefPtr<VideoDataPromise>
  RequestVideoData(bool aSkipToNextKeyframe, int64_t aTimeThreshold) MOZ_OVERRIDE;

  virtual nsRefPtr<AudioDataPromise> RequestAudioData() MOZ_OVERRIDE;

  virtual bool HasAudio() MOZ_OVERRIDE;
  virtual bool HasVideo() MOZ_OVERRIDE;

  
  
  
  
  virtual void PreReadMetadata() MOZ_OVERRIDE;
  virtual nsresult ReadMetadata(MediaInfo* aInfo,
                                MetadataTags** aTags) MOZ_OVERRIDE;

  virtual void ReadUpdatedMetadata(MediaInfo* aInfo) MOZ_OVERRIDE;

  virtual nsRefPtr<SeekPromise>
  Seek(int64_t aTime,
       int64_t aStartTime,
       int64_t aEndTime,
       int64_t aCurrentTime) MOZ_OVERRIDE;

  virtual bool IsMediaSeekable() MOZ_OVERRIDE;

  virtual int64_t GetEvictionOffset(double aTime) MOZ_OVERRIDE;

  virtual nsresult GetBuffered(dom::TimeRanges* aBuffered) MOZ_OVERRIDE;

  
  virtual void SetIdle() MOZ_OVERRIDE;
  virtual bool IsWaitingMediaResources() MOZ_OVERRIDE;
  virtual bool IsDormantNeeded() MOZ_OVERRIDE;
  virtual void ReleaseMediaResources() MOZ_OVERRIDE;
  virtual void SetSharedDecoderManager(SharedDecoderManager* aManager)
    MOZ_OVERRIDE;

  virtual nsresult ResetDecode() MOZ_OVERRIDE;

  virtual nsRefPtr<ShutdownPromise> Shutdown() MOZ_OVERRIDE;

private:

  void ReturnOutput(MediaData* aData, TrackType aTrack);

  
  
  void Update(TrackType aTrack);

  
  
  void ScheduleUpdate(TrackType aTrack);

  void ExtractCryptoInitData(nsTArray<uint8_t>& aInitData);

  
  void InitLayersBackendType();

  
  
  mp4_demuxer::MP4Sample* PopSample(mp4_demuxer::TrackType aTrack);

  bool SkipVideoDemuxToNextKeyFrame(int64_t aTimeThreshold, uint32_t& parsed);

  
  
  void Output(mp4_demuxer::TrackType aType, MediaData* aSample);
  void InputExhausted(mp4_demuxer::TrackType aTrack);
  void Error(mp4_demuxer::TrackType aTrack);
  void Flush(mp4_demuxer::TrackType aTrack);
  void DrainComplete(mp4_demuxer::TrackType aTrack);
  void UpdateIndex();
  bool IsSupportedAudioMimeType(const char* aMimeType);
  bool IsSupportedVideoMimeType(const char* aMimeType);
  void NotifyResourcesStatusChanged();
  void RequestCodecResource();
  bool IsWaitingOnCodecResource();
  virtual bool IsWaitingOnCDMResource() MOZ_OVERRIDE;

  size_t SizeOfQueue(TrackType aTrack);

  nsAutoPtr<mp4_demuxer::MP4Demuxer> mDemuxer;
  nsAutoPtr<PlatformDecoderModule> mPlatform;

  class DecoderCallback : public MediaDataDecoderCallback {
  public:
    DecoderCallback(MP4Reader* aReader,
                    mp4_demuxer::TrackType aType)
      : mReader(aReader)
      , mType(aType)
    {
    }
    virtual void Output(MediaData* aSample) MOZ_OVERRIDE {
      mReader->Output(mType, aSample);
    }
    virtual void InputExhausted() MOZ_OVERRIDE {
      mReader->InputExhausted(mType);
    }
    virtual void Error() MOZ_OVERRIDE {
      mReader->Error(mType);
    }
    virtual void DrainComplete() MOZ_OVERRIDE {
      mReader->DrainComplete(mType);
    }
    virtual void NotifyResourcesStatusChanged() MOZ_OVERRIDE {
      mReader->NotifyResourcesStatusChanged();
    }
    virtual void ReleaseMediaResources() MOZ_OVERRIDE {
      mReader->ReleaseMediaResources();
    }
  private:
    MP4Reader* mReader;
    mp4_demuxer::TrackType mType;
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

    
    nsRefPtr<MediaDataDecoder> mDecoder;
    
    
    nsRefPtr<MediaTaskQueue> mTaskQueue;
    
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

    bool HasPromise() MOZ_OVERRIDE { return !mPromise.IsEmpty(); }
    void RejectPromise(MediaDecoderReader::NotDecodedReason aReason,
                       const char* aMethodName) MOZ_OVERRIDE
    {
      mPromise.Reject(aReason, aMethodName);
    }
  };

  DecoderDataWithPromise<AudioDataPromise> mAudio;
  DecoderDataWithPromise<VideoDataPromise> mVideo;

  
  
  nsAutoPtr<mp4_demuxer::MP4Sample> mQueuedVideoSample;

  
  
  bool NeedInput(DecoderData& aDecoder);

  
  
  
  
  uint64_t mLastReportedNumDecodedFrames;

  DecoderData& GetDecoderData(mp4_demuxer::TrackType aTrack);

  layers::LayersBackend mLayersBackendType;

  nsTArray<nsTArray<uint8_t>> mInitDataEncountered;

  
  bool mDemuxerInitialized;

  
  bool mIsEncrypted;

  bool mIndexReady;
  Monitor mIndexMonitor;
  nsRefPtr<SharedDecoderManager> mSharedDecoderManager;
};

} 

#endif
