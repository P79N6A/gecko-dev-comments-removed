







































#ifndef _NSNSSIOLAYER_H
#define _NSNSSIOLAYER_H

#include "prtypes.h"
#include "prio.h"
#include "certt.h"
#include "nsString.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsITransportSecurityInfo.h"
#include "nsISSLSocketControl.h"
#include "nsSSLStatus.h"
#include "nsISSLStatusProvider.h"
#include "nsIIdentityInfo.h"
#include "nsIAssociatedContentSecurity.h"
#include "nsXPIDLString.h"
#include "nsNSSShutDown.h"
#include "nsIClientAuthDialogs.h"
#include "nsAutoPtr.h"
#include "nsNSSCertificate.h"
#include "nsDataHashtable.h"

class nsIChannel;
class nsSSLThread;






class nsSSLSocketThreadData
{
public:
  nsSSLSocketThreadData();
  ~nsSSLSocketThreadData();

  PRBool ensure_buffer_size(PRInt32 amount);
  
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

  PRBool mOneBytePendingFromEarlierWrite;
  unsigned char mThePendingByte;
  PRInt32 mOriginalRequestedTransferAmount;
};

class nsNSSSocketInfo : public nsITransportSecurityInfo,
                        public nsISSLSocketControl,
                        public nsIInterfaceRequestor,
                        public nsISSLStatusProvider,
                        public nsIIdentityInfo,
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
  NS_DECL_NSIIDENTITYINFO
  NS_DECL_NSIASSOCIATEDCONTENTSECURITY
  NS_DECL_NSISERIALIZABLE
  NS_DECL_NSICLASSINFO
  NS_DECL_NSICLIENTAUTHUSERDECISION

  nsresult SetSecurityState(PRUint32 aState);
  nsresult SetShortSecurityDescription(const PRUnichar *aText);
  nsresult SetErrorMessage(const PRUnichar *aText);

  nsresult SetForSTARTTLS(PRBool aForSTARTTLS);
  nsresult GetForSTARTTLS(PRBool *aForSTARTTLS);

  nsresult GetFileDescPtr(PRFileDesc** aFilePtr);
  nsresult SetFileDescPtr(PRFileDesc* aFilePtr);

  nsresult GetHandshakePending(PRBool *aHandshakePending);
  nsresult SetHandshakePending(PRBool aHandshakePending);

  nsresult GetHostName(char **aHostName);
  nsresult SetHostName(const char *aHostName);

  nsresult GetPort(PRInt32 *aPort);
  nsresult SetPort(PRInt32 aPort);

  nsresult GetCert(nsIX509Cert** _result);
  nsresult SetCert(nsIX509Cert *aCert);

  nsresult GetPreviousCert(nsIX509Cert** _result);

  void SetCanceled(PRBool aCanceled);
  PRBool GetCanceled();
  
  void SetHasCleartextPhase(PRBool aHasCleartextPhase);
  PRBool GetHasCleartextPhase();
  
  void SetHandshakeInProgress(PRBool aIsIn);
  PRBool GetHandshakeInProgress() { return mHandshakeInProgress; }
  PRBool HandshakeTimeout();

  void SetAllowTLSIntoleranceTimeout(PRBool aAllow);

  nsresult GetExternalErrorReporting(PRBool* state);
  nsresult SetExternalErrorReporting(PRBool aState);

  nsresult RememberCAChain(CERTCertList *aCertList);

  
  nsresult SetSSLStatus(nsSSLStatus *aSSLStatus);
  nsSSLStatus* SSLStatus() { return mSSLStatus; }
  PRBool hasCertErrors();
  
  PRStatus CloseSocketAndDestroy();
  
protected:
  nsCOMPtr<nsIInterfaceRequestor> mCallbacks;
  PRFileDesc* mFd;
  nsCOMPtr<nsIX509Cert> mCert;
  nsCOMPtr<nsIX509Cert> mPreviousCert; 
  enum { 
    blocking_state_unknown, is_nonblocking_socket, is_blocking_socket 
  } mBlockingState;
  PRUint32 mSecurityState;
  PRInt32 mSubRequestsHighSecurity;
  PRInt32 mSubRequestsLowSecurity;
  PRInt32 mSubRequestsBrokenSecurity;
  PRInt32 mSubRequestsNoSecurity;
  nsString mShortDesc;
  nsString mErrorMessage;
  PRPackedBool mDocShellDependentStuffKnown;
  PRPackedBool mExternalErrorReporting; 
  PRPackedBool mForSTARTTLS;
  PRPackedBool mHandshakePending;
  PRPackedBool mCanceled;
  PRPackedBool mHasCleartextPhase;
  PRPackedBool mHandshakeInProgress;
  PRPackedBool mAllowTLSIntoleranceTimeout;
  PRPackedBool mRememberClientAuthCertificate;
  PRIntervalTime mHandshakeStartTime;
  PRInt32 mPort;
  nsXPIDLCString mHostName;

  
  nsRefPtr<nsSSLStatus> mSSLStatus;

  nsresult ActivateSSL();

  nsSSLSocketThreadData *mThreadData;

  nsresult EnsureDocShellDependentStuffKnown();

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
    PRBool mIsDomainMismatch;
    PRBool mIsNotValidAtThisTime;
    PRBool mIsUntrusted;
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

  static PRBool nsSSLIOLayerInitialized;
  static PRDescIdentity nsSSLIOLayerIdentity;
  static PRIOMethods nsSSLIOLayerMethods;

  static PRLock *mutex;
  static nsCStringHashSet *mTLSIntolerantSites;
  static nsCStringHashSet *mTLSTolerantSites;
  static nsPSMRememberCertErrorsTable* mHostsWithCertErrors;
  
  static void getSiteKey(nsNSSSocketInfo *socketInfo, nsCSubstring &key);
  static PRBool rememberPossibleTLSProblemSite(PRFileDesc* fd, nsNSSSocketInfo *socketInfo);
  static void rememberTolerantSite(PRFileDesc* ssl_layer_fd, nsNSSSocketInfo *socketInfo);

  static void addIntolerantSite(const nsCString &str);
  static void removeIntolerantSite(const nsCString &str);
  static PRBool isKnownAsIntolerantSite(const nsCString &str);
  
  static PRFileDesc *mSharedPollableEvent;
  static nsNSSSocketInfo *mSocketOwningPollableEvent;
  
  static PRBool mPollableEventCurrentlySet;
};

nsresult nsSSLIOLayerNewSocket(PRInt32 family,
                               const char *host,
                               PRInt32 port,
                               const char *proxyHost,
                               PRInt32 proxyPort,
                               PRFileDesc **fd,
                               nsISupports **securityInfo,
                               PRBool forSTARTTLS,
                               PRBool anonymousLoad);

nsresult nsSSLIOLayerAddToSocket(PRInt32 family,
                                 const char *host,
                                 PRInt32 port,
                                 const char *proxyHost,
                                 PRInt32 proxyPort,
                                 PRFileDesc *fd,
                                 nsISupports **securityInfo,
                                 PRBool forSTARTTLS,
                                 PRBool anonymousLoad);

nsresult nsSSLIOLayerFreeTLSIntolerantSites();
nsresult displayUnknownCertErrorAlert(nsNSSSocketInfo *infoObject, int error);


#define NS_NSSSOCKETINFO_CID \
{ 0x16786594, 0x0296, 0x4471, \
    { 0x80, 0x96, 0x8f, 0x84, 0x49, 0x7c, 0xa4, 0x28 } }


#endif 
