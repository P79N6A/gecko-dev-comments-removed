






































#include "mozilla/dom/AudioParent.h"
#include "mozilla/unused.h"
#include "nsThreadUtils.h"


namespace mozilla {
namespace dom {

class AudioWriteEvent : public nsRunnable
{
 public:
  AudioWriteEvent(nsAudioStream* owner, nsCString data, PRUint32 frames)
  {
    mOwner = owner;
    mData  = data;
    mFrames = frames;
  }

  NS_IMETHOD Run()
  {
    mOwner->Write(mData.get(), mFrames);
    return NS_OK;
  }

 private:
    nsRefPtr<nsAudioStream> mOwner;
    nsCString mData;
    PRUint32  mFrames;
};

class AudioPauseEvent : public nsRunnable
{
 public:
  AudioPauseEvent(nsAudioStream* owner, bool aPause)
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
    bool mPause;
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


class AudioMinWriteSizeDone : public nsRunnable
{
 public:
  AudioMinWriteSizeDone(AudioParent* owner, PRInt32 minFrames)
  {
    mOwner = owner;
    mMinFrames = minFrames;
  }

  NS_IMETHOD Run()
  {
    mOwner->SendMinWriteSizeDone(mMinFrames);
    return NS_OK;
  }

 private:
    nsRefPtr<AudioParent> mOwner;
    PRInt32 mMinFrames;
};

class AudioMinWriteSizeEvent : public nsRunnable
{
 public:
  AudioMinWriteSizeEvent(AudioParent* parent, nsAudioStream* owner)
  {
    mParent = parent;
    mOwner = owner;
  }

  NS_IMETHOD Run()
  {
    PRInt32 minFrames = mOwner->GetMinWriteSize();
    nsCOMPtr<nsIRunnable> event = new AudioMinWriteSizeDone(mParent, minFrames);
    NS_DispatchToMainThread(event);
    return NS_OK;
  }

 private:
    nsRefPtr<nsAudioStream> mOwner;
    nsRefPtr<AudioParent> mParent;
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
  PRInt64 position = mStream->GetPositionInFrames();
  unused << SendPositionInFramesUpdate(position, PR_IntervalNow());
  return NS_OK;
}

bool
AudioParent::RecvWrite(const nsCString& data, const PRUint32& frames)
{
  if (!mStream)
    return false;
  nsCOMPtr<nsIRunnable> event = new AudioWriteEvent(mStream, data, frames);
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
AudioParent::RecvMinWriteSize()
{
  if (!mStream)
    return false;
  nsCOMPtr<nsIRunnable> event = new AudioMinWriteSizeEvent(this, mStream);
  nsCOMPtr<nsIThread> thread = mStream->GetThread();
  thread->Dispatch(event, nsIEventTarget::DISPATCH_NORMAL);
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
  unused << PAudioParent::Send__delete__(this);
  return true;
}

bool
AudioParent::SendMinWriteSizeDone(PRInt32 minFrames)
{
  if (mIPCOpen)
    return PAudioParent::SendMinWriteSizeDone(minFrames);
  return true;
}

bool
AudioParent::SendDrainDone()
{
  if (mIPCOpen)
    return PAudioParent::SendDrainDone();
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
