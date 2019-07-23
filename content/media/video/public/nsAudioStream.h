




































#include "nscore.h"
#include "prlog.h"

extern PRLogModuleInfo* gAudioStreamLog;

class nsAudioStream 
{
 public:
  
  
  static nsresult InitLibrary();

  
  
  static void ShutdownLibrary();

  nsAudioStream();

  
  
  
  nsresult Init(PRInt32 aNumChannels, PRInt32 aRate);

  
  nsresult Shutdown();

  
  nsresult Pause();

  
  nsresult Resume();

  
  
  
  nsresult Write(float* aBuf, PRUint32 count);

  
  
  PRInt32 Available();

  
  
  nsresult GetTime(double* aTime);

  
  
  nsresult GetVolume(float* aVolume);

  
  
  nsresult SetVolume(float aVolume);

 private:
  double mVolume;
#if defined(SYDNEY_AUDIO_NO_POSITION)
  
  double mPauseTime;
#else
  
  
  PRInt64 mPauseBytes;
#endif
  void* mAudioHandle;
  int mRate;
  int mChannels;
  PRBool mPaused;
};
