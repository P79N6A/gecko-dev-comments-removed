






































#include "nsTArray.h"
#include "nsAudioAvailableEventManager.h"

#define MILLISECONDS_PER_SECOND 1000

using namespace mozilla;

class nsAudioAvailableEventRunner : public nsRunnable
{
private:
  nsCOMPtr<nsBuiltinDecoder> mDecoder;
  nsAutoArrayPtr<float> mFrameBuffer;
public:
  nsAudioAvailableEventRunner(nsBuiltinDecoder* aDecoder, float* aFrameBuffer,
                              PRUint32 aFrameBufferLength, PRUint64 aTime) :
    mDecoder(aDecoder),
    mFrameBuffer(aFrameBuffer),
    mFrameBufferLength(aFrameBufferLength),
    mTime(aTime)
  {
  }

  NS_IMETHOD Run()
  {
    mDecoder->AudioAvailable(mFrameBuffer.forget(), mFrameBufferLength, mTime);
    return NS_OK;
  }

  const PRUint32 mFrameBufferLength;
  const PRUint64 mTime;
};


nsAudioAvailableEventManager::nsAudioAvailableEventManager(nsBuiltinDecoder* aDecoder) :
  mDecoder(aDecoder),
  mSignalBuffer(new float[mDecoder->GetFrameBufferLength()]),
  mSignalBufferLength(mDecoder->GetFrameBufferLength()),
  mSignalBufferPosition(0),
  mMonitor("media.audioavailableeventmanager")
{
}

void nsAudioAvailableEventManager::Init(PRUint32 aChannels, PRUint32 aRate)
{
  mSamplesPerSecond = aChannels * aRate;
}

void nsAudioAvailableEventManager::DispatchPendingEvents(PRUint64 aCurrentTime)
{
  MonitorAutoEnter mon(mMonitor);

  while (mPendingEvents.Length() > 0) {
    nsAudioAvailableEventRunner* e =
      (nsAudioAvailableEventRunner*)mPendingEvents[0].get();
    if (e->mTime > aCurrentTime) {
      break;
    }
    nsCOMPtr<nsIRunnable> event = mPendingEvents[0];
    mPendingEvents.RemoveElementAt(0);
    NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
  }
}

void nsAudioAvailableEventManager::QueueWrittenAudioData(float* aAudioData,
                                                         PRUint32 aAudioDataLength,
                                                         PRUint64 aEndTimeSampleOffset)
{
  PRUint32 currentBufferSize = mDecoder->GetFrameBufferLength();
  if (!mSignalBuffer ||
      (mSignalBufferPosition == 0 && mSignalBufferLength != currentBufferSize)) {
    if (!mSignalBuffer || (mSignalBufferLength < currentBufferSize)) {
      
      mSignalBuffer = new float[currentBufferSize];
    }
    mSignalBufferLength = currentBufferSize;
  }
  float* audioData = aAudioData;
  PRUint32 audioDataLength = aAudioDataLength;
  PRUint32 signalBufferTail = mSignalBufferLength - mSignalBufferPosition;

  
  while (signalBufferTail <= audioDataLength) {
    PRUint64 time = 0;
    
    if (aEndTimeSampleOffset > mSignalBufferPosition + audioDataLength) {
      time = MILLISECONDS_PER_SECOND * (aEndTimeSampleOffset -
             mSignalBufferPosition - audioDataLength) / mSamplesPerSecond;
    }

    
    memcpy(mSignalBuffer.get() + mSignalBufferPosition,
           audioData, sizeof(float) * signalBufferTail);
    audioData += signalBufferTail;
    audioDataLength -= signalBufferTail;

    MonitorAutoEnter mon(mMonitor);

    if (mPendingEvents.Length() > 0) {
      
      
      nsAudioAvailableEventRunner* lastPendingEvent =
        (nsAudioAvailableEventRunner*)mPendingEvents[mPendingEvents.Length() - 1].get();
      if (lastPendingEvent->mTime > time) {
        
        mPendingEvents.Clear();
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
    
    memcpy(mSignalBuffer.get() + mSignalBufferPosition,
           audioData, sizeof(float) * audioDataLength);
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

  
  nsCOMPtr<nsIRunnable> lastEvent =
    new nsAudioAvailableEventRunner(mDecoder, mSignalBuffer.forget(),
                                    mSignalBufferLength, aEndTime);
  NS_DispatchToMainThread(lastEvent, NS_DISPATCH_NORMAL);

  mSignalBufferPosition = 0;
}
