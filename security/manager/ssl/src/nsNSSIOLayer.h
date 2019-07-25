







































#ifndef _NSNSSIOLAYER_H
#define _NSNSSIOLAYER_H

#include "prtypes.h"
#include "prio.h"
#include "certt.h"
#include "mozilla/Mutex.h"
#include "nsString.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsITransportSecurityInfo.h"
#include "nsISSLSocketControl.h"
#include "nsSSLStatus.h"
#include "nsISSLStatusProvider.h"
#include "nsIAssociatedContentSecurity.h"
#include "nsXPIDLString.h"
#include "nsNSSShutDown.h"
#include "nsIClientAuthDialogs.h"
#include "nsAutoPtr.h"
#include "nsNSSCertificate.h"
#include "nsDataHashtable.h"

class nsIChannel;
class nsSSLThread;
class ::mozilla::MutexAutoLock;






class nsSSLSocketThreadData
{
public:
  nsSSLSocketThreadData();
  ~nsSSLSocketThreadData();

  bool ensure_buffer_size(PRInt32 amount);
  
  enum ssl_state { 
    ssl_invalid,       
    ssl_idle,          
    ssl_pending_write, 
    ssl_pending_read,  
    ssl_writing_done,  
    ssl_reading_done   
  };
  
  ssl_state mSSLState;

  
  
  PRErrorCode mPRErrorCode;

  
  char *mSSLDataBuffer;
  PRInt32 mSSLDataBufferAllocatedSize;

  
  PRInt32 mSSLRequestedTransferAmount;

  
  
  
  
  const char *mSSLRemainingReadResultData;
  
  
  
  
  
  
  
  PRInt32 mSSLResultRemainingBytes;

  
  
  
  
  
  
  
  PRFileDesc *mReplacedSSLFileDesc;

  bool mOneBytePendingFromEarlierWrite;
  unsigned char mThePendingByte;
  PRInt32 mOriginalRequestedTransferAmount;
};

class nsNSSSocketInfo : public nsITransportSecurityInfo,
                        public nsISSLSocketControl,
                        public nsIInterfaceRequestor,
                        public nsISSLStatusProvider,
                        public nsIAssociatedContentSecurity,
                        public nsISerializable,
                        public nsIClassInfo,
                        public nsIClientAuthUserDecision,
                        public nsNSSShutDownObject,
                        public nsOnPK11LogoutCancelObject
{
public:
  nsNSSSocketInfo();
  virtual ~nsNSSSocketInfo();
  
  NS_DECL_ISUPPORTS
  NS_DECL_NSITRANSPORTSECURITYINFO
  NS_DECL_NSISSLSOCKETCONTROL
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSISSLSTATUSPROVIDER
  NS_DECL_NSIASSOCIATEDCONTENTSECURITY
  NS_DECL_NSISERIALIZABLE
  NS_DECL_NSICLASSINFO
  NS_DECL_NSICLIENTAUTHUSERDECISION

  nsresult SetSecurityState(PRUint32 aState);
  nsresult SetShortSecurityDescription(const PRUnichar *aText);

  nsresult SetForSTARTTLS(bool aForSTARTTLS);
  nsresult GetForSTARTTLS(bool *aForSTARTTLS);

  nsresult GetFileDescPtr(PRFileDesc** aFilePtr);
  nsresult SetFileDescPtr(PRFileDesc* aFilePtr);

  nsresult GetHandshakePending(bool *aHandshakePending);
  nsresult SetHandshakePending(bool aHandshakePending);

  nsresult GetHostName(char **aHostName);
  nsresult SetHostName(const char *aHostName);

  nsresult GetPort(PRInt32 *aPort);
  nsresult SetPort(PRInt32 aPort);

  void GetPreviousCert(nsIX509Cert** _result);

  enum ErrorMessageType {
    OverridableCertErrorMessage  = 1, 
    PlainErrorMessage = 2,            
  };
  void SetCanceled(PRErrorCode errorCode, ErrorMessageType errorMessageType);
  PRErrorCode GetErrorCode() const;
  
  void SetHasCleartextPhase(bool aHasCleartextPhase);
  bool GetHasCleartextPhase();
  
  void SetHandshakeInProgress(bool aIsIn);
  bool GetHandshakeInProgress() { return mHandshakeInProgress; }
  bool HandshakeTimeout();

  void SetAllowTLSIntoleranceTimeout(bool aAllow);

  nsresult RememberCAChain(CERTCertList *aCertList);

  
  nsresult SetSSLStatus(nsSSLStatus *aSSLStatus);
  nsSSLStatus* SSLStatus() { return mSSLStatus; }
  
  PRStatus CloseSocketAndDestroy();
  
  bool IsCertIssuerBlacklisted() const {
    return mIsCertIssuerBlacklisted;
  }
  void SetCertIssuerBlacklisted() {
    mIsCertIssuerBlacklisted = true;
  }
protected:
  mutable ::mozilla::Mutex mMutex;

  nsCOMPtr<nsIInterfaceRequestor> mCallbacks;
  PRFileDesc* mFd;
  enum { 
    blocking_state_unknown, is_nonblocking_socket, is_blocking_socket 
  } mBlockingState;
  PRUint32 mSecurityState;
  PRInt32 mSubRequestsHighSecurity;
  PRInt32 mSubRequestsLowSecurity;
  PRInt32 mSubRequestsBrokenSecurity;
  PRInt32 mSubRequestsNoSecurity;
  nsString mShortDesc;

  PRErrorCode mErrorCode;
  ErrorMessageType mErrorMessageType;
  nsString mErrorMessageCached;
  nsresult formatErrorMessage(::mozilla::MutexAutoLock const & proofOfLock);

  bool mDocShellDependentStuffKnown;
  bool mExternalErrorReporting; 
  bool mForSTARTTLS;
  bool mHandshakePending;
  bool mHasCleartextPhase;
  bool mHandshakeInProgress;
  bool mAllowTLSIntoleranceTimeout;
  bool mRememberClientAuthCertificate;
  PRIntervalTime mHandshakeStartTime;
  PRInt32 mPort;
  nsXPIDLCString mHostName;
  PRErrorCode mIsCertIssuerBlacklisted;

  
  nsRefPtr<nsSSLStatus> mSSLStatus;

  nsresult ActivateSSL();

  nsSSLSocketThreadData *mThreadData;

private:
  virtual void virtualDestroyNSSReference();
  void destructorSafeDestroyNSSReference();

friend class nsSSLThread;
};

class nsCStringHashSet;

class nsSSLStatus;
class nsNSSSocketInfo;

class nsPSMRememberCertErrorsTable
{
private:
  struct CertStateBits
  {
    bool mIsDomainMismatch;
    bool mIsNotValidAtThisTime;
    bool mIsUntrusted;
  };
  nsDataHashtableMT<nsCStringHashKey, CertStateBits> mErrorHosts;
  nsresult GetHostPortKey(nsNSSSocketInfo* infoObject, nsCAutoString& result);

public:
  friend class nsSSLIOLayerHelpers;
  nsPSMRememberCertErrorsTable();
  void RememberCertHasError(nsNSSSocketInfo* infoObject,
                           nsSSLStatus* status,
                           SECStatus certVerificationResult);
  void LookupCertErrorBits(nsNSSSocketInfo* infoObject,
                           nsSSLStatus* status);
};

class nsSSLIOLayerHelpers
{
public:
  static nsresult Init();
  static void Cleanup();

  static bool nsSSLIOLayerInitialized;
  static PRDescIdentity nsSSLIOLayerIdentity;
  static PRIOMethods nsSSLIOLayerMethods;

  static mozilla::Mutex *mutex;
  static nsCStringHashSet *mTLSIntolerantSites;
  static nsCStringHashSet *mTLSTolerantSites;
  static nsPSMRememberCertErrorsTable* mHostsWithCertErrors;

  static nsCStringHashSet *mRenegoUnrestrictedSites;
  static bool mTreatUnsafeNegotiationAsBroken;
  static PRInt32 mWarnLevelMissingRFC5746;

  static void setTreatUnsafeNegotiationAsBroken(bool broken);
  static bool treatUnsafeNegotiationAsBroken();

  static void setWarnLevelMissingRFC5746(PRInt32 level);
  static PRInt32 getWarnLevelMissingRFC5746();

  static void getSiteKey(nsNSSSocketInfo *socketInfo, nsCSubstring &key);
  static bool rememberPossibleTLSProblemSite(PRFileDesc* fd, nsNSSSocketInfo *socketInfo);
  static void rememberTolerantSite(PRFileDesc* ssl_layer_fd, nsNSSSocketInfo *socketInfo);

  static void addIntolerantSite(const nsCString &str);
  static void removeIntolerantSite(const nsCString &str);
  static bool isKnownAsIntolerantSite(const nsCString &str);

  static void setRenegoUnrestrictedSites(const nsCString &str);
  static bool isRenegoUnrestrictedSite(const nsCString &str);

  static PRFileDesc *mSharedPollableEvent;
  static nsNSSSocketInfo *mSocketOwningPollableEvent;
  
  static bool mPollableEventCurrentlySet;
};

nsresult nsSSLIOLayerNewSocket(PRInt32 family,
                               const char *host,
                               PRInt32 port,
                               const char *proxyHost,
                               PRInt32 proxyPort,
                               PRFileDesc **fd,
                               nsISupports **securityInfo,
                               bool forSTARTTLS,
                               bool anonymousLoad);

nsresult nsSSLIOLayerAddToSocket(PRInt32 family,
                                 const char *host,
                                 PRInt32 port,
                                 const char *proxyHost,
                                 PRInt32 proxyPort,
                                 PRFileDesc *fd,
                                 nsISupports **securityInfo,
                                 bool forSTARTTLS,
                                 bool anonymousLoad);

nsresult nsSSLIOLayerFreeTLSIntolerantSites();
nsresult displayUnknownCertErrorAlert(nsNSSSocketInfo *infoObject, int error);


#define NS_NSSSOCKETINFO_CID \
{ 0x16786594, 0x0296, 0x4471, \
    { 0x80, 0x96, 0x8f, 0x84, 0x49, 0x7c, 0xa4, 0x28 } }


#endif 
