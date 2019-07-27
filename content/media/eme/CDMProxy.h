





#ifndef CDMProxy_h_
#define CDMProxy_h_

#include "nsString.h"
#include "nsAutoPtr.h"
#include "mozilla/dom/MediaKeys.h"
#include "mozilla/Monitor.h"
#include "nsIThread.h"
#include "GMPDecryptorProxy.h"
#include "mozilla/CDMCaps.h"
#include "mp4_demuxer/DecoderData.h"

namespace mozilla {

class CDMCallbackProxy;

namespace dom {
class MediaKeySession;
}

class DecryptionClient {
public:
  virtual ~DecryptionClient() {}
  virtual void Decrypted(nsresult aResult,
                         mp4_demuxer::MP4Sample* aSample) = 0;
};





class CDMProxy {
  typedef dom::PromiseId PromiseId;
  typedef dom::SessionType SessionType;
public:

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(CDMProxy)

  
  CDMProxy(dom::MediaKeys* aKeys, const nsAString& aKeySystem);

  
  
  
  void Init(PromiseId aPromiseId);

  
  
  
  
  void CreateSession(dom::SessionType aSessionType,
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

  
  const nsAString& GetOrigin() const;

  
  void OnResolveNewSessionPromise(uint32_t aPromiseId,
                                  const nsAString& aSessionId);

  
  void OnSessionMessage(const nsAString& aSessionId,
                        nsTArray<uint8_t>& aMessage,
                        const nsAString& aDestinationURL);

  
  void OnExpirationChange(const nsAString& aSessionId,
                          GMPTimestamp aExpiryTime);

  
  void OnSessionClosed(const nsAString& aSessionId);

  
  void OnSessionError(const nsAString& aSessionId,
                      nsresult aException,
                      uint32_t aSystemCode,
                      const nsAString& aMsg);

  
  void OnRejectPromise(uint32_t aPromiseId,
                       nsresult aDOMException,
                       const nsAString& aMsg);

  
  void Decrypt(mp4_demuxer::MP4Sample* aSample,
               DecryptionClient* aSink);

  
  
  void RejectPromise(PromiseId aId, nsresult aExceptionCode);

  
  
  void ResolvePromise(PromiseId aId);

  
  const nsString& KeySystem() const;

  
  void gmp_Decrypted(uint32_t aId,
                     GMPErr aResult,
                     const nsTArray<uint8_t>& aDecryptedData);

  
  void gmp_Terminated();

  CDMCaps& Capabilites();

#ifdef DEBUG
  bool IsOnGMPThread();
#endif

private:

  
  void gmp_Init(uint32_t aPromiseId);

  
  void gmp_Shutdown();

  
  void OnCDMCreated(uint32_t aPromiseId);

  struct CreateSessionData {
    dom::SessionType mSessionType;
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

  struct DecryptJob {
    DecryptJob(mp4_demuxer::MP4Sample* aSample,
               DecryptionClient* aClient)
      : mId(0)
      , mSample(aSample)
      , mClient(aClient)
    {}
    uint32_t mId;
    nsAutoPtr<mp4_demuxer::MP4Sample> mSample;
    nsAutoPtr<DecryptionClient> mClient;
  };
  
  void gmp_Decrypt(nsAutoPtr<DecryptJob> aJob);

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

  nsAutoString mOrigin;

  GMPDecryptorProxy* mCDM;
  CDMCaps mCapabilites;
  nsAutoPtr<CDMCallbackProxy> mCallback;

  
  
  nsTArray<nsAutoPtr<DecryptJob>> mDecryptionJobs;

  
  
  
  
  
  uint32_t mDecryptionJobCount;
};


} 

#endif 
