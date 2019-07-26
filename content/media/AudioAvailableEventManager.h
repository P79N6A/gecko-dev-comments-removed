





#ifndef AudioAvailableEventManager_h__
#define AudioAvailableEventManager_h__

#include "nsCOMPtr.h"
#include "nsIRunnable.h"
#include "MediaDecoder.h"
#include "MediaDecoderReader.h"

namespace mozilla {

class AudioAvailableEventManager
{
public:
  AudioAvailableEventManager(MediaDecoder* aDecoder);
  ~AudioAvailableEventManager();

  
  
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
  
  
  
  MediaDecoder* mDecoder;

  
  float mSamplesPerSecond;

  
  nsAutoArrayPtr<float> mSignalBuffer;

  
  uint32_t mSignalBufferLength;

  
  uint32_t mNewSignalBufferLength;

  
  uint32_t mSignalBufferPosition;

  
  
  nsTArray< nsCOMPtr<nsIRunnable> > mPendingEvents;

  
  
  ReentrantMonitor mReentrantMonitor;

  
  
  
  bool mHasListener;
};

} 

#endif
