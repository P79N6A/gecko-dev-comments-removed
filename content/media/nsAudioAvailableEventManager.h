





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

  
  
  void Init(uint32_t aChannels, uint32_t aRate);

  
  
  void DispatchPendingEvents(uint64_t aCurrentTime);

  
  
  void QueueWrittenAudioData(AudioDataValue* aAudioData,
                             uint32_t aAudioDataLength,
                             uint64_t aEndTimeSampleOffset);

  
  
  void Clear();

  
  
  
  void Drain(uint64_t aTime);

  
  
  void SetSignalBufferLength(uint32_t aLength);

  
  
  
  void NotifyAudioAvailableListener();

private:
  
  
  
  nsBuiltinDecoder* mDecoder;

  
  float mSamplesPerSecond;

  
  nsAutoArrayPtr<float> mSignalBuffer;

  
  uint32_t mSignalBufferLength;

  
  uint32_t mNewSignalBufferLength;

  
  uint32_t mSignalBufferPosition;

  
  
  nsTArray< nsCOMPtr<nsIRunnable> > mPendingEvents;

  
  
  ReentrantMonitor mReentrantMonitor;

  
  
  
  bool mHasListener;
};

#endif
