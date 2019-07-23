




































#if !defined(nsAudioStream_h_)
#define nsAudioStream_h_

#include "nscore.h"
#include "prlog.h"

extern PRLogModuleInfo* gAudioStreamLog;

class nsAudioStream 
{
 public:
  
  
  static void InitLibrary();

  
  
  static void ShutdownLibrary();

  nsAudioStream();

  
  
  
  void Init(PRInt32 aNumChannels, PRInt32 aRate);

  
  void Shutdown();

  
  
  
  void Write(const float* aBuf, PRUint32 aCount);

  
  
  
  void Write(const short* aBuf, PRUint32 aCount);

  
  
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

  PRPackedBool mPaused;
};
#endif
