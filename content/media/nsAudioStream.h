




#if !defined(nsAudioStream_h_)
#define nsAudioStream_h_

#include "nscore.h"
#include "nsISupportsImpl.h"
#include "nsIThread.h"
#include "nsAutoPtr.h"
#include "AudioSampleFormat.h"





class nsAudioStream : public nsISupports
{
public:
  nsAudioStream()
    : mRate(0),
      mChannels(0)
  {}

  virtual ~nsAudioStream();

  
  
  static void InitLibrary();

  
  
  static void ShutdownLibrary();

  
  
  nsIThread *GetThread();

  
  
  
  static nsAudioStream* AllocateStream();

  
  
  
  
  
  virtual nsresult Init(int32_t aNumChannels, int32_t aRate) = 0;

  
  
  
  virtual void Shutdown() = 0;

  
  
  
  
  virtual nsresult Write(const void* aBuf, uint32_t aFrames) = 0;

  
  virtual uint32_t Available() = 0;

  
  
  virtual void SetVolume(double aVolume) = 0;

  
  
  
  virtual void Drain() = 0;

  
  virtual void Pause() = 0;

  
  virtual void Resume() = 0;

  
  
  virtual int64_t GetPosition() = 0;

  
  
  virtual int64_t GetPositionInFrames() = 0;

  
  virtual bool IsPaused() = 0;

  
  
  
  
  virtual int32_t GetMinWriteSize() = 0;

  int GetRate() { return mRate; }
  int GetChannels() { return mChannels; }

  static mozilla::AudioSampleFormat Format() {
#ifdef MOZ_SAMPLE_TYPE_S16
    return mozilla::AUDIO_FORMAT_S16;
#else
    return mozilla::AUDIO_FORMAT_FLOAT32;
#endif
  }

protected:
  nsCOMPtr<nsIThread> mAudioPlaybackThread;
  int mRate;
  int mChannels;
};

#endif
