





#ifndef _NSNSSIOLAYER_H
#define _NSNSSIOLAYER_H

#include "TransportSecurityInfo.h"
#include "nsISSLSocketControl.h"
#include "nsIClientAuthDialogs.h"
#include "nsNSSCertificate.h"
#include "nsDataHashtable.h"
#include "nsTHashtable.h"
#include "mozilla/TimeStamp.h"
#include "sslt.h"

namespace mozilla {
namespace psm {
class SharedSSLState;
}
}

class nsIObserver;

class nsNSSSocketInfo : public mozilla::psm::TransportSecurityInfo,
                        public nsISSLSocketControl,
                        public nsIClientAuthUserDecision
{
public:
  nsNSSSocketInfo(mozilla::psm::SharedSSLState& aState, uint32_t providerFlags);
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSISSLSOCKETCONTROL
  NS_DECL_NSICLIENTAUTHUSERDECISION
 
  nsresult SetForSTARTTLS(bool aForSTARTTLS);
  nsresult GetForSTARTTLS(bool *aForSTARTTLS);

  nsresult GetFileDescPtr(PRFileDesc** aFilePtr);
  nsresult SetFileDescPtr(PRFileDesc* aFilePtr);

  bool IsHandshakePending() const { return mHandshakePending; }
  void SetHandshakeNotPending() { mHandshakePending = false; }

  void GetPreviousCert(nsIX509Cert** _result);
  
  void SetHasCleartextPhase(bool aHasCleartextPhase);
  bool GetHasCleartextPhase();
  
  void SetTLSVersionRange(SSLVersionRange range) { mTLSVersionRange = range; }
  SSLVersionRange GetTLSVersionRange() const { return mTLSVersionRange; };

  PRStatus CloseSocketAndDestroy(
                const nsNSSShutDownPreventionLock & proofOfLock);
  
  void SetNegotiatedNPN(const char *value, uint32_t length);

  void SetHandshakeCompleted();
  void NoteTimeUntilReady();


  void SetFalseStartCallbackCalled() { mFalseStartCallbackCalled = true; }
  void SetFalseStarted() { mFalseStarted = true; }

  
  
  void SetFullHandshake() { mIsFullHandshake = true; }
  bool IsFullHandshake() const { return mIsFullHandshake; }

  bool GetJoined() { return mJoined; }
  void SetSentClientCert() { mSentClientCert = true; }

  uint32_t GetProviderFlags() const { return mProviderFlags; }

  mozilla::psm::SharedSSLState& SharedState();

  
  enum CertVerificationState {
    before_cert_verification,
    waiting_for_cert_verification,
    after_cert_verification
  };
  void SetCertVerificationWaiting();
  
  
  void SetCertVerificationResult(PRErrorCode errorCode,
              ::mozilla::psm::SSLErrorMessageType errorMessageType);
  
  
  PRBool IsWaitingForCertVerification() const
  {
    return mCertVerificationState == waiting_for_cert_verification;
  }
  void AddPlaintextBytesRead(uint64_t val) { mPlaintextBytesRead += val; }

  bool IsPreliminaryHandshakeDone() const { return mPreliminaryHandshakeDone; }
  void SetPreliminaryHandshakeDone() { mPreliminaryHandshakeDone = true; }

  void SetKEAUsed(uint16_t kea) { mKEAUsed = kea; }
  inline int16_t GetKEAExpected() 
  {
    int16_t result;
    mozilla::DebugOnly<nsresult> rv = GetKEAExpected(&result);
    MOZ_ASSERT(NS_SUCCEEDED(rv));
    return result;
  }
  void SetSymmetricCipherUsed(uint16_t symmetricCipher)
  {
    mSymmetricCipherUsed = symmetricCipher;
  }
  inline int16_t GetSymmetricCipherExpected() 
  {
    int16_t result;
    mozilla::DebugOnly<nsresult> rv = GetSymmetricCipherExpected(&result);
    MOZ_ASSERT(NS_SUCCEEDED(rv));
    return result;
  }

private:
  PRFileDesc* mFd;

  CertVerificationState mCertVerificationState;

  mozilla::psm::SharedSSLState& mSharedState;
  bool mForSTARTTLS;
  SSLVersionRange mTLSVersionRange;
  bool mHandshakePending;
  bool mHasCleartextPhase;
  bool mRememberClientAuthCertificate;
  bool mPreliminaryHandshakeDone; 

  nsresult ActivateSSL();

  nsCString mNegotiatedNPN;
  bool      mNPNCompleted;
  bool      mFalseStartCallbackCalled;
  bool      mFalseStarted;
  bool      mIsFullHandshake;
  bool      mHandshakeCompleted;
  bool      mJoined;
  bool      mSentClientCert;
  bool      mNotedTimeUntilReady;

  
  
  int16_t mKEAUsed;
  int16_t mKEAExpected;
  int16_t mSymmetricCipherUsed;
  int16_t mSymmetricCipherExpected;

  uint32_t mProviderFlags;
  mozilla::TimeStamp mSocketCreationTimestamp;
  uint64_t mPlaintextBytesRead;
};

class nsSSLIOLayerHelpers
{
public:
  nsSSLIOLayerHelpers();
  ~nsSSLIOLayerHelpers();

  nsresult Init();
  void Cleanup();

  static bool nsSSLIOLayerInitialized;
  static PRDescIdentity nsSSLIOLayerIdentity;
  static PRDescIdentity nsSSLPlaintextLayerIdentity;
  static PRIOMethods nsSSLIOLayerMethods;
  static PRIOMethods nsSSLPlaintextLayerMethods;

  nsTHashtable<nsCStringHashKey> *mRenegoUnrestrictedSites;
  bool mTreatUnsafeNegotiationAsBroken;
  int32_t mWarnLevelMissingRFC5746;

  void setTreatUnsafeNegotiationAsBroken(bool broken);
  bool treatUnsafeNegotiationAsBroken();
  void setWarnLevelMissingRFC5746(int32_t level);
  int32_t getWarnLevelMissingRFC5746();

private:
  struct IntoleranceEntry
  {
    uint16_t tolerant;
    uint16_t intolerant;

    void AssertInvariant() const
    {
      MOZ_ASSERT(intolerant == 0 || tolerant < intolerant);
    }
  };
  nsDataHashtable<nsCStringHashKey, IntoleranceEntry> mTLSIntoleranceInfo;
public:
  void rememberTolerantAtVersion(const nsACString & hostname, int16_t port,
                                 uint16_t tolerant);
  bool rememberIntolerantAtVersion(const nsACString & hostname, int16_t port,
                                   uint16_t intolerant, uint16_t minVersion);
  void adjustForTLSIntolerance(const nsACString & hostname, int16_t port,
                                SSLVersionRange & range);

  void setRenegoUnrestrictedSites(const nsCString &str);
  bool isRenegoUnrestrictedSite(const nsCString &str);
  void clearStoredData();

  bool mFalseStartRequireNPN;
  bool mFalseStartRequireForwardSecrecy;
private:
  mozilla::Mutex mutex;
  nsCOMPtr<nsIObserver> mPrefObserver;
};

nsresult nsSSLIOLayerNewSocket(int32_t family,
                               const char *host,
                               int32_t port,
                               const char *proxyHost,
                               int32_t proxyPort,
                               PRFileDesc **fd,
                               nsISupports **securityInfo,
                               bool forSTARTTLS,
                               uint32_t flags);

nsresult nsSSLIOLayerAddToSocket(int32_t family,
                                 const char *host,
                                 int32_t port,
                                 const char *proxyHost,
                                 int32_t proxyPort,
                                 PRFileDesc *fd,
                                 nsISupports **securityInfo,
                                 bool forSTARTTLS,
                                 uint32_t flags);

nsresult nsSSLIOLayerFreeTLSIntolerantSites();
nsresult displayUnknownCertErrorAlert(nsNSSSocketInfo *infoObject, int error);

#endif 
