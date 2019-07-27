





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

namespace dom {

class MediaSource;

} 

class MediaSourceReader : public MediaDecoderReader
{
public:
  MediaSourceReader(MediaSourceDecoder* aDecoder, dom::MediaSource* aSource);

  nsresult Init(MediaDecoderReader* aCloneDonor) MOZ_OVERRIDE
  {
    
    
    
    return NS_OK;
  }

  bool IsWaitingMediaResources() MOZ_OVERRIDE;

  void RequestAudioData() MOZ_OVERRIDE;

  void OnAudioDecoded(AudioData* aSample);

  void OnAudioEOS();

  void RequestVideoData(bool aSkipToNextKeyframe, int64_t aTimeThreshold) MOZ_OVERRIDE;

  void OnVideoDecoded(VideoData* aSample);

  void OnVideoEOS();

  void OnDecodeError();

  bool HasVideo() MOZ_OVERRIDE
  {
    return mInfo.HasVideo();
  }

  bool HasAudio() MOZ_OVERRIDE
  {
    return mInfo.HasAudio();
  }

  bool IsMediaSeekable() { return true; }

  nsresult ReadMetadata(MediaInfo* aInfo, MetadataTags** aTags) MOZ_OVERRIDE;
  nsresult Seek(int64_t aTime, int64_t aStartTime, int64_t aEndTime,
                int64_t aCurrentTime) MOZ_OVERRIDE;
  already_AddRefed<SourceBufferDecoder> CreateSubDecoder(const nsACString& aType);

  void Shutdown();

  virtual void BreakCycles();

  void InitializePendingDecoders();

  bool IsShutdown()
  {
    ReentrantMonitorAutoEnter decoderMon(mDecoder->GetReentrantMonitor());
    return mDecoder->IsShutdown();
  }

  
  bool DecodersContainTime(double aTime);

private:
  enum SwitchType {
    SWITCH_OPTIONAL,
    SWITCH_FORCED
  };

  bool SwitchReaders(SwitchType aType);

  bool SwitchAudioReader(MediaDecoderReader* aTargetReader);
  bool SwitchVideoReader(MediaDecoderReader* aTargetReader);

  void SetMediaSourceDuration(double aDuration) ;

  
  int64_t mTimeThreshold;
  bool mDropAudioBeforeThreshold;
  bool mDropVideoBeforeThreshold;

  nsTArray<nsRefPtr<SourceBufferDecoder>> mPendingDecoders;
  nsTArray<nsRefPtr<SourceBufferDecoder>> mDecoders;

  nsRefPtr<MediaDecoderReader> mAudioReader;
  nsRefPtr<MediaDecoderReader> mVideoReader;

  dom::MediaSource* mMediaSource;
};

} 

#endif
