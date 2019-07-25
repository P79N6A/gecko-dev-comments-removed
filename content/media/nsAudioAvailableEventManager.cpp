





#include "nsTArray.h"
#include "nsAudioAvailableEventManager.h"
#include "VideoUtils.h"

static const nsTArray< nsCOMPtr<nsIRunnable> >::size_type MAX_PENDING_EVENTS = 100;

using namespace mozilla;

class nsAudioAvailableEventRunner : public nsRunnable
{
private:
  nsCOMPtr<nsBuiltinDecoder> mDecoder;
  nsAutoArrayPtr<float> mFrameBuffer;
public:
  nsAudioAvailableEventRunner(nsBuiltinDecoder* aDecoder, float* aFrameBuffer,
                              uint32_t aFrameBufferLength, float aTime) :
    mDecoder(aDecoder),
    mFrameBuffer(aFrameBuffer),
    mFrameBufferLength(aFrameBufferLength),
    mTime(aTime)
  {
    MOZ_COUNT_CTOR(nsAudioAvailableEventRunner);
  }

  ~nsAudioAvailableEventRunner() {
    MOZ_COUNT_DTOR(nsAudioAvailableEventRunner);
  }

  NS_IMETHOD Run()
  {
    mDecoder->AudioAvailable(mFrameBuffer.forget(), mFrameBufferLength, mTime);
    return NS_OK;
  }

  const uint32_t mFrameBufferLength;

  
  const float mTime;
};


nsAudioAvailableEventManager::nsAudioAvailableEventManager(nsBuiltinDecoder* aDecoder) :
  mDecoder(aDecoder),
  mSignalBuffer(new float[mDecoder->GetFrameBufferLength()]),
  mSignalBufferLength(mDecoder->GetFrameBufferLength()),
  mNewSignalBufferLength(mSignalBufferLength),
  mSignalBufferPosition(0),
  mReentrantMonitor("media.audioavailableeventmanager"),
  mHasListener(false)
{
  MOZ_COUNT_CTOR(nsAudioAvailableEventManager);
}

nsAudioAvailableEventManager::~nsAudioAvailableEventManager()
{
  MOZ_COUNT_DTOR(nsAudioAvailableEventManager);
}

void nsAudioAvailableEventManager::Init(uint32_t aChannels, uint32_t aRate)
{
  NS_ASSERTION(aChannels != 0 && aRate != 0, "Audio metadata not known.");
  mSamplesPerSecond = static_cast<float>(aChannels * aRate);
}

void nsAudioAvailableEventManager::DispatchPendingEvents(uint64_t aCurrentTime)
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  if (!mHasListener) {
    return;
  }

  while (mPendingEvents.Length() > 0) {
    nsAudioAvailableEventRunner* e =
      (nsAudioAvailableEventRunner*)mPendingEvents[0].get();
    if (e->mTime * USECS_PER_S > aCurrentTime) {
      break;
    }
    nsCOMPtr<nsIRunnable> event = mPendingEvents[0];
    mPendingEvents.RemoveElementAt(0);
    NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
  }
}

void nsAudioAvailableEventManager::QueueWrittenAudioData(AudioDataValue* aAudioData,
                                                         uint32_t aAudioDataLength,
                                                         uint64_t aEndTimeSampleOffset)
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  if (!mHasListener) {
    return;
  }

  uint32_t currentBufferSize = mNewSignalBufferLength;
  if (currentBufferSize == 0) {
    NS_WARNING("Decoder framebuffer length not set.");
    return;
  }

  if (!mSignalBuffer ||
      (mSignalBufferPosition == 0 && mSignalBufferLength != currentBufferSize)) {
    if (!mSignalBuffer || (mSignalBufferLength < currentBufferSize)) {
      
      mSignalBuffer = new float[currentBufferSize];
    }
    mSignalBufferLength = currentBufferSize;
  }
  AudioDataValue* audioData = aAudioData;
  uint32_t audioDataLength = aAudioDataLength;
  uint32_t signalBufferTail = mSignalBufferLength - mSignalBufferPosition;

  
  while (signalBufferTail <= audioDataLength) {
    float time = 0.0;
    
    if (aEndTimeSampleOffset > mSignalBufferPosition + audioDataLength) {
      time = (aEndTimeSampleOffset - mSignalBufferPosition - audioDataLength) / 
             mSamplesPerSecond;
    }

    
    uint32_t i;
    float *signalBuffer = mSignalBuffer.get() + mSignalBufferPosition;
    if (audioData) {
      for (i = 0; i < signalBufferTail; ++i) {
        signalBuffer[i] = MOZ_CONVERT_AUDIO_SAMPLE(audioData[i]);
      }
    } else {
      memset(signalBuffer, 0, signalBufferTail*sizeof(signalBuffer[0]));
    }
    if (audioData) {
      audioData += signalBufferTail;
    }

    NS_ASSERTION(audioDataLength >= signalBufferTail,
                 "audioDataLength about to wrap past zero to +infinity!");
    audioDataLength -= signalBufferTail;

    if (mPendingEvents.Length() > 0) {
      
      
      nsAudioAvailableEventRunner* lastPendingEvent =
        (nsAudioAvailableEventRunner*)mPendingEvents[mPendingEvents.Length() - 1].get();
      if (lastPendingEvent->mTime > time) {
        
        mPendingEvents.Clear();
      } else if (mPendingEvents.Length() >= MAX_PENDING_EVENTS) {
        NS_WARNING("Hit audio event queue max.");
        mPendingEvents.RemoveElementsAt(0, mPendingEvents.Length() - MAX_PENDING_EVENTS + 1);
      }
    }

    
    nsCOMPtr<nsIRunnable> event =
      new nsAudioAvailableEventRunner(mDecoder, mSignalBuffer.forget(),
                                      mSignalBufferLength, time);
    mPendingEvents.AppendElement(event);

    
    mSignalBufferLength = currentBufferSize;
    mSignalBuffer = new float[currentBufferSize];
    mSignalBufferPosition = 0;
    signalBufferTail = currentBufferSize;
  }

  NS_ASSERTION(mSignalBufferPosition + audioDataLength < mSignalBufferLength,
               "Intermediate signal buffer must fit at least one more item.");

  if (audioDataLength > 0) {
    
    uint32_t i;
    float *signalBuffer = mSignalBuffer.get() + mSignalBufferPosition;
    if (audioData) {
      for (i = 0; i < audioDataLength; ++i) {
        signalBuffer[i] = MOZ_CONVERT_AUDIO_SAMPLE(audioData[i]);
      }
    } else {
      memset(signalBuffer, 0, audioDataLength*sizeof(signalBuffer[0]));
    }
    mSignalBufferPosition += audioDataLength;
  }
}

void nsAudioAvailableEventManager::Clear()
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  mPendingEvents.Clear();
  mSignalBufferPosition = 0;
}

void nsAudioAvailableEventManager::Drain(uint64_t aEndTime)
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  if (!mHasListener) {
    return;
  }

  
  for (uint32_t i = 0; i < mPendingEvents.Length(); ++i) {
    nsCOMPtr<nsIRunnable> event = mPendingEvents[i];
    NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
  }
  mPendingEvents.Clear();

  
  if (0 == mSignalBufferPosition)
    return;

  
  memset(mSignalBuffer.get() + mSignalBufferPosition, 0,
         (mSignalBufferLength - mSignalBufferPosition) * sizeof(float));

  
  float time = (aEndTime / static_cast<float>(USECS_PER_S)) - 
               (mSignalBufferPosition / mSamplesPerSecond);
  nsCOMPtr<nsIRunnable> lastEvent =
    new nsAudioAvailableEventRunner(mDecoder, mSignalBuffer.forget(),
                                    mSignalBufferLength, time);
  NS_DispatchToMainThread(lastEvent, NS_DISPATCH_NORMAL);

  mSignalBufferPosition = 0;
}

void nsAudioAvailableEventManager::SetSignalBufferLength(uint32_t aLength)
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  mNewSignalBufferLength = aLength;
}

void nsAudioAvailableEventManager::NotifyAudioAvailableListener()
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  mHasListener = true;
}
