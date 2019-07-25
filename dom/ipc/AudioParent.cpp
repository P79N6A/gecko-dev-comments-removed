






































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

class AudioStreamShutdownEvent : public nsRunnable
{
 public:
  AudioStreamShutdownEvent(nsAudioStream* owner)
  {
    mOwner = owner;
  }

  NS_IMETHOD Run()
  {
    mOwner->Shutdown();
    return NS_OK;
  }

 private:
    nsRefPtr<nsAudioStream> mOwner;
};

class AudioDrainDoneEvent : public nsRunnable
{
 public:
  AudioDrainDoneEvent(AudioParent* owner)
  {
    mOwner = owner;
  }

  NS_IMETHOD Run()
  {
    mOwner->SendDrainDone();
    return NS_OK;
  }

 private:
    nsRefPtr<AudioParent> mOwner;
};

class AudioDrainEvent : public nsRunnable
{
 public:
  AudioDrainEvent(AudioParent* parent, nsAudioStream* owner)
  {
    mParent = parent;
    mOwner = owner;
  }

  NS_IMETHOD Run()
  {
    mOwner->Drain();
    nsCOMPtr<nsIRunnable> event = new AudioDrainDoneEvent(mParent);
    NS_DispatchToMainThread(event);
    return NS_OK;
  }

 private:
    nsRefPtr<nsAudioStream> mOwner;
    nsRefPtr<AudioParent> mParent;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(AudioParent, nsITimerCallback)

nsresult
AudioParent::Notify(nsITimer* timer)
{
  if (!mIPCOpen) {
    timer->Cancel();
    return NS_ERROR_FAILURE;
  }

  NS_ASSERTION(mStream, "AudioStream not initialized.");
  PRInt64 offset = mStream->GetSampleOffset();
  SendSampleOffsetUpdate(offset, PR_IntervalNow());
  return NS_OK;
}

bool
AudioParent::RecvWrite(
        const nsCString& data,
        const PRUint32& count)
{
  if (!mStream)
    return false;
  nsCOMPtr<nsIRunnable> event = new AudioWriteEvent(mStream, data, count);
  nsCOMPtr<nsIThread> thread = mStream->GetThread();
  thread->Dispatch(event, nsIEventTarget::DISPATCH_NORMAL);
  return true;
}

bool
AudioParent::RecvSetVolume(const float& aVolume)
{
  if (!mStream)
      return false;
  mStream->SetVolume(aVolume);
  return true;
}

bool
AudioParent::RecvDrain()
{
  if (!mStream)
    return false;
  nsCOMPtr<nsIRunnable> event = new AudioDrainEvent(this, mStream);
  nsCOMPtr<nsIThread> thread = mStream->GetThread();
  thread->Dispatch(event, nsIEventTarget::DISPATCH_NORMAL);
  return true;
}

bool
AudioParent::RecvPause()
{
  if (!mStream)
    return false;
  nsCOMPtr<nsIRunnable> event = new AudioPauseEvent(mStream, PR_TRUE);
  nsCOMPtr<nsIThread> thread = mStream->GetThread();
  thread->Dispatch(event, nsIEventTarget::DISPATCH_NORMAL);
  return true;
}

bool
AudioParent::RecvResume()
{
  if (!mStream)
    return false;
  nsCOMPtr<nsIRunnable> event = new AudioPauseEvent(mStream, PR_FALSE);
  nsCOMPtr<nsIThread> thread = mStream->GetThread();
  thread->Dispatch(event, nsIEventTarget::DISPATCH_NORMAL);
  return true;
}

bool
AudioParent::RecvShutdown()
{
  Shutdown();
  PAudioParent::Send__delete__(this);
  return true;
}

AudioParent::AudioParent(PRInt32 aNumChannels, PRInt32 aRate, PRInt32 aFormat)
  : mIPCOpen(PR_TRUE)
{
  mStream = nsAudioStream::AllocateStream();
  NS_ASSERTION(mStream, "AudioStream allocation failed.");
  if (NS_FAILED(mStream->Init(aNumChannels,
                              aRate,
                              (nsAudioStream::SampleFormat) aFormat))) {
      NS_WARNING("AudioStream initialization failed.");
      mStream = nsnull;
      return;
  }

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

  Shutdown();
}

void
AudioParent::Shutdown()
{
  if (mTimer) {
    mTimer->Cancel();
    mTimer = nsnull;
  }

  if (mStream) {
      nsCOMPtr<nsIRunnable> event = new AudioStreamShutdownEvent(mStream);
      nsCOMPtr<nsIThread> thread = mStream->GetThread();
      thread->Dispatch(event, nsIEventTarget::DISPATCH_NORMAL);
      mStream = nsnull;
  }
}

} 
} 
