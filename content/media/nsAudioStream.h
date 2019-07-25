




































#if !defined(nsAudioStream_h_)
#define nsAudioStream_h_

#include "nscore.h"
#include "nsISupportsImpl.h"
#include "nsIThread.h"

class nsAudioStream : public nsISupports
{
public:

  enum SampleFormat
  {
    FORMAT_U8,
    FORMAT_S16_LE,
    FORMAT_FLOAT32
  };

  
  
  static void InitLibrary();

  
  
  static void ShutdownLibrary();

  
  
  static nsIThread *GetGlobalThread();

  
  
  
  
  static nsAudioStream* AllocateStream();

  
  
  
  virtual nsresult Init(PRInt32 aNumChannels, PRInt32 aRate, SampleFormat aFormat) = 0;

  
  virtual void Shutdown() = 0;

  
  
  
  
  
  
  virtual nsresult Write(const void* aBuf, PRUint32 aCount, PRBool aBlocking) = 0;

  
  
  virtual PRUint32 Available() = 0;

  
  
  virtual void SetVolume(float aVolume) = 0;

  
  virtual void Drain() = 0;

  
  virtual void Pause() = 0;

  
  virtual void Resume() = 0;

  
  
  virtual PRInt64 GetPosition() = 0;

  
  
  virtual PRInt64 GetSampleOffset() = 0;

  
  virtual PRBool IsPaused() = 0;
};

#endif
