





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

class nsNSSSocketInfo final : public mozilla::psm::TransportSecurityInfo,
                              public nsISSLSocketControl,
                              public nsIClientAuthUserDecision
{
public:
  nsNSSSocketInfo(mozilla::psm::SharedSSLState& aState, uint32_t providerFlags);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSISSLSOCKETCONTROL
  NS_DECL_NSICLIENTAUTHUSERDECISION

  void SetForSTARTTLS(bool aForSTARTTLS);
  bool GetForSTARTTLS();

  nsresult GetFileDescPtr(PRFileDesc** aFilePtr);
  nsresult SetFileDescPtr(PRFileDesc* aFilePtr);

  bool IsHandshakePending() const { return mHandshakePending; }
  void SetHandshakeNotPending() { mHandshakePending = false; }

  void SetTLSVersionRange(SSLVersionRange range) { mTLSVersionRange = range; }
  SSLVersionRange GetTLSVersionRange() const { return mTLSVersionRange; };

  PRStatus CloseSocketAndDestroy(
                const nsNSSShutDownPreventionLock& proofOfLock);

  void SetNegotiatedNPN(const char* value, uint32_t length);

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

  void SetKEAKeyBits(uint32_t keaBits) { mKEAKeyBits = keaBits; }

  void SetBypassAuthentication(bool val)
  {
    if (!mHandshakeCompleted) {
      mBypassAuthentication = val;
    }
  }

  void SetSSLVersionUsed(int16_t version)
  {
    mSSLVersionUsed = version;
  }

  void SetMACAlgorithmUsed(int16_t mac) { mMACAlgorithmUsed = mac; }

  inline bool GetBypassAuthentication()
  {
    bool result = false;
    mozilla::DebugOnly<nsresult> rv = GetBypassAuthentication(&result);
    MOZ_ASSERT(NS_SUCCEEDED(rv));
    return result;
  }

protected:
  virtual ~nsNSSSocketInfo();

private:
  PRFileDesc* mFd;

  CertVerificationState mCertVerificationState;

  mozilla::psm::SharedSSLState& mSharedState;
  bool mForSTARTTLS;
  SSLVersionRange mTLSVersionRange;
  bool mHandshakePending;
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
  bool      mFailedVerification;

  
  
  int16_t mKEAUsed;
  uint32_t mKEAKeyBits;
  int16_t mSSLVersionUsed;
  int16_t mMACAlgorithmUsed;
  bool    mBypassAuthentication;

  uint32_t mProviderFlags;
  mozilla::TimeStamp mSocketCreationTimestamp;
  uint64_t mPlaintextBytesRead;

  nsCOMPtr<nsIX509Cert> mClientCert;
};

enum StrongCipherStatus {
  StrongCipherStatusUnknown,
  StrongCiphersWorked,
  StrongCiphersFailed
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
    PRErrorCode intoleranceReason;
    StrongCipherStatus strongCipherStatus;

    void AssertInvariant() const
    {
      MOZ_ASSERT(intolerant == 0 || tolerant < intolerant);
    }
  };
  nsDataHashtable<nsCStringHashKey, IntoleranceEntry> mTLSIntoleranceInfo;
  
  
  
  nsTHashtable<nsCStringHashKey> mInsecureFallbackSites;
public:
  void rememberTolerantAtVersion(const nsACString& hostname, int16_t port,
                                 uint16_t tolerant);
  bool fallbackLimitReached(const nsACString& hostname, uint16_t intolerant);
  bool rememberIntolerantAtVersion(const nsACString& hostname, int16_t port,
                                   uint16_t intolerant, uint16_t minVersion,
                                   PRErrorCode intoleranceReason);
  bool rememberStrongCiphersFailed(const nsACString& hostName, int16_t port,
                                   PRErrorCode intoleranceReason);
  
  
  uint16_t forgetIntolerance(const nsACString& hostname, int16_t port);
  void adjustForTLSIntolerance(const nsACString& hostname, int16_t port,
                                SSLVersionRange& range,
                                StrongCipherStatus& strongCipherStatus);
  PRErrorCode getIntoleranceReason(const nsACString& hostname, int16_t port);

  void clearStoredData();
  void loadVersionFallbackLimit();
  void setInsecureFallbackSites(const nsCString& str);
  bool isInsecureFallbackSite(const nsACString& hostname);

  bool mFalseStartRequireNPN;
  
  
  
  bool mUseStaticFallbackList;
  bool mUnrestrictedRC4Fallback;
  uint16_t mVersionFallbackLimit;
private:
  mozilla::Mutex mutex;
  nsCOMPtr<nsIObserver> mPrefObserver;
};

nsresult nsSSLIOLayerNewSocket(int32_t family,
                               const char* host,
                               int32_t port,
                               const char* proxyHost,
                               int32_t proxyPort,
                               PRFileDesc** fd,
                               nsISupports** securityInfo,
                               bool forSTARTTLS,
                               uint32_t flags);

nsresult nsSSLIOLayerAddToSocket(int32_t family,
                                 const char* host,
                                 int32_t port,
                                 const char* proxyHost,
                                 int32_t proxyPort,
                                 PRFileDesc* fd,
                                 nsISupports** securityInfo,
                                 bool forSTARTTLS,
                                 uint32_t flags);

nsresult nsSSLIOLayerFreeTLSIntolerantSites();
nsresult displayUnknownCertErrorAlert(nsNSSSocketInfo* infoObject, int error);

#endif 
