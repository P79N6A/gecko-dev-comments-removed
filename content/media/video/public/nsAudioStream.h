




































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

  
  
  
  void Write(float* aBuf, PRUint32 count);

  
  
  PRInt32 Available();

  
  
  float GetVolume();

  
  
  void SetVolume(float aVolume);

 private:
  double mVolume;
  void* mAudioHandle;
  int mRate;
  int mChannels;
  PRPackedBool mMute;
};
