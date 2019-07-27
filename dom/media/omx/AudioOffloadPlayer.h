


















#ifndef AUDIO_OFFLOAD_PLAYER_H_
#define AUDIO_OFFLOAD_PLAYER_H_

#include <stagefright/MediaBuffer.h>
#include <stagefright/MediaSource.h>
#include <stagefright/TimeSource.h>
#include <utils/threads.h>
#include <utils/RefBase.h>

#include "AudioOutput.h"
#include "AudioOffloadPlayerBase.h"
#include "MediaDecoderOwner.h"
#include "MediaOmxCommonDecoder.h"

namespace mozilla {

namespace dom {
class WakeLock;
}






















class AudioOffloadPlayer : public AudioOffloadPlayerBase
{
  typedef android::Mutex Mutex;
  typedef android::MetaData MetaData;
  typedef android::status_t status_t;
  typedef android::AudioTrack AudioTrack;
  typedef android::MediaBuffer MediaBuffer;
  typedef android::MediaSource MediaSource;

public:
  enum {
    REACHED_EOS,
    SEEK_COMPLETE
  };

  AudioOffloadPlayer(MediaOmxCommonDecoder* aDecoder = nullptr);

  ~AudioOffloadPlayer();

  
  virtual void SetSource(const android::sp<MediaSource> &aSource) override;

  
  
  virtual status_t Start(bool aSourceAlreadyStarted = false) override;

  virtual status_t ChangeState(MediaDecoder::PlayState aState) override;

  virtual void SetVolume(double aVolume) override;

  virtual double GetMediaTimeSecs() override;

  
  virtual void SetElementVisibility(bool aIsVisible) override;;

  
  
  
  virtual MediaDecoderOwner::NextFrameStatus GetNextFrameStatus() override;

  virtual nsRefPtr<MediaDecoder::SeekPromise> Seek(SeekTarget aTarget) override;

  void TimeUpdate();

  
  void Reset();

private:
  
  
  bool mStarted;

  
  
  bool mPlaying;

  
  
  
  
  bool mReachedEOS;

  
  
  bool mIsElementVisible;

  
  
  int mSessionId;

  
  int mSampleRate;

  
  
  
  
  
  
  
  
  int64_t mStartPosUs;

  
  
  
  SeekTarget mSeekTarget;

  
  
  
  MediaPromiseHolder<MediaDecoder::SeekPromise> mSeekPromise;

  
  
  
  int64_t mPositionTimeMediaUs;

  
  MediaDecoder::PlayState mPlayState;

  
  
  Mutex mLock;

  
  
  android::sp<MediaSource> mSource;

  
  
  
  android::sp<AudioSink> mAudioSink;

  
  MediaBuffer* mInputBuffer;

  
  MediaOmxCommonDecoder* mObserver;

  TimeStamp mLastFireUpdateTime;

  
  nsCOMPtr<nsITimer> mTimeUpdateTimer;

  
  
  
  nsCOMPtr<nsITimer> mResetTimer;

  
  
  nsRefPtr<mozilla::dom::WakeLock> mWakeLock;

  int64_t GetMediaTimeUs();

  
  
  int64_t GetOutputPlayPositionUs_l() const;

  
  
  
  size_t FillBuffer(void *aData, size_t aSize);

  
  static size_t AudioSinkCallback(AudioSink *aAudioSink,
                                  void *aData,
                                  size_t aSize,
                                  void *aMe,
                                  AudioSink::cb_event_t aEvent);

  bool IsSeeking();

  
  
  
  
  
  
  
  
  status_t DoSeek();

  
  
  status_t Play();

  
  
  void Pause(bool aPlayPendingSamples = false);

  
  
  
  
  
  
  nsresult StartTimeUpdate();
  nsresult StopTimeUpdate();

  void WakeLockCreate();
  void WakeLockRelease();

  
  
  void NotifyAudioEOS();

  
  
  void NotifyPositionChanged();

  
  
  void NotifyAudioTearDown();

  
  void SendMetaDataToHal(android::sp<AudioSink>& aSink,
                         const android::sp<MetaData>& aMeta);

  AudioOffloadPlayer(const AudioOffloadPlayer &);
  AudioOffloadPlayer &operator=(const AudioOffloadPlayer &);
};

} 

#endif
