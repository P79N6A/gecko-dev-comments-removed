





#ifndef CDMCaps_h_
#define CDMCaps_h_

#include "nsString.h"
#include "nsAutoPtr.h"
#include "mozilla/Monitor.h"
#include "nsIThread.h"
#include "nsTArray.h"
#include "mozilla/Attributes.h"
#include "SamplesWaitingForKey.h"

namespace mozilla {



class CDMCaps {
public:
  CDMCaps();
  ~CDMCaps();

  
  
  class MOZ_STACK_CLASS AutoLock {
  public:
    explicit AutoLock(CDMCaps& aKeyCaps);
    ~AutoLock();

    
    
    bool AreCapsKnown();

    bool IsKeyUsable(const CencKeyId& aKeyId);

    
    
    bool SetKeyUsable(const CencKeyId& aKeyId, const nsString& aSessionId);

    
    
    bool SetKeyUnusable(const CencKeyId& aKeyId, const nsString& aSessionId);

    void GetUsableKeysForSession(const nsAString& aSessionId,
                                 nsTArray<CencKeyId>& aOutKeyIds);

    
    
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

  struct UsableKey {
    UsableKey(const CencKeyId& aId,
              const nsString& aSessionId)
      : mId(aId)
      , mSessionId(aSessionId)
    {}
    UsableKey(const UsableKey& aOther)
      : mId(aOther.mId)
      , mSessionId(aOther.mSessionId)
    {}
    bool operator==(const UsableKey& aOther) const {
      return mId == aOther.mId &&
             mSessionId == aOther.mSessionId;
    };

    CencKeyId mId;
    nsString mSessionId;
  };
  nsTArray<UsableKey> mUsableKeyIds;

  nsTArray<WaitForKeys> mWaitForKeys;

  nsTArray<nsRefPtr<nsIRunnable>> mWaitForCaps;
  uint64_t mCaps;

  
  CDMCaps(const CDMCaps&) MOZ_DELETE;
  CDMCaps& operator=(const CDMCaps&) MOZ_DELETE;
};

} 

#endif
