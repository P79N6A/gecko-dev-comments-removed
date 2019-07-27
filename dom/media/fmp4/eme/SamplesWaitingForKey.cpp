





#include "SamplesWaitingForKey.h"
#include "mozilla/CDMProxy.h"
#include "mozilla/CDMCaps.h"

namespace mozilla {

SamplesWaitingForKey::SamplesWaitingForKey(MediaDataDecoder* aDecoder,
                                           MediaTaskQueue* aTaskQueue,
                                           CDMProxy* aProxy)
  : mMutex("SamplesWaitingForKey")
  , mDecoder(aDecoder)
  , mTaskQueue(aTaskQueue)
  , mProxy(aProxy)
{
}

SamplesWaitingForKey::~SamplesWaitingForKey()
{
}

bool
SamplesWaitingForKey::WaitIfKeyNotUsable(MP4Sample* aSample)
{
  if (!aSample || !aSample->crypto.valid || !mProxy) {
    return false;
  }
  CDMCaps::AutoLock caps(mProxy->Capabilites());
  const auto& keyid = aSample->crypto.key;
  if (!caps.IsKeyUsable(keyid)) {
    {
      MutexAutoLock lock(mMutex);
      mSamples.AppendElement(aSample);
    }
    caps.NotifyWhenKeyIdUsable(aSample->crypto.key, this);
    return true;
  }
  return false;
}

void
SamplesWaitingForKey::NotifyUsable(const CencKeyId& aKeyId)
{
  MutexAutoLock lock(mMutex);
  size_t i = 0;
  while (i < mSamples.Length()) {
    if (aKeyId == mSamples[i]->crypto.key) {
      RefPtr<nsIRunnable> task;
      task = NS_NewRunnableMethodWithArg<MP4Sample*>(mDecoder,
                                                     &MediaDataDecoder::Input,
                                                     mSamples[i].forget());
      mSamples.RemoveElementAt(i);
      mTaskQueue->Dispatch(task.forget());
    } else {
      i++;
    }
  }
}

void
SamplesWaitingForKey::Flush()
{
  MutexAutoLock lock(mMutex);
  mSamples.Clear();
}

void
SamplesWaitingForKey::BreakCycles()
{
  MutexAutoLock lock(mMutex);
  mDecoder = nullptr;
  mTaskQueue = nullptr;
  mProxy = nullptr;
  mSamples.Clear();
}

} 
