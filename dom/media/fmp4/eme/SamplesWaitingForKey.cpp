





#include "SamplesWaitingForKey.h"
#include "mozilla/CDMProxy.h"
#include "mozilla/CDMCaps.h"
#include "MediaData.h"

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
SamplesWaitingForKey::WaitIfKeyNotUsable(MediaRawData* aSample)
{
  if (!aSample || !aSample->mCrypto.valid || !mProxy) {
    return false;
  }
  CDMCaps::AutoLock caps(mProxy->Capabilites());
  const auto& keyid = aSample->mCrypto.key;
  if (!caps.IsKeyUsable(keyid)) {
    {
      MutexAutoLock lock(mMutex);
      mSamples.AppendElement(aSample);
    }
    caps.NotifyWhenKeyIdUsable(aSample->mCrypto.key, this);
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
    if (aKeyId == mSamples[i]->mCrypto.key) {
      RefPtr<nsIRunnable> task;
      task = NS_NewRunnableMethodWithArg<nsRefPtr<MediaRawData>>(mDecoder,
                                                     &MediaDataDecoder::Input,
                                                     nsRefPtr<MediaRawData>(mSamples[i]));
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
