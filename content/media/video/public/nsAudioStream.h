




































#if !defined(nsAudioStream_h_)
#define nsAudioStream_h_

#include "nscore.h"
#include "prlog.h"
#include "nsTArray.h"

extern PRLogModuleInfo* gAudioStreamLog;

class nsAudioStream 
{
 public:
  enum SampleFormat
  {
    FORMAT_U8,
    FORMAT_S16_LE,
    FORMAT_FLOAT32_LE
  };

  
  
  static void InitLibrary();

  
  
  static void ShutdownLibrary();

  nsAudioStream();

  
  
  
  void Init(PRInt32 aNumChannels, PRInt32 aRate, SampleFormat aFormat);

  
  void Shutdown();

  
  
  
  void Write(const void* aBuf, PRUint32 aCount);

  
  
  PRInt32 Available();

  
  
  float GetVolume();

  
  
  void SetVolume(float aVolume);

  
  void Drain();

  
  void Pause();

  
  void Resume();

  
  
  double GetTime();

 private:
  double mVolume;
  void* mAudioHandle;
  int mRate;
  int mChannels;

  
  PRInt64 mSavedPauseBytes;
  PRInt64 mPauseBytes;

  float mStartTime;
  float mPauseTime;
  PRInt64 mSamplesBuffered;

  SampleFormat mFormat;

  
  
  
  
  nsTArray<short> mBufferOverflow;

  PRPackedBool mPaused;
};
#endif
