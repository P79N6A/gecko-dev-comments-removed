




































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
    FORMAT_FLOAT32
  };

  
  
  static void InitLibrary();

  
  
  static void ShutdownLibrary();

  nsAudioStream();
  ~nsAudioStream();

  
  
  
  void Init(PRInt32 aNumChannels, PRInt32 aRate, SampleFormat aFormat);

  
  void Shutdown();

  
  
  
  void Write(const void* aBuf, PRUint32 aCount);

  
  
  PRInt32 Available();

  
  
  void SetVolume(float aVolume);

  
  void Drain();

  
  void Pause();

  
  void Resume();

  
  
  float GetPosition();

 private:
  double mVolume;
  void* mAudioHandle;
  int mRate;
  int mChannels;

  SampleFormat mFormat;

  
  
  
  
  nsTArray<short> mBufferOverflow;
};
#endif
