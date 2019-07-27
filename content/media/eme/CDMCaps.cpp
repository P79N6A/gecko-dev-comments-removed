





#include "CDMCaps.h"
#include "gmp-decryption.h"
#include "EMELog.h"
#include "nsThreadUtils.h"

namespace mozilla {

CDMCaps::CDMCaps()
  : mMonitor("CDMCaps")
  , mCaps(0)
{
}

CDMCaps::~CDMCaps()
{
}

void
CDMCaps::Lock()
{
  mMonitor.Lock();
}

void
CDMCaps::Unlock()
{
  mMonitor.Unlock();
}

bool
CDMCaps::HasCap(uint64_t aCap)
{
  mMonitor.AssertCurrentThreadOwns();
  return (mCaps & aCap) == aCap;
}

CDMCaps::AutoLock::AutoLock(CDMCaps& aInstance)
  : mData(aInstance)
{
  mData.Lock();
}

CDMCaps::AutoLock::~AutoLock()
{
  mData.Unlock();
}

void
CDMCaps::AutoLock::SetCaps(uint64_t aCaps)
{
  EME_LOG("SetCaps()");
  mData.mMonitor.AssertCurrentThreadOwns();
  mData.mCaps = aCaps;
  for (size_t i = 0; i < mData.mWaitForCaps.Length(); i++) {
    NS_DispatchToMainThread(mData.mWaitForCaps[i], NS_DISPATCH_NORMAL);
  }
  mData.mWaitForCaps.Clear();
}

void
CDMCaps::AutoLock::CallOnMainThreadWhenCapsAvailable(nsIRunnable* aContinuation)
{
  mData.mMonitor.AssertCurrentThreadOwns();
  if (mData.mCaps) {
    NS_DispatchToMainThread(aContinuation, NS_DISPATCH_NORMAL);
    MOZ_ASSERT(mData.mWaitForCaps.IsEmpty());
  } else {
    mData.mWaitForCaps.AppendElement(aContinuation);
  }
}

bool
CDMCaps::AutoLock::IsKeyUsable(const CencKeyId& aKeyId)
{
  mData.mMonitor.AssertCurrentThreadOwns();
  const auto& keys = mData.mUsableKeyIds;
  for (size_t i = 0; i < keys.Length(); i++) {
    if (keys[i].mId == aKeyId) {
      return true;
    }
  }
  return false;
}

void
CDMCaps::AutoLock::SetKeyUsable(const CencKeyId& aKeyId,
                                const nsString& aSessionId)
{
  mData.mMonitor.AssertCurrentThreadOwns();
  mData.mUsableKeyIds.AppendElement(UsableKey(aKeyId, aSessionId));
  auto& waiters = mData.mWaitForKeys;
  size_t i = 0;
  while (i < waiters.Length()) {
    auto& w = waiters[i];
    if (w.mKeyId == aKeyId) {
      if (waiters[i].mTarget) {
        EME_LOG("SetKeyUsable() notified waiter.");
        w.mTarget->Dispatch(w.mContinuation, NS_DISPATCH_NORMAL);
      } else {
        w.mContinuation->Run();
      }
      waiters.RemoveElementAt(i);
    } else {
      i++;
    }
  }
}

void
CDMCaps::AutoLock::SetKeyUnusable(const CencKeyId& aKeyId,
                                  const nsString& aSessionId)
{
  mData.mMonitor.AssertCurrentThreadOwns();
  auto& keys = mData.mUsableKeyIds;
  for (size_t i = 0; i < keys.Length(); i++) {
    if (keys[i].mId == aKeyId &&
        keys[i].mSessionId == aSessionId) {
      keys.RemoveElementAt(i);
      break;
    }
  }
}

void
CDMCaps::AutoLock::CallWhenKeyUsable(const CencKeyId& aKey,
                                     nsIRunnable* aContinuation,
                                     nsIThread* aTarget)
{
  mData.mMonitor.AssertCurrentThreadOwns();
  MOZ_ASSERT(!IsKeyUsable(aKey));
  MOZ_ASSERT(aContinuation);
  mData.mWaitForKeys.AppendElement(WaitForKeys(aKey, aContinuation, aTarget));
}

void
CDMCaps::AutoLock::DropKeysForSession(const nsAString& aSessionId)
{
  mData.mMonitor.AssertCurrentThreadOwns();
  auto& keys = mData.mUsableKeyIds;
  size_t i = 0;
  while (i < keys.Length()) {
    if (keys[i].mSessionId == aSessionId) {
      keys.RemoveElementAt(i);
    } else {
      i++;
    }
  }
}

bool
CDMCaps::AutoLock::AreCapsKnown()
{
  mData.mMonitor.AssertCurrentThreadOwns();
  return mData.mCaps != 0;
}

bool
CDMCaps::AutoLock::CanDecryptAndDecodeAudio()
{
  return mData.HasCap(GMP_EME_CAP_DECRYPT_AND_DECODE_AUDIO);
}

bool
CDMCaps::AutoLock::CanDecryptAndDecodeVideo()
{
  return mData.HasCap(GMP_EME_CAP_DECRYPT_AND_DECODE_VIDEO);
}

bool
CDMCaps::AutoLock::CanDecryptAudio()
{
  return mData.HasCap(GMP_EME_CAP_DECRYPT_AUDIO);
}

bool
CDMCaps::AutoLock::CanDecryptVideo()
{
  return mData.HasCap(GMP_EME_CAP_DECRYPT_VIDEO);
}

} 