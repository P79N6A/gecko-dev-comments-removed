






































#ifndef nsAudioAvailableEventManager_h__
#define nsAudioAvailableEventManager_h__

#include "nsCOMPtr.h"
#include "nsIRunnable.h"
#include "nsBuiltinDecoder.h"
#include "nsBuiltinDecoderReader.h"

using namespace mozilla;

class nsAudioAvailableEventManager
{
public:
  nsAudioAvailableEventManager(nsBuiltinDecoder* aDecoder);
  ~nsAudioAvailableEventManager();

  
  
  void Init(PRUint32 aChannels, PRUint32 aRate);

  
  
  void DispatchPendingEvents(PRUint64 aCurrentTime);

  
  
  void QueueWrittenAudioData(SoundDataValue* aAudioData,
                             PRUint32 aAudioDataLength,
                             PRUint64 aEndTimeSampleOffset);

  
  
  void Clear();

  
  
  
  void Drain(PRUint64 aTime);

  
  
  void SetSignalBufferLength(PRUint32 aLength);

private:
  
  
  
  nsBuiltinDecoder* mDecoder;

  
  float mSamplesPerSecond;

  
  nsAutoArrayPtr<float> mSignalBuffer;

  
  PRUint32 mSignalBufferLength;

  
  PRUint32 mNewSignalBufferLength;

  
  PRUint32 mSignalBufferPosition;

  
  
  nsTArray< nsCOMPtr<nsIRunnable> > mPendingEvents;

  
  
  ReentrantMonitor mReentrantMonitor;
};

#endif
