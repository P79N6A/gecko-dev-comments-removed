






































#include "nsTArray.h"
#include "nsAudioAvailableEventManager.h"
#include "VideoUtils.h"

#define MAX_PENDING_EVENTS 100

using namespace mozilla;

class nsAudioAvailableEventRunner : public nsRunnable
{
private:
  nsCOMPtr<nsBuiltinDecoder> mDecoder;
  nsAutoArrayPtr<float> mFrameBuffer;
public:
  nsAudioAvailableEventRunner(nsBuiltinDecoder* aDecoder, float* aFrameBuffer,
                              PRUint32 aFrameBufferLength, float aTime) :
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

  const PRUint32 mFrameBufferLength;

  
  const float mTime;
};


nsAudioAvailableEventManager::nsAudioAvailableEventManager(nsBuiltinDecoder* aDecoder) :
  mDecoder(aDecoder),
  mSignalBuffer(new float[mDecoder->GetFrameBufferLength()]),
  mSignalBufferLength(mDecoder->GetFrameBufferLength()),
  mNewSignalBufferLength(mSignalBufferLength),
  mSignalBufferPosition(0),
  mMonitor("media.audioavailableeventmanager")
{
  MOZ_COUNT_CTOR(nsAudioAvailableEventManager);
}

nsAudioAvailableEventManager::~nsAudioAvailableEventManager()
{
  MOZ_COUNT_DTOR(nsAudioAvailableEventManager);
}

void nsAudioAvailableEventManager::Init(PRUint32 aChannels, PRUint32 aRate)
{
  NS_ASSERTION(aChannels != 0 && aRate != 0, "Audio metadata not known.");
  mSamplesPerSecond = static_cast<float>(aChannels * aRate);
}

void nsAudioAvailableEventManager::DispatchPendingEvents(PRUint64 aCurrentTime)
{
  MonitorAutoEnter mon(mMonitor);

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

void nsAudioAvailableEventManager::QueueWrittenAudioData(SoundDataValue* aAudioData,
                                                         PRUint32 aAudioDataLength,
                                                         PRUint64 aEndTimeSampleOffset)
{
  MonitorAutoEnter mon(mMonitor);

  PRUint32 currentBufferSize = mNewSignalBufferLength;
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
  SoundDataValue* audioData = aAudioData;
  PRUint32 audioDataLength = aAudioDataLength;
  PRUint32 signalBufferTail = mSignalBufferLength - mSignalBufferPosition;

  
  while (signalBufferTail <= audioDataLength) {
    float time = 0.0;
    
    if (aEndTimeSampleOffset > mSignalBufferPosition + audioDataLength) {
      time = (aEndTimeSampleOffset - mSignalBufferPosition - audioDataLength) / 
             mSamplesPerSecond;
    }

    
    PRUint32 i;
    float *signalBuffer = mSignalBuffer.get() + mSignalBufferPosition;
    for (i = 0; i < signalBufferTail; ++i) {
      signalBuffer[i] = MOZ_CONVERT_SOUND_SAMPLE(audioData[i]);
    }
    audioData += signalBufferTail;
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
    NS_ASSERTION(audioDataLength >= 0, "Past new signal data length.");
  }

  NS_ASSERTION(mSignalBufferPosition + audioDataLength < mSignalBufferLength,
               "Intermediate signal buffer must fit at least one more item.");

  if (audioDataLength > 0) {
    
    PRUint32 i;
    float *signalBuffer = mSignalBuffer.get() + mSignalBufferPosition;
    for (i = 0; i < audioDataLength; ++i) {
      signalBuffer[i] = MOZ_CONVERT_SOUND_SAMPLE(audioData[i]);
    }
    mSignalBufferPosition += audioDataLength;
  }
}

void nsAudioAvailableEventManager::Clear()
{
  MonitorAutoEnter mon(mMonitor);

  mPendingEvents.Clear();
  mSignalBufferPosition = 0;
}

void nsAudioAvailableEventManager::Drain(PRUint64 aEndTime)
{
  MonitorAutoEnter mon(mMonitor);

  
  for (PRUint32 i = 0; i < mPendingEvents.Length(); ++i) {
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

void nsAudioAvailableEventManager::SetSignalBufferLength(PRUint32 aLength)
{
  MonitorAutoEnter mon(mMonitor);

  mNewSignalBufferLength = aLength;
}

