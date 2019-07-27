





#ifndef MOZILLA_MEDIASOURCEREADER_H_
#define MOZILLA_MEDIASOURCEREADER_H_

#include "mozilla/Attributes.h"
#include "mozilla/ReentrantMonitor.h"
#include "nsCOMPtr.h"
#include "nsError.h"
#include "nsString.h"
#include "nsTArray.h"
#include "MediaDecoderReader.h"

namespace mozilla {

class MediaSourceDecoder;
class SourceBufferDecoder;
class TrackBuffer;

namespace dom {

class MediaSource;

} 

class MediaSourceReader : public MediaDecoderReader
{
public:
  explicit MediaSourceReader(MediaSourceDecoder* aDecoder);

  nsresult Init(MediaDecoderReader* aCloneDonor) MOZ_OVERRIDE
  {
    
    
    
    return NS_OK;
  }

  
  
  void PrepareInitialization();

  bool IsWaitingMediaResources() MOZ_OVERRIDE;

  nsRefPtr<AudioDataPromise> RequestAudioData() MOZ_OVERRIDE;
  nsRefPtr<VideoDataPromise>
  RequestVideoData(bool aSkipToNextKeyframe, int64_t aTimeThreshold) MOZ_OVERRIDE;

  virtual size_t SizeOfVideoQueueInFrames() MOZ_OVERRIDE;
  virtual size_t SizeOfAudioQueueInFrames() MOZ_OVERRIDE;

  virtual bool IsDormantNeeded() MOZ_OVERRIDE;
  virtual void ReleaseMediaResources() MOZ_OVERRIDE;

  void OnAudioDecoded(AudioData* aSample);
  void OnAudioNotDecoded(NotDecodedReason aReason);
  void OnVideoDecoded(VideoData* aSample);
  void OnVideoNotDecoded(NotDecodedReason aReason);

  void DoVideoSeek();
  void DoAudioSeek();
  void OnVideoSeekCompleted(int64_t aTime);
  void OnVideoSeekFailed(nsresult aResult);
  void OnAudioSeekCompleted(int64_t aTime);
  void OnAudioSeekFailed(nsresult aResult);

  virtual bool IsWaitForDataSupported() MOZ_OVERRIDE { return true; }
  virtual nsRefPtr<WaitForDataPromise> WaitForData(MediaData::Type aType) MOZ_OVERRIDE;
  void MaybeNotifyHaveData();

  bool HasVideo() MOZ_OVERRIDE
  {
    return mInfo.HasVideo();
  }

  bool HasAudio() MOZ_OVERRIDE
  {
    return mInfo.HasAudio();
  }

  void NotifyTimeRangesChanged();

  
  
  
  virtual int64_t ComputeStartTime(const VideoData* aVideo, const AudioData* aAudio) MOZ_OVERRIDE { return 0; }

  
  
  
  
  bool UseBufferingHeuristics() MOZ_OVERRIDE { return false; }

  bool IsMediaSeekable() MOZ_OVERRIDE { return true; }

  nsresult ReadMetadata(MediaInfo* aInfo, MetadataTags** aTags) MOZ_OVERRIDE;
  void ReadUpdatedMetadata(MediaInfo* aInfo) MOZ_OVERRIDE;
  nsRefPtr<SeekPromise>
  Seek(int64_t aTime, int64_t aEndTime) MOZ_OVERRIDE;

  void CancelSeek() MOZ_OVERRIDE;

  
  nsresult GetBuffered(dom::TimeRanges* aBuffered) MOZ_OVERRIDE;

  already_AddRefed<SourceBufferDecoder> CreateSubDecoder(const nsACString& aType,
                                                         int64_t aTimestampOffset );

  void AddTrackBuffer(TrackBuffer* aTrackBuffer);
  void RemoveTrackBuffer(TrackBuffer* aTrackBuffer);
  void OnTrackBufferConfigured(TrackBuffer* aTrackBuffer, const MediaInfo& aInfo);

  nsRefPtr<ShutdownPromise> Shutdown() MOZ_OVERRIDE;

  virtual void BreakCycles() MOZ_OVERRIDE;

  bool IsShutdown()
  {
    ReentrantMonitorAutoEnter decoderMon(mDecoder->GetReentrantMonitor());
    return mDecoder->IsShutdown();
  }

  
  bool TrackBuffersContainTime(int64_t aTime);

  
  void Ended();

  
  bool IsEnded();
  bool IsNearEnd(int64_t aTime );

  
  void SetMediaSourceDuration(double aDuration );

#ifdef MOZ_EME
  nsresult SetCDMProxy(CDMProxy* aProxy);
#endif

  virtual bool IsAsync() const MOZ_OVERRIDE {
    return (!GetAudioReader() || GetAudioReader()->IsAsync()) &&
           (!GetVideoReader() || GetVideoReader()->IsAsync());
  }

  
  bool IsActiveReader(MediaDecoderReader* aReader);

  
  
  void GetMozDebugReaderData(nsAString& aString);

private:
  
  
  
  enum SwitchSourceResult {
    SOURCE_ERROR = -1,
    SOURCE_EXISTING = 0,
    SOURCE_NEW = 1,
  };

  SwitchSourceResult SwitchAudioSource(int64_t aTarget);
  SwitchSourceResult SwitchVideoSource(int64_t aTarget);

  void DoAudioRequest();
  void DoVideoRequest();

  void CompleteAudioSeekAndDoRequest()
  {
    mAudioSeekRequest.Complete();
    DoAudioRequest();
  }

  void CompleteVideoSeekAndDoRequest()
  {
    mVideoSeekRequest.Complete();
    DoVideoRequest();
  }

  void CompleteAudioSeekAndRejectPromise()
  {
    mAudioSeekRequest.Complete();
    mAudioPromise.Reject(DECODE_ERROR, __func__);
  }

  void CompleteVideoSeekAndRejectPromise()
  {
    mVideoSeekRequest.Complete();
    mVideoPromise.Reject(DECODE_ERROR, __func__);
  }

  MediaDecoderReader* GetAudioReader() const;
  MediaDecoderReader* GetVideoReader() const;
  int64_t GetReaderAudioTime(int64_t aTime) const;
  int64_t GetReaderVideoTime(int64_t aTime) const;

  
  
  void CheckForWaitOrEndOfStream(MediaData::Type aType, int64_t aTime );

  
  
  already_AddRefed<SourceBufferDecoder> SelectDecoder(int64_t aTarget,
                                                      int64_t aTolerance,
                                                      const nsTArray<nsRefPtr<SourceBufferDecoder>>& aTrackDecoders);
  bool HaveData(int64_t aTarget, MediaData::Type aType);

  void AttemptSeek();
  bool IsSeeking() { return mPendingSeekTime != -1; }

  nsRefPtr<SourceBufferDecoder> mAudioSourceDecoder;
  nsRefPtr<SourceBufferDecoder> mVideoSourceDecoder;

  nsTArray<nsRefPtr<TrackBuffer>> mTrackBuffers;
  nsTArray<nsRefPtr<TrackBuffer>> mShutdownTrackBuffers;
  nsTArray<nsRefPtr<TrackBuffer>> mEssentialTrackBuffers;
  nsRefPtr<TrackBuffer> mAudioTrack;
  nsRefPtr<TrackBuffer> mVideoTrack;

  MediaPromiseConsumerHolder<AudioDataPromise> mAudioRequest;
  MediaPromiseConsumerHolder<VideoDataPromise> mVideoRequest;

  MediaPromiseHolder<AudioDataPromise> mAudioPromise;
  MediaPromiseHolder<VideoDataPromise> mVideoPromise;

  MediaPromiseHolder<WaitForDataPromise> mAudioWaitPromise;
  MediaPromiseHolder<WaitForDataPromise> mVideoWaitPromise;
  MediaPromiseHolder<WaitForDataPromise>& WaitPromise(MediaData::Type aType)
  {
    return aType == MediaData::AUDIO_DATA ? mAudioWaitPromise : mVideoWaitPromise;
  }

#ifdef MOZ_EME
  nsRefPtr<CDMProxy> mCDMProxy;
#endif

  
  int64_t mLastAudioTime;
  int64_t mLastVideoTime;

  MediaPromiseConsumerHolder<SeekPromise> mAudioSeekRequest;
  MediaPromiseConsumerHolder<SeekPromise> mVideoSeekRequest;
  MediaPromiseHolder<SeekPromise> mSeekPromise;

  
  
  int64_t mPendingSeekTime;
  bool mWaitingForSeekData;

  int64_t mTimeThreshold;
  bool mDropAudioBeforeThreshold;
  bool mDropVideoBeforeThreshold;

  bool mEnded;
  double mMediaSourceDuration;

  bool mHasEssentialTrackBuffers;

  void ContinueShutdown();
  MediaPromiseHolder<ShutdownPromise> mMediaSourceShutdownPromise;
#ifdef MOZ_FMP4
  nsRefPtr<SharedDecoderManager> mSharedDecoderManager;
#endif
};

} 

#endif
