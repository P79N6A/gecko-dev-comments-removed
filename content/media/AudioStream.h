




#if !defined(AudioStream_h_)
#define AudioStream_h_

#include "nscore.h"
#include "AudioSampleFormat.h"
#include "AudioChannelCommon.h"
#include "soundtouch/SoundTouch.h"
#include "nsAutoPtr.h"

namespace mozilla {

class AudioStream;

class AudioClock
{
  public:
    AudioClock(mozilla::AudioStream* aStream);
    
    
    void Init();
    
    
    void UpdateWritePosition(uint32_t aCount);
    
    
    uint64_t GetPosition();
    
    
    uint64_t GetPositionInFrames();
    
    
    void SetPlaybackRate(double aPlaybackRate);
    
    
    double GetPlaybackRate();
    
    
    void SetPreservesPitch(bool aPreservesPitch);
    
    
    bool GetPreservesPitch();
    
    int64_t GetWritten();
  private:
    
    
    AudioStream* mAudioStream;
    
    
    
    int mOldOutRate;
    
    int64_t mBasePosition;
    
    int64_t mBaseOffset;
    
    
    int64_t mOldBaseOffset;
    
    
    int64_t mOldBasePosition;
    
    int64_t mPlaybackRateChangeOffset;
    
    
    int64_t mPreviousPosition;
    
    int64_t mWritten;
    
    int mOutRate;
    
    int mInRate;
    
    bool mPreservesPitch;
    
    double mPlaybackRate;
    
    bool mCompensatingLatency;
};





class AudioStream
{
public:
  AudioStream();

  virtual ~AudioStream();

  
  
  static void InitLibrary();

  
  
  static void ShutdownLibrary();

  
  
  
  static AudioStream* AllocateStream();

  
  
  
  virtual nsresult Init(int32_t aNumChannels, int32_t aRate,
                        const dom::AudioChannelType aAudioStreamType) = 0;

  
  virtual void Shutdown() = 0;

  
  
  
  
  virtual nsresult Write(const mozilla::AudioDataValue* aBuf, uint32_t aFrames) = 0;

  
  virtual uint32_t Available() = 0;

  
  
  virtual void SetVolume(double aVolume) = 0;

  
  virtual void Drain() = 0;

  
  virtual void Start() = 0;

  
  
  virtual int64_t GetWritten();

  
  virtual void Pause() = 0;

  
  virtual void Resume() = 0;

  
  
  virtual int64_t GetPosition() = 0;

  
  
  virtual int64_t GetPositionInFrames() = 0;

  
  
  
  virtual int64_t GetPositionInFramesInternal() = 0;

  
  virtual bool IsPaused() = 0;

  
  
  virtual int32_t GetMinWriteSize() = 0;

  int GetRate() { return mOutRate; }
  int GetChannels() { return mChannels; }

  
  virtual nsresult EnsureTimeStretcherInitialized();
  
  
  virtual nsresult SetPlaybackRate(double aPlaybackRate);
  
  virtual nsresult SetPreservesPitch(bool aPreservesPitch);

protected:
  
  int mInRate;
  
  int mOutRate;
  int mChannels;
  
  int64_t mWritten;
  AudioClock mAudioClock;
  nsAutoPtr<soundtouch::SoundTouch> mTimeStretcher;
};

} 

#endif
