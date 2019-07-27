





#ifndef CDMProxy_h_
#define CDMProxy_h_

#include "nsString.h"
#include "nsAutoPtr.h"
#include "mozilla/dom/MediaKeys.h"
#include "mozilla/Monitor.h"
#include "nsIThread.h"
#include "GMPDecryptorProxy.h"
#include "mozilla/CDMCaps.h"
#include "MediaPromise.h"

namespace mozilla {
class MediaRawData;
class CDMCallbackProxy;

namespace dom {
class MediaKeySession;
}

struct DecryptResult {
  DecryptResult(GMPErr aStatus, MediaRawData* aSample)
    : mStatus(aStatus)
    , mSample(aSample)
  {}
  GMPErr mStatus;
  nsRefPtr<MediaRawData> mSample;
};





class CDMProxy {
  typedef dom::PromiseId PromiseId;
  typedef dom::SessionType SessionType;
public:

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(CDMProxy)

  typedef MediaPromise<DecryptResult, DecryptResult,  true> DecryptPromise;

  
  CDMProxy(dom::MediaKeys* aKeys, const nsAString& aKeySystem);

  
  
  
  void Init(PromiseId aPromiseId,
            const nsAString& aOrigin,
            const nsAString& aTopLevelOrigin,
            bool aInPrivateBrowsing);

  
  
  
  
  void CreateSession(uint32_t aCreateSessionToken,
                     dom::SessionType aSessionType,
                     PromiseId aPromiseId,
                     const nsAString& aInitDataType,
                     nsTArray<uint8_t>& aInitData);

  
  
  
  void LoadSession(PromiseId aPromiseId,
                   const nsAString& aSessionId);

  
  
  
  
  
  void SetServerCertificate(PromiseId aPromiseId,
                            nsTArray<uint8_t>& aCert);

  
  
  
  
  
  void UpdateSession(const nsAString& aSessionId,
                     PromiseId aPromiseId,
                     nsTArray<uint8_t>& aResponse);

  
  
  
  
  
  
  void CloseSession(const nsAString& aSessionId,
                    PromiseId aPromiseId);

  
  
  
  
  void RemoveSession(const nsAString& aSessionId,
                     PromiseId aPromiseId);

  
  void Shutdown();

  
  void Terminated();

  
  const nsCString& GetNodeId() const;

  
  void OnSetSessionId(uint32_t aCreateSessionToken,
                      const nsAString& aSessionId);

  
  void OnResolveLoadSessionPromise(uint32_t aPromiseId, bool aSuccess);

  
  void OnSessionMessage(const nsAString& aSessionId,
                        GMPSessionMessageType aMessageType,
                        nsTArray<uint8_t>& aMessage);

  
  void OnExpirationChange(const nsAString& aSessionId,
                          GMPTimestamp aExpiryTime);

  
  void OnSessionClosed(const nsAString& aSessionId);

  
  void OnSessionError(const nsAString& aSessionId,
                      nsresult aException,
                      uint32_t aSystemCode,
                      const nsAString& aMsg);

  
  void OnRejectPromise(uint32_t aPromiseId,
                       nsresult aDOMException,
                       const nsCString& aMsg);

  nsRefPtr<DecryptPromise> Decrypt(MediaRawData* aSample);

  
  
  void RejectPromise(PromiseId aId, nsresult aExceptionCode,
                     const nsCString& aReason);

  
  
  void ResolvePromise(PromiseId aId);

  
  const nsString& KeySystem() const;

  
  void gmp_Decrypted(uint32_t aId,
                     GMPErr aResult,
                     const nsTArray<uint8_t>& aDecryptedData);

  CDMCaps& Capabilites();

  
  void OnKeyStatusesChange(const nsAString& aSessionId);

  void GetSessionIdsForKeyId(const nsTArray<uint8_t>& aKeyId,
                             nsTArray<nsCString>& aSessionIds);

#ifdef DEBUG
  bool IsOnGMPThread();
#endif

private:
  friend class gmp_InitDoneCallback;
  friend class gmp_InitGetGMPDecryptorCallback;

  struct InitData {
    uint32_t mPromiseId;
    nsAutoString mOrigin;
    nsAutoString mTopLevelOrigin;
    bool mInPrivateBrowsing;
  };

  
  void gmp_Init(nsAutoPtr<InitData>&& aData);
  void gmp_InitDone(GMPDecryptorProxy* aCDM, nsAutoPtr<InitData>&& aData);
  void gmp_InitGetGMPDecryptor(nsresult aResult,
                               const nsACString& aNodeId,
                               nsAutoPtr<InitData>&& aData);

  
  void gmp_Shutdown();

  
  void OnCDMCreated(uint32_t aPromiseId);

  struct CreateSessionData {
    dom::SessionType mSessionType;
    uint32_t mCreateSessionToken;
    PromiseId mPromiseId;
    nsAutoCString mInitDataType;
    nsTArray<uint8_t> mInitData;
  };
  
  void gmp_CreateSession(nsAutoPtr<CreateSessionData> aData);

  struct SessionOpData {
    PromiseId mPromiseId;
    nsAutoCString mSessionId;
  };
  
  void gmp_LoadSession(nsAutoPtr<SessionOpData> aData);

  struct SetServerCertificateData {
    PromiseId mPromiseId;
    nsTArray<uint8_t> mCert;
  };
  
  void gmp_SetServerCertificate(nsAutoPtr<SetServerCertificateData> aData);

  struct UpdateSessionData {
    PromiseId mPromiseId;
    nsAutoCString mSessionId;
    nsTArray<uint8_t> mResponse;
  };
  
  void gmp_UpdateSession(nsAutoPtr<UpdateSessionData> aData);

  
  void gmp_CloseSession(nsAutoPtr<SessionOpData> aData);

  
  void gmp_RemoveSession(nsAutoPtr<SessionOpData> aData);

  class DecryptJob {
  public:
    NS_INLINE_DECL_THREADSAFE_REFCOUNTING(DecryptJob)

    explicit DecryptJob(MediaRawData* aSample)
      : mId(0)
      , mSample(aSample)
    {
    }

    void PostResult(GMPErr aResult, const nsTArray<uint8_t>& aDecryptedData);
    void PostResult(GMPErr aResult);

    nsRefPtr<DecryptPromise> Ensure() {
      return mPromise.Ensure(__func__);
    }

    uint32_t mId;
    nsRefPtr<MediaRawData> mSample;
  private:
    ~DecryptJob() {}
    MediaPromiseHolder<DecryptPromise> mPromise;
  };
  
  void gmp_Decrypt(nsRefPtr<DecryptJob> aJob);

  class RejectPromiseTask : public nsRunnable {
  public:
    RejectPromiseTask(CDMProxy* aProxy,
                      PromiseId aId,
                      nsresult aCode,
                      const nsCString& aReason)
      : mProxy(aProxy)
      , mId(aId)
      , mCode(aCode)
      , mReason(aReason)
    {
    }
    NS_METHOD Run() {
      mProxy->RejectPromise(mId, mCode, mReason);
      return NS_OK;
    }
  private:
    nsRefPtr<CDMProxy> mProxy;
    PromiseId mId;
    nsresult mCode;
    nsCString mReason;
  };

  ~CDMProxy();

  
  template<class Type>
  class MainThreadOnlyRawPtr {
  public:
    explicit MainThreadOnlyRawPtr(Type* aPtr)
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

    Type* operator->() const MOZ_NO_ADDREF_RELEASE_ON_RETURN {
      MOZ_ASSERT(NS_IsMainThread());
      return mPtr;
    }
  private:
    Type* mPtr;
  };

  
  
  
  MainThreadOnlyRawPtr<dom::MediaKeys> mKeys;

  const nsAutoString mKeySystem;

  
  
  nsRefPtr<nsIThread> mGMPThread;

  nsCString mNodeId;

  GMPDecryptorProxy* mCDM;
  CDMCaps mCapabilites;
  nsAutoPtr<CDMCallbackProxy> mCallback;

  
  
  nsTArray<nsRefPtr<DecryptJob>> mDecryptionJobs;

  
  
  
  
  
  uint32_t mDecryptionJobCount;

  
  
  bool mShutdownCalled;
};


} 

#endif 
