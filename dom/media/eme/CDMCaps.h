





#ifndef CDMCaps_h_
#define CDMCaps_h_

#include "nsString.h"
#include "nsAutoPtr.h"
#include "mozilla/Monitor.h"
#include "nsIThread.h"
#include "nsTArray.h"
#include "mozilla/Attributes.h"
#include "SamplesWaitingForKey.h"
#include "gmp-decryption.h"

namespace mozilla {



class CDMCaps {
public:
  CDMCaps();
  ~CDMCaps();

  struct KeyStatus {
    KeyStatus(const CencKeyId& aId,
              const nsString& aSessionId,
              GMPMediaKeyStatus aStatus)
      : mId(aId)
      , mSessionId(aSessionId)
      , mStatus(aStatus)
    {}
    KeyStatus(const KeyStatus& aOther)
      : mId(aOther.mId)
      , mSessionId(aOther.mSessionId)
      , mStatus(aOther.mStatus)
    {}
    bool operator==(const KeyStatus& aOther) const {
      return mId == aOther.mId &&
             mSessionId == aOther.mSessionId;
    };

    CencKeyId mId;
    nsString mSessionId;
    GMPMediaKeyStatus mStatus;
  };

  
  
  class MOZ_STACK_CLASS AutoLock {
  public:
    explicit AutoLock(CDMCaps& aKeyCaps);
    ~AutoLock();

    
    
    bool AreCapsKnown();

    bool IsKeyUsable(const CencKeyId& aKeyId);

    
    
    bool SetKeyStatus(const CencKeyId& aKeyId, const nsString& aSessionId, GMPMediaKeyStatus aStatus);

    void GetKeyStatusesForSession(const nsAString& aSessionId,
                                  nsTArray<KeyStatus>& aOutKeyStatuses);

    
    
    void SetCaps(uint64_t aCaps);

    bool CanDecryptAndDecodeAudio();
    bool CanDecryptAndDecodeVideo();

    bool CanDecryptAudio();
    bool CanDecryptVideo();

    void CallOnMainThreadWhenCapsAvailable(nsIRunnable* aContinuation);

    
    void NotifyWhenKeyIdUsable(const CencKeyId& aKey,
                               SamplesWaitingForKey* aSamplesWaiting);
  private:
    
    CDMCaps& mData;
  };

private:
  void Lock();
  void Unlock();
  bool HasCap(uint64_t);

  struct WaitForKeys {
    WaitForKeys(const CencKeyId& aKeyId,
                SamplesWaitingForKey* aListener)
      : mKeyId(aKeyId)
      , mListener(aListener)
    {}
    CencKeyId mKeyId;
    nsRefPtr<SamplesWaitingForKey> mListener;
  };

  Monitor mMonitor;

  nsTArray<KeyStatus> mKeyStatuses;

  nsTArray<WaitForKeys> mWaitForKeys;

  nsTArray<nsCOMPtr<nsIRunnable>> mWaitForCaps;
  uint64_t mCaps;

  
  CDMCaps(const CDMCaps&) = delete;
  CDMCaps& operator=(const CDMCaps&) = delete;
};

} 

#endif
