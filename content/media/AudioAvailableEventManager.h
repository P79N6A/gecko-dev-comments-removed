





#ifndef AudioAvailableEventManager_h__
#define AudioAvailableEventManager_h__

#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "AudioSampleFormat.h"
#include "mozilla/ReentrantMonitor.h"

class nsIRunnable;
template <class T> class nsCOMPtr;

namespace mozilla {

class MediaDecoder;

class AudioAvailableEventManager
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(AudioAvailableEventManager)

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
