






































#include "mozilla/dom/AudioParent.h"
#include "nsThreadUtils.h"


namespace mozilla {
namespace dom {

class AudioWriteEvent : public nsRunnable
{
 public:
  AudioWriteEvent(nsAudioStream* owner, nsCString data, PRUint32 count)
  {
    mOwner = owner;
    mData  = data;
    mCount = count;
  }

  NS_IMETHOD Run()
  {
    mOwner->Write(mData.get(), mCount, true);
    return NS_OK;
  }

 private:
    nsRefPtr<nsAudioStream> mOwner;
    nsCString mData;
    PRUint32  mCount;
};

class AudioPauseEvent : public nsRunnable
{
 public:
  AudioPauseEvent(nsAudioStream* owner, PRBool aPause)
  {
    mOwner = owner;
    mPause = aPause;
  }

  NS_IMETHOD Run()
  {
    if (mPause)
        mOwner->Pause();
    else
        mOwner->Resume();
    return NS_OK;
  }

 private:
    nsRefPtr<nsAudioStream> mOwner;
    PRBool mPause;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(AudioParent, nsITimerCallback)

nsresult
AudioParent::Notify(nsITimer* timer)
{
  if (!mIPCOpen || !mStream) {
    timer->Cancel();
    return NS_ERROR_FAILURE;
  }

  PRInt64 offset = mStream->GetSampleOffset();
  SendSampleOffsetUpdate(offset, PR_IntervalNow());
  return NS_OK;
}
bool
AudioParent::RecvWrite(
        const nsCString& data,
        const PRUint32& count)
{
  nsCOMPtr<nsIRunnable> event = new AudioWriteEvent(mStream, data, count);
  nsCOMPtr<nsIThread> thread = nsAudioStream::GetGlobalThread();
  thread->Dispatch(event, nsIEventTarget::DISPATCH_NORMAL);
  return true;
}
    
bool
AudioParent::RecvSetVolume(const float& aVolume)
{
  if (mStream)
    mStream->SetVolume(aVolume);
  return true;
}

bool
AudioParent::RecvDrain()
{
  if (mStream)
    mStream->Drain();
  return true;
}

bool
AudioParent::RecvPause()
{
  nsCOMPtr<nsIRunnable> event = new AudioPauseEvent(mStream, PR_TRUE);
  nsCOMPtr<nsIThread> thread = nsAudioStream::GetGlobalThread();
  thread->Dispatch(event, nsIEventTarget::DISPATCH_NORMAL);
  return true;
}

bool
AudioParent::RecvResume()
{
  nsCOMPtr<nsIRunnable> event = new AudioPauseEvent(mStream, PR_FALSE);
  nsCOMPtr<nsIThread> thread = nsAudioStream::GetGlobalThread();
  thread->Dispatch(event, nsIEventTarget::DISPATCH_NORMAL);
  return true;
}

bool
AudioParent::Recv__delete__()
{
  if (mStream) {
    mStream->Shutdown();
    mStream = nsnull;
  }

  if (mTimer) {
    mTimer->Cancel();
    mTimer = nsnull;
  }
  return true;
}

AudioParent::AudioParent(PRInt32 aNumChannels, PRInt32 aRate, PRInt32 aFormat)
  : mIPCOpen(PR_TRUE)
{
  mStream = nsAudioStream::AllocateStream();
  if (mStream)
    mStream->Init(aNumChannels,
                  aRate,
                  (nsAudioStream::SampleFormat) aFormat);
  if (!mStream)
    return; 

  mTimer = do_CreateInstance("@mozilla.org/timer;1");
  mTimer->InitWithCallback(this, 1000, nsITimer::TYPE_REPEATING_SLACK);
}

AudioParent::~AudioParent()
{
}

void
AudioParent::ActorDestroy(ActorDestroyReason aWhy)
{
  mIPCOpen = PR_FALSE;
}

} 
} 
