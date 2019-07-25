




































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

  virtual ~nsAudioStream();

  
  
  static void InitLibrary();

  
  
  static void ShutdownLibrary();

  
  
  nsIThread *GetThread();

  
  
  
  static nsAudioStream* AllocateStream();

  
  
  
  virtual nsresult Init(PRInt32 aNumChannels, PRInt32 aRate, SampleFormat aFormat) = 0;

  
  virtual nsresult Shutdown() = 0;

  
  
  
  
  
  
  virtual nsresult Write(const void* aBuf, PRUint32 aCount, PRBool aBlocking) = 0;

  
  
  virtual PRUint32 Available() = 0;

  
  
  virtual nsresult SetVolume(double aVolume) = 0;

  
  virtual nsresult Drain() = 0;

  
  virtual void Pause() = 0;

  
  virtual void Resume() = 0;

  
  
  virtual PRInt64 GetPosition() = 0;

  
  
  virtual PRInt64 GetSampleOffset() = 0;

  
  virtual PRBool IsPaused() = 0;

  
  
  virtual PRInt32 GetMinWriteSamples() = 0;

protected:
  nsCOMPtr<nsIThread> mAudioPlaybackThread;
};

#endif
