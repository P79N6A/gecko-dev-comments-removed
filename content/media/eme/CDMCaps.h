





#ifndef CDMCaps_h_
#define CDMCaps_h_

#include "nsString.h"
#include "nsAutoPtr.h"
#include "mozilla/Monitor.h"
#include "nsIThread.h"
#include "nsTArray.h"
#include "mozilla/Attributes.h"

namespace mozilla {

typedef nsTArray<uint8_t> CencKeyId;



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

    void SetKeyUsable(const CencKeyId& aKeyId, const nsString& aSessionId);
    void SetKeyUnusable(const CencKeyId& aKeyId, const nsString& aSessionId);

    void DropKeysForSession(const nsAString& aSessionId);

    
    
    void SetCaps(uint64_t aCaps);

    bool CanDecryptAndDecodeAudio();
    bool CanDecryptAndDecodeVideo();

    bool CanDecryptAudio();
    bool CanDecryptVideo();

    void CallOnMainThreadWhenCapsAvailable(nsIRunnable* aContinuation);

    
    
    
    void CallWhenKeyUsable(const CencKeyId& aKey,
                           nsIRunnable* aContinuation,
                           nsIThread* aTarget = nullptr);

  private:
    
    CDMCaps& mData;
  };

private:
  void Lock();
  void Unlock();
  bool HasCap(uint64_t);

  struct WaitForKeys {
    WaitForKeys(const CencKeyId& aKeyId,
                nsIRunnable* aContinuation,
                nsIThread* aTarget)
      : mKeyId(aKeyId)
      , mContinuation(aContinuation)
      , mTarget(aTarget)
    {}
    CencKeyId mKeyId;
    nsRefPtr<nsIRunnable> mContinuation;
    nsCOMPtr<nsIThread> mTarget;
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
