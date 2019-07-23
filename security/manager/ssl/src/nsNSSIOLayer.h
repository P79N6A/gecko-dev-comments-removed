







































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
#include "nsISSLStatus.h"
#include "nsISSLStatusProvider.h"
#include "nsXPIDLString.h"
#include "nsNSSShutDown.h"

class nsIChannel;
class nsSSLThread;






class nsSSLSocketThreadData
{
public:
  nsSSLSocketThreadData();
  ~nsSSLSocketThreadData();

  PRBool ensure_buffer_size(PRInt32 amount);
  
  enum ssl_state { 
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
};

class nsNSSSocketInfo : public nsITransportSecurityInfo,
                        public nsISSLSocketControl,
                        public nsIInterfaceRequestor,
                        public nsISSLStatusProvider,
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

  void SetCanceled(PRBool aCanceled);
  PRBool GetCanceled();
  
  void SetHasCleartextPhase(PRBool aHasCleartextPhase);
  PRBool GetHasCleartextPhase();
  
  void SetHandshakeInProgress(PRBool aIsIn);
  PRBool GetHandshakeInProgress() { return mHandshakeInProgress; }
  PRBool HandshakeTimeout();

  void SetAllowTLSIntoleranceTimeout(PRBool aAllow);

  enum BadCertUIStatusType {
    bcuis_not_shown, bcuis_active, bcuis_was_shown
  };

  void SetBadCertUIStatus(BadCertUIStatusType aNewStatus);
  BadCertUIStatusType GetBadCertUIStatus() { return mBadCertUIStatus; }

  nsresult GetExternalErrorReporting(PRBool* state);
  nsresult SetExternalErrorReporting(PRBool aState);

  nsresult RememberCAChain(CERTCertList *aCertList);

  
  nsresult SetSSLStatus(nsISSLStatus *aSSLStatus);  
  
  PRStatus CloseSocketAndDestroy();
  
protected:
  nsCOMPtr<nsIInterfaceRequestor> mCallbacks;
  PRFileDesc* mFd;
  enum { 
    blocking_state_unknown, is_nonblocking_socket, is_blocking_socket 
  } mBlockingState;
  PRUint32 mSecurityState;
  nsString mShortDesc;
  nsString mErrorMessage;
  PRPackedBool mExternalErrorReporting;
  PRPackedBool mForSTARTTLS;
  PRPackedBool mHandshakePending;
  PRPackedBool mCanceled;
  PRPackedBool mHasCleartextPhase;
  PRPackedBool mHandshakeInProgress;
  PRPackedBool mAllowTLSIntoleranceTimeout;
  BadCertUIStatusType mBadCertUIStatus;
  PRIntervalTime mHandshakeStartTime;
  PRInt32 mPort;
  nsXPIDLCString mHostName;
  CERTCertList *mCAChain;

  
  nsCOMPtr<nsISSLStatus> mSSLStatus;

  nsresult ActivateSSL();

  nsSSLSocketThreadData *mThreadData;

private:
  virtual void virtualDestroyNSSReference();
  void destructorSafeDestroyNSSReference();

friend class nsSSLThread;
};

class nsCStringHashSet;

class nsSSLIOLayerHelpers
{
public:
  static nsresult Init();
  static void Cleanup();

  static PRDescIdentity nsSSLIOLayerIdentity;
  static PRIOMethods nsSSLIOLayerMethods;

  static PRLock *mutex;
  static nsCStringHashSet *mTLSIntolerantSites;
  
  static PRBool rememberPossibleTLSProblemSite(PRFileDesc* fd, nsNSSSocketInfo *socketInfo);

  static void addIntolerantSite(const nsCString &str);
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
                               PRBool forSTARTTLS);

nsresult nsSSLIOLayerAddToSocket(PRInt32 family,
                                 const char *host,
                                 PRInt32 port,
                                 const char *proxyHost,
                                 PRInt32 proxyPort,
                                 PRFileDesc *fd,
                                 nsISupports **securityInfo,
                                 PRBool forSTARTTLS);

nsresult nsSSLIOLayerFreeTLSIntolerantSites();
nsresult displayUnknownCertErrorAlert(nsNSSSocketInfo *infoObject, int error);
 
#endif 
