





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

class MP4Reader : public MediaDecoderReader
{
public:
  explicit MP4Reader(AbstractMediaDecoder* aDecoder);

  virtual ~MP4Reader();

  virtual nsresult Init(MediaDecoderReader* aCloneDonor) MOZ_OVERRIDE;

  virtual bool DecodeAudioData() MOZ_OVERRIDE;
  virtual bool DecodeVideoFrame(bool &aKeyframeSkip,
                                int64_t aTimeThreshold) MOZ_OVERRIDE;

  virtual bool HasAudio() MOZ_OVERRIDE;
  virtual bool HasVideo() MOZ_OVERRIDE;

  virtual nsresult ReadMetadata(MediaInfo* aInfo,
                                MetadataTags** aTags) MOZ_OVERRIDE;

  virtual nsresult Seek(int64_t aTime,
                        int64_t aStartTime,
                        int64_t aEndTime,
                        int64_t aCurrentTime) MOZ_OVERRIDE;

  virtual bool IsMediaSeekable() MOZ_OVERRIDE;

  virtual void NotifyDataArrived(const char* aBuffer, uint32_t aLength,
                                 int64_t aOffset) MOZ_OVERRIDE;

  virtual int64_t GetEvictionOffset(double aTime) MOZ_OVERRIDE;

  virtual nsresult GetBuffered(dom::TimeRanges* aBuffered,
                               int64_t aStartTime) MOZ_OVERRIDE;

  
  virtual void SetIdle() MOZ_OVERRIDE;
  virtual bool IsWaitingMediaResources() MOZ_OVERRIDE;
  virtual bool IsDormantNeeded() MOZ_OVERRIDE;
  virtual void ReleaseMediaResources() MOZ_OVERRIDE;
  virtual void SetSharedDecoderManager(SharedDecoderManager* aManager)
    MOZ_OVERRIDE;

  virtual nsresult ResetDecode() MOZ_OVERRIDE;

  virtual void Shutdown() MOZ_OVERRIDE;

private:

  void ExtractCryptoInitData(nsTArray<uint8_t>& aInitData);

  
  void InitLayersBackendType();

  
  
  mp4_demuxer::MP4Sample* PopSample(mp4_demuxer::TrackType aTrack);

  bool SkipVideoDemuxToNextKeyFrame(int64_t aTimeThreshold, uint32_t& parsed);

  void Output(mp4_demuxer::TrackType aType, MediaData* aSample);
  void InputExhausted(mp4_demuxer::TrackType aTrack);
  void Error(mp4_demuxer::TrackType aTrack);
  bool Decode(mp4_demuxer::TrackType aTrack);
  void Flush(mp4_demuxer::TrackType aTrack);
  void DrainComplete(mp4_demuxer::TrackType aTrack);
  void UpdateIndex();
  bool IsSupportedAudioMimeType(const char* aMimeType);
  void NotifyResourcesStatusChanged();
  bool IsWaitingOnCodecResource();
  bool IsWaitingOnCDMResource();

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
    DecoderData(const char* aMonitorName,
                uint32_t aDecodeAhead)
      : mMonitor(aMonitorName)
      , mNumSamplesInput(0)
      , mNumSamplesOutput(0)
      , mDecodeAhead(aDecodeAhead)
      , mActive(false)
      , mInputExhausted(false)
      , mError(false)
      , mIsFlushing(false)
      , mDrainComplete(false)
      , mEOS(false)
    {
    }

    
    nsRefPtr<MediaDataDecoder> mDecoder;
    
    
    nsRefPtr<MediaTaskQueue> mTaskQueue;
    
    nsAutoPtr<DecoderCallback> mCallback;
    
    
    Monitor mMonitor;
    uint64_t mNumSamplesInput;
    uint64_t mNumSamplesOutput;
    uint32_t mDecodeAhead;
    
    bool mActive;
    bool mInputExhausted;
    bool mError;
    bool mIsFlushing;
    bool mDrainComplete;
    bool mEOS;
  };
  DecoderData mAudio;
  DecoderData mVideo;
  
  
  nsAutoPtr<mp4_demuxer::MP4Sample> mQueuedVideoSample;

  
  
  
  
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
