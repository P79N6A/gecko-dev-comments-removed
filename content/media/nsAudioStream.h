




#if !defined(nsAudioStream_h_)
#define nsAudioStream_h_

#include "nscore.h"
#include "nsISupportsImpl.h"
#include "nsIThread.h"
#include "nsAutoPtr.h"

class nsAudioStream : public nsISupports
{
public:

  enum SampleFormat
  {
    FORMAT_U8,
    FORMAT_S16_LE,
    FORMAT_FLOAT32
  };

  nsAudioStream()
    : mRate(0),
      mChannels(0),
      mFormat(FORMAT_S16_LE)
  {}

  virtual ~nsAudioStream();

  
  
  static void InitLibrary();

  
  
  static void ShutdownLibrary();

  
  
  nsIThread *GetThread();

  
  
  
  static nsAudioStream* AllocateStream();

  
  
  
  
  virtual nsresult Init(PRInt32 aNumChannels, PRInt32 aRate, SampleFormat aFormat) = 0;

  
  
  virtual void Shutdown() = 0;

  
  
  
  
  virtual nsresult Write(const void* aBuf, PRUint32 aFrames) = 0;

  
  virtual PRUint32 Available() = 0;

  
  
  virtual void SetVolume(double aVolume) = 0;

  
  
  virtual void Drain() = 0;

  
  virtual void Pause() = 0;

  
  virtual void Resume() = 0;

  
  
  virtual PRInt64 GetPosition() = 0;

  
  
  virtual PRInt64 GetPositionInFrames() = 0;

  
  virtual bool IsPaused() = 0;

  
  
  
  virtual PRInt32 GetMinWriteSize() = 0;

  int GetRate() { return mRate; }
  int GetChannels() { return mChannels; }
  SampleFormat GetFormat() { return mFormat; }

protected:
  nsCOMPtr<nsIThread> mAudioPlaybackThread;
  int mRate;
  int mChannels;
  SampleFormat mFormat;
};

#endif
