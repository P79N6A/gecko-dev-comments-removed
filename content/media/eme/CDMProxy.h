





#ifndef CDMProxy_h_
#define CDMProxy_h_

#include "nsString.h"
#include "nsAutoPtr.h"
#include "nsProxyRelease.h"
#include "mozilla/dom/MediaKeys.h"
#include "mozilla/dom/TypedArray.h"

class nsIThread;

namespace mozilla {

namespace dom {
class MediaKeySession;
}








class CDMProxy {
  typedef dom::PromiseId PromiseId;
  typedef dom::SessionType SessionType;
  typedef dom::Uint8Array Uint8Array;
public:

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(CDMProxy)

  
  CDMProxy(dom::MediaKeys* aKeys, const nsAString& aKeySystem);

  
  
  
  void Init(PromiseId aPromiseId);

  
  
  
  
  void CreateSession(dom::SessionType aSessionType,
                     PromiseId aPromiseId,
                     const nsAString& aInitDataType,
                     const Uint8Array& aInitData);

  
  
  
  void LoadSession(PromiseId aPromiseId,
                   const nsAString& aSessionId);

  
  
  
  
  
  void SetServerCertificate(PromiseId aPromiseId,
                            const Uint8Array& aCert);

  
  
  
  
  
  void UpdateSession(const nsAString& aSessionId,
                     PromiseId aPromiseId,
                     const Uint8Array& aResponse);

  
  
  
  
  
  
  void CloseSession(const nsAString& aSessionId,
                    PromiseId aPromiseId);

  
  
  
  
  void RemoveSession(const nsAString& aSessionId,
                     PromiseId aPromiseId);

  
  void Shutdown();

private:

  class RejectPromiseTask : public nsRunnable {
  public:
    RejectPromiseTask(CDMProxy* aProxy,
                      PromiseId aId,
                      nsresult aCode)
      : mProxy(aProxy)
      , mId(aId)
      , mCode(aCode)
    {
    }
    NS_METHOD Run() {
      mProxy->RejectPromise(mId, mCode);
      return NS_OK;
    }
  private:
    nsRefPtr<CDMProxy> mProxy;
    PromiseId mId;
    nsresult mCode;
  };

  
  
  void RejectPromise(PromiseId aId, nsresult aExceptionCode);
  
  
  void ResolvePromise(PromiseId aId);

  ~CDMProxy();

  
  template<class Type>
  class MainThreadOnlyRawPtr {
  public:
    MainThreadOnlyRawPtr(Type* aPtr)
      : mPtr(aPtr)
    {
      MOZ_ASSERT(NS_IsMainThread());
    }

    bool IsNull() const {
      MOZ_ASSERT(NS_IsMainThread());
      return !mPtr;
    }

    void Clear() {
      MOZ_ASSERT(NS_IsMainThread());
      mPtr = nullptr;
    }

    Type* operator->() const {
      MOZ_ASSERT(NS_IsMainThread());
      return mPtr;
    }
  private:
    Type* mPtr;
  };

  
  
  
  MainThreadOnlyRawPtr<dom::MediaKeys> mKeys;

  const nsAutoString mKeySystem;

  
  
  nsRefPtr<nsIThread> mGMPThread;
};

} 

#endif 
