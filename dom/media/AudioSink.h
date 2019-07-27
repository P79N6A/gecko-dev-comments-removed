




#if !defined(AudioSink_h__)
#define AudioSink_h__

#include "nsISupportsImpl.h"
#include "MediaDecoderReader.h"
#include "mozilla/dom/AudioChannelBinding.h"

namespace mozilla {

class AudioStream;
class MediaDecoderStateMachine;

class AudioSink {
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(AudioSink)

  AudioSink(MediaDecoderStateMachine* aStateMachine,
            int64_t aStartTime, AudioInfo aInfo, dom::AudioChannel aChannel);

  nsresult Init();

  int64_t GetPosition();

  
  
  bool HasUnplayedFrames();

  
  
  void PrepareToShutdown();

  
  
  void Shutdown();

  void SetVolume(double aVolume);
  void SetPlaybackRate(double aPlaybackRate);
  void SetPreservesPitch(bool aPreservesPitch);

  void StartPlayback();
  void StopPlayback();

private:
  ~AudioSink() {}

  
  
  
  void AudioLoop();

  
  nsresult InitializeAudioStream();

  void Drain();

  void Cleanup();

  bool ExpectMoreAudioData();

  
  void WaitForAudioToPlay();

  
  
  
  bool IsPlaybackContinuing();

  
  
  
  
  
  
  
  uint32_t PlaySilence(uint32_t aFrames);

  
  
  uint32_t PlayFromAudioQueue();

  void UpdateStreamSettings();

  
  
  void StartAudioStreamPlaybackIfNeeded();
  void WriteSilence(uint32_t aFrames);

  int64_t GetEndTime();

  MediaQueue<AudioData>& AudioQueue();

  ReentrantMonitor& GetReentrantMonitor();
  void AssertCurrentThreadInMonitor();
  void AssertOnAudioThread();

  nsRefPtr<MediaDecoderStateMachine> mStateMachine;

  
  
  nsCOMPtr<nsIThread> mThread;

  
  
  
  
  nsRefPtr<AudioStream> mAudioStream;

  
  
  
  
  int64_t mStartTime;

  
  int64_t mWritten;

  
  
  
  int64_t mLastGoodPosition;

  AudioInfo mInfo;

  dom::AudioChannel mChannel;

  double mVolume;
  double mPlaybackRate;
  bool mPreservesPitch;

  bool mStopAudioThread;

  bool mSetVolume;
  bool mSetPlaybackRate;
  bool mSetPreservesPitch;

  bool mPlaying;
};

} 

#endif
