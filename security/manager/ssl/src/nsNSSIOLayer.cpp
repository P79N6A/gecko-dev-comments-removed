





#include "nsNSSComponent.h"
#include "nsNSSIOLayer.h"

#include "mozilla/Telemetry.h"

#include "prlog.h"
#include "prnetdb.h"
#include "nsIPrefService.h"
#include "nsIClientAuthDialogs.h"
#include "nsClientAuthRemember.h"
#include "nsISSLErrorListener.h"

#include "nsPrintfCString.h"
#include "SSLServerCertVerification.h"
#include "nsNSSCertHelper.h"
#include "nsNSSCleaner.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsISecureBrowserUI.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsCharSeparatedTokenizer.h"
#include "nsIConsoleService.h"
#include "PSMRunnable.h"
#include "ScopedNSSTypes.h"
#include "SharedSSLState.h"
#include "mozilla/Preferences.h"

#include "ssl.h"
#include "secerr.h"
#include "sslerr.h"
#include "secder.h"
#include "keyhi.h"

using namespace mozilla;
using namespace mozilla::psm;


                            
                            

                       
                       
                       
                       
                       

namespace {

NSSCleanupAutoPtrClass(void, PR_FREEIF)

static NS_DEFINE_CID(kNSSComponentCID, NS_NSSCOMPONENT_CID);


typedef enum {ASK, AUTO} SSM_UserCertChoice;

} 

#ifdef PR_LOGGING
extern PRLogModuleInfo* gPIPNSSLog;
#endif

nsNSSSocketInfo::nsNSSSocketInfo(SharedSSLState& aState, uint32_t providerFlags)
  : mFd(nullptr),
    mCertVerificationState(before_cert_verification),
    mSharedState(aState),
    mForSTARTTLS(false),
    mSSL3Enabled(false),
    mTLSEnabled(false),
    mHandshakePending(true),
    mHasCleartextPhase(false),
    mHandshakeInProgress(false),
    mAllowTLSIntoleranceTimeout(true),
    mRememberClientAuthCertificate(false),
    mHandshakeStartTime(0),
    mFirstServerHelloReceived(false),
    mNPNCompleted(false),
    mHandshakeCompleted(false),
    mJoined(false),
    mSentClientCert(false),
    mProviderFlags(providerFlags),
    mSocketCreationTimestamp(TimeStamp::Now())
{
}

NS_IMPL_ISUPPORTS_INHERITED2(nsNSSSocketInfo, TransportSecurityInfo,
                             nsISSLSocketControl,
                             nsIClientAuthUserDecision)

NS_IMETHODIMP
nsNSSSocketInfo::GetProviderFlags(uint32_t* aProviderFlags)
{
  *aProviderFlags = mProviderFlags;
  return NS_OK;
}

nsresult
nsNSSSocketInfo::GetHandshakePending(bool *aHandshakePending)
{
  *aHandshakePending = mHandshakePending;
  return NS_OK;
}

nsresult
nsNSSSocketInfo::SetHandshakePending(bool aHandshakePending)
{
  mHandshakePending = aHandshakePending;
  return NS_OK;
}

NS_IMETHODIMP nsNSSSocketInfo::GetRememberClientAuthCertificate(bool *aRememberClientAuthCertificate)
{
  NS_ENSURE_ARG_POINTER(aRememberClientAuthCertificate);
  *aRememberClientAuthCertificate = mRememberClientAuthCertificate;
  return NS_OK;
}

NS_IMETHODIMP nsNSSSocketInfo::SetRememberClientAuthCertificate(bool aRememberClientAuthCertificate)
{
  mRememberClientAuthCertificate = aRememberClientAuthCertificate;
  return NS_OK;
}

void nsNSSSocketInfo::SetHasCleartextPhase(bool aHasCleartextPhase)
{
  mHasCleartextPhase = aHasCleartextPhase;
}

bool nsNSSSocketInfo::GetHasCleartextPhase()
{
  return mHasCleartextPhase;
}

NS_IMETHODIMP
nsNSSSocketInfo::GetNotificationCallbacks(nsIInterfaceRequestor** aCallbacks)
{
  *aCallbacks = mCallbacks;
  NS_IF_ADDREF(*aCallbacks);
  return NS_OK;
}

NS_IMETHODIMP
nsNSSSocketInfo::SetNotificationCallbacks(nsIInterfaceRequestor* aCallbacks)
{
  if (!aCallbacks) {
    mCallbacks = nullptr;
    return NS_OK;
  }

  mCallbacks = aCallbacks;

  return NS_OK;
}

static void
getSecureBrowserUI(nsIInterfaceRequestor * callbacks,
                   nsISecureBrowserUI ** result)
{
  NS_ASSERTION(result, "result parameter to getSecureBrowserUI is null");
  *result = nullptr;

  NS_ASSERTION(NS_IsMainThread(),
               "getSecureBrowserUI called off the main thread");

  if (!callbacks)
    return;

  nsCOMPtr<nsISecureBrowserUI> secureUI = do_GetInterface(callbacks);
  if (secureUI) {
    secureUI.forget(result);
    return;
  }

  nsCOMPtr<nsIDocShellTreeItem> item = do_GetInterface(callbacks);
  if (item) {
    nsCOMPtr<nsIDocShellTreeItem> rootItem;
    (void) item->GetSameTypeRootTreeItem(getter_AddRefs(rootItem));
      
    nsCOMPtr<nsIDocShell> docShell = do_QueryInterface(rootItem);
    if (docShell) {
      (void) docShell->GetSecurityUI(result);
    }
  }
}

void
nsNSSSocketInfo::SetHandshakeCompleted(bool aResumedSession)
{
  if (!mHandshakeCompleted) {
    
    Telemetry::AccumulateTimeDelta(Telemetry::SSL_TIME_UNTIL_READY,
                                   mSocketCreationTimestamp, TimeStamp::Now());

    
    
    Telemetry::Accumulate(Telemetry::SSL_RESUMED_SESSION, aResumedSession);
    mHandshakeCompleted = true;
  }
}

void
nsNSSSocketInfo::SetNegotiatedNPN(const char *value, uint32_t length)
{
  if (!value)
    mNegotiatedNPN.Truncate();
  else
    mNegotiatedNPN.Assign(value, length);
  mNPNCompleted = true;
}

NS_IMETHODIMP
nsNSSSocketInfo::GetNegotiatedNPN(nsACString &aNegotiatedNPN)
{
  if (!mNPNCompleted)
    return NS_ERROR_NOT_CONNECTED;

  aNegotiatedNPN = mNegotiatedNPN;
  return NS_OK;
}

NS_IMETHODIMP
nsNSSSocketInfo::JoinConnection(const nsACString & npnProtocol,
                                const nsACString & hostname,
                                int32_t port,
                                bool *_retval)
{
  *_retval = false;

  
  if (port != GetPort())
    return NS_OK;

  
  if (!mNPNCompleted || !mNegotiatedNPN.Equals(npnProtocol))
    return NS_OK;

  
  
  if (GetHostName() && hostname.Equals(GetHostName())) {
    *_retval = true;
    return NS_OK;
  }

  
  
  if (!mHandshakeCompleted || !SSLStatus() || !SSLStatus()->mServerCert)
    return NS_OK;

  
  
  
  if (SSLStatus()->mHaveCertErrorBits)
    return NS_OK;

  
  
  
  if (mSentClientCert)
    return NS_OK;

  
  

  ScopedCERTCertificate nssCert;

  nsCOMPtr<nsIX509Cert2> cert2 = do_QueryInterface(SSLStatus()->mServerCert);
  if (cert2)
    nssCert = cert2->GetCert();

  if (!nssCert)
    return NS_OK;

  if (CERT_VerifyCertName(nssCert, PromiseFlatCString(hostname).get()) !=
      SECSuccess)
    return NS_OK;

  
  mJoined = true;
  *_retval = true;
  return NS_OK;
}

nsresult
nsNSSSocketInfo::GetForSTARTTLS(bool* aForSTARTTLS)
{
  *aForSTARTTLS = mForSTARTTLS;
  return NS_OK;
}

nsresult
nsNSSSocketInfo::SetForSTARTTLS(bool aForSTARTTLS)
{
  mForSTARTTLS = aForSTARTTLS;
  return NS_OK;
}

NS_IMETHODIMP
nsNSSSocketInfo::ProxyStartSSL()
{
  return ActivateSSL();
}

NS_IMETHODIMP
nsNSSSocketInfo::StartTLS()
{
  return ActivateSSL();
}

NS_IMETHODIMP
nsNSSSocketInfo::SetNPNList(nsTArray<nsCString> &protocolArray)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return NS_ERROR_NOT_AVAILABLE;
  if (!mFd)
    return NS_ERROR_FAILURE;

  
  nsCString npnList;

  for (uint32_t index = 0; index < protocolArray.Length(); ++index) {
    if (protocolArray[index].IsEmpty() ||
        protocolArray[index].Length() > 255)
      return NS_ERROR_ILLEGAL_VALUE;

    npnList.Append(protocolArray[index].Length());
    npnList.Append(protocolArray[index]);
  }
  
  if (SSL_SetNextProtoNego(
        mFd,
        reinterpret_cast<const unsigned char *>(npnList.get()),
        npnList.Length()) != SECSuccess)
    return NS_ERROR_FAILURE;

  return NS_OK;
}

nsresult nsNSSSocketInfo::ActivateSSL()
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return NS_ERROR_NOT_AVAILABLE;

  if (SECSuccess != SSL_OptionSet(mFd, SSL_SECURITY, true))
    return NS_ERROR_FAILURE;		
  if (SECSuccess != SSL_ResetHandshake(mFd, false))
    return NS_ERROR_FAILURE;

  mHandshakePending = true;

  return NS_OK;
}

nsresult nsNSSSocketInfo::GetFileDescPtr(PRFileDesc** aFilePtr)
{
  *aFilePtr = mFd;
  return NS_OK;
}

nsresult nsNSSSocketInfo::SetFileDescPtr(PRFileDesc* aFilePtr)
{
  mFd = aFilePtr;
  return NS_OK;
}

class PreviousCertRunnable : public SyncRunnableBase
{
public:
  PreviousCertRunnable(nsIInterfaceRequestor * callbacks)
    : mCallbacks(callbacks)
  {
  }

  virtual void RunOnTargetThread()
  {
    nsCOMPtr<nsISecureBrowserUI> secureUI;
    getSecureBrowserUI(mCallbacks, getter_AddRefs(secureUI));
    nsCOMPtr<nsISSLStatusProvider> statusProvider = do_QueryInterface(secureUI);
    if (statusProvider) {
      nsCOMPtr<nsISSLStatus> status;
      (void) statusProvider->GetSSLStatus(getter_AddRefs(status));
      if (status) {
        (void) status->GetServerCert(getter_AddRefs(mPreviousCert));
      }
    }
  }

  nsCOMPtr<nsIX509Cert> mPreviousCert; 
private:
  nsCOMPtr<nsIInterfaceRequestor> mCallbacks; 
};

void nsNSSSocketInfo::GetPreviousCert(nsIX509Cert** _result)
{
  NS_ASSERTION(_result, "_result parameter to GetPreviousCert is null");
  *_result = nullptr;

  RefPtr<PreviousCertRunnable> runnable(new PreviousCertRunnable(mCallbacks));
  nsresult rv = runnable->DispatchToMainThreadAndWait();
  NS_ASSERTION(NS_SUCCEEDED(rv), "runnable->DispatchToMainThreadAndWait() failed");
  runnable->mPreviousCert.forget(_result);
}

void
nsNSSSocketInfo::SetCertVerificationWaiting()
{
  
  
  
  NS_ASSERTION(mCertVerificationState != waiting_for_cert_verification,
               "Invalid state transition to waiting_for_cert_verification");
  mCertVerificationState = waiting_for_cert_verification;
}





void
nsNSSSocketInfo::SetCertVerificationResult(PRErrorCode errorCode,
                                           SSLErrorMessageType errorMessageType)
{
  NS_ASSERTION(mCertVerificationState == waiting_for_cert_verification,
               "Invalid state transition to cert_verification_finished");

  if (mFd) {
    SECStatus rv = SSL_AuthCertificateComplete(mFd, errorCode);
    
    if (rv != SECSuccess && errorCode == 0) {
      errorCode = PR_GetError();
      errorMessageType = PlainErrorMessage;
      if (errorCode == 0) {
        NS_ERROR("SSL_AuthCertificateComplete didn't set error code");
        errorCode = PR_INVALID_STATE_ERROR;
      }
    }
  }

  if (errorCode) {
    SetCanceled(errorCode, errorMessageType);
  }

  mCertVerificationState = after_cert_verification;
}

void nsNSSSocketInfo::SetHandshakeInProgress(bool aIsIn)
{
  mHandshakeInProgress = aIsIn;

  if (mHandshakeInProgress && !mHandshakeStartTime)
  {
    mHandshakeStartTime = PR_IntervalNow();
  }
}

void nsNSSSocketInfo::SetAllowTLSIntoleranceTimeout(bool aAllow)
{
  mAllowTLSIntoleranceTimeout = aAllow;
}

SharedSSLState& nsNSSSocketInfo::SharedState()
{
  return mSharedState;
}

bool nsNSSSocketInfo::HandshakeTimeout()
{
  if (!mAllowTLSIntoleranceTimeout)
    return false;

  if (!mHandshakeInProgress)
    return false; 

  if (mFirstServerHelloReceived)
    return false;

  
  
  
  
  
  

  static const PRIntervalTime handshakeTimeoutInterval
    = PR_SecondsToInterval(25);

  PRIntervalTime now = PR_IntervalNow();
  bool result = (now - mHandshakeStartTime) > handshakeTimeoutInterval;
  return result;
}

void nsSSLIOLayerHelpers::Cleanup()
{
  if (mTLSIntolerantSites) {
    delete mTLSIntolerantSites;
    mTLSIntolerantSites = nullptr;
  }

  if (mTLSTolerantSites) {
    delete mTLSTolerantSites;
    mTLSTolerantSites = nullptr;
  }

  if (mRenegoUnrestrictedSites) {
    delete mRenegoUnrestrictedSites;
    mRenegoUnrestrictedSites = nullptr;
  }

  if (mutex) {
    delete mutex;
    mutex = nullptr;
  }
}

static void
nsHandleSSLError(nsNSSSocketInfo *socketInfo, 
                 ::mozilla::psm::SSLErrorMessageType errtype, 
                 PRErrorCode err)
{
  if (!NS_IsMainThread()) {
    NS_ERROR("nsHandleSSLError called off the main thread");
    return;
  }

  
  
  
  
  if (socketInfo->GetErrorCode()) {
    
    
    return;
  }

  nsresult rv;
  NS_DEFINE_CID(nssComponentCID, NS_NSSCOMPONENT_CID);
  nsCOMPtr<nsINSSComponent> nssComponent(do_GetService(nssComponentCID, &rv));
  if (NS_FAILED(rv))
    return;

  nsXPIDLCString hostName;
  socketInfo->GetHostName(getter_Copies(hostName));

  int32_t port;
  socketInfo->GetPort(&port);

  
  nsCOMPtr<nsIInterfaceRequestor> cb;
  socketInfo->GetNotificationCallbacks(getter_AddRefs(cb));
  if (cb) {
    nsCOMPtr<nsISSLErrorListener> sel = do_GetInterface(cb);
    if (sel) {
      nsIInterfaceRequestor *csi = static_cast<nsIInterfaceRequestor*>(socketInfo);
      nsCString hostWithPortString = hostName;
      hostWithPortString.AppendLiteral(":");
      hostWithPortString.AppendInt(port);
    
      bool suppressMessage = false; 
      rv = sel->NotifySSLError(csi, err, hostWithPortString, &suppressMessage);
    }
  }
  
  
  socketInfo->SetCanceled(err, PlainErrorMessage);
  nsXPIDLString errorString;
  socketInfo->GetErrorLogMessage(err, errtype, errorString);
  
  if (!errorString.IsEmpty()) {
    nsCOMPtr<nsIConsoleService> console;
    console = do_GetService(NS_CONSOLESERVICE_CONTRACTID);
    if (console) {
      console->LogStringMessage(errorString.get());
    }
  }
}

namespace {

enum Operation { reading, writing, not_reading_or_writing };

int32_t checkHandshake(int32_t bytesTransfered, bool wasReading,
                       PRFileDesc* ssl_layer_fd,
                       nsNSSSocketInfo *socketInfo);

nsNSSSocketInfo *
getSocketInfoIfRunning(PRFileDesc * fd, Operation op,
                       const nsNSSShutDownPreventionLock & )
{
  if (!fd || !fd->lower || !fd->secret ||
      fd->identity != nsSSLIOLayerHelpers::nsSSLIOLayerIdentity) {
    NS_ERROR("bad file descriptor passed to getSocketInfoIfRunning");
    PR_SetError(PR_BAD_DESCRIPTOR_ERROR, 0);
    return nullptr;
  }

  nsNSSSocketInfo *socketInfo = (nsNSSSocketInfo*)fd->secret;

  if (socketInfo->isAlreadyShutDown() || socketInfo->isPK11LoggedOut()) {
    PR_SetError(PR_SOCKET_SHUTDOWN_ERROR, 0);
    return nullptr;
  }

  if (socketInfo->GetErrorCode()) {
    PRErrorCode err = socketInfo->GetErrorCode();
    PR_SetError(err, 0);
    if (op == reading || op == writing) {
      
      
      (void) checkHandshake(-1, op == reading, fd, socketInfo);
    }

    
    
    return nullptr;
  }

  return socketInfo;
}

} 

static PRStatus
nsSSLIOLayerConnect(PRFileDesc* fd, const PRNetAddr* addr,
                    PRIntervalTime timeout)
{
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("[%p] connecting SSL socket\n", (void*)fd));
  nsNSSShutDownPreventionLock locker;
  if (!getSocketInfoIfRunning(fd, not_reading_or_writing, locker))
    return PR_FAILURE;
  
  PRStatus status = fd->lower->methods->connect(fd->lower, addr, timeout);
  if (status != PR_SUCCESS) {
    PR_LOG(gPIPNSSLog, PR_LOG_ERROR, ("[%p] Lower layer connect error: %d\n",
                                      (void*)fd, PR_GetError()));
    return status;
  }

  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("[%p] Connect\n", (void*)fd));
  return status;
}

void
nsSSLIOLayerHelpers::getSiteKey(nsNSSSocketInfo *socketInfo, nsCSubstring &key)
{
  int32_t port;
  socketInfo->GetPort(&port);

  nsXPIDLCString host;
  socketInfo->GetHostName(getter_Copies(host));

  key = host + NS_LITERAL_CSTRING(":") + nsPrintfCString("%d", port);
}





bool
nsSSLIOLayerHelpers::rememberPossibleTLSProblemSite(nsNSSSocketInfo *socketInfo)
{
  nsAutoCString key;
  getSiteKey(socketInfo, key);

  if (!socketInfo->IsTLSEnabled()) {
    
    
    
    
    removeIntolerantSite(key);
    return false;
  }

  if (socketInfo->IsSSL3Enabled()) {
    
    addIntolerantSite(key);
  }
  else {
    return false; 
  }
  
  return socketInfo->IsTLSEnabled();
}

void
nsSSLIOLayerHelpers::rememberTolerantSite(nsNSSSocketInfo *socketInfo)
{
  if (!socketInfo->IsTLSEnabled())
    return;

  nsAutoCString key;
  getSiteKey(socketInfo, key);

  MutexAutoLock lock(*mutex);
  nsSSLIOLayerHelpers::mTLSTolerantSites->PutEntry(key);
}

static PRStatus
nsSSLIOLayerClose(PRFileDesc *fd)
{
  nsNSSShutDownPreventionLock locker;
  if (!fd)
    return PR_FAILURE;

  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("[%p] Shutting down socket\n", (void*)fd));
  
  nsNSSSocketInfo *socketInfo = (nsNSSSocketInfo*)fd->secret;
  NS_ASSERTION(socketInfo,"nsNSSSocketInfo was null for an fd");

  return socketInfo->CloseSocketAndDestroy(locker);
}

PRStatus nsNSSSocketInfo::CloseSocketAndDestroy(
  const nsNSSShutDownPreventionLock & )
{
  nsNSSShutDownList::trackSSLSocketClose();

  PRFileDesc* popped = PR_PopIOLayer(mFd, PR_TOP_IO_LAYER);

  PRStatus status = mFd->methods->close(mFd);
  
  
  
  
  
  mFd = nullptr;
  
  if (status != PR_SUCCESS) return status;

  popped->identity = PR_INVALID_IO_LAYER;
  NS_RELEASE_THIS();
  popped->dtor(popped);

  return PR_SUCCESS;
}

#if defined(DEBUG_SSL_VERBOSE) && defined(DUMP_BUFFER)



#define DUMPBUF_LINESIZE 24
static void
nsDumpBuffer(unsigned char *buf, int len)
{
  char hexbuf[DUMPBUF_LINESIZE*3+1];
  char chrbuf[DUMPBUF_LINESIZE+1];
  static const char *hex = "0123456789abcdef";
  int i = 0, l = 0;
  char ch, *c, *h;
  if (len == 0)
    return;
  hexbuf[DUMPBUF_LINESIZE*3] = '\0';
  chrbuf[DUMPBUF_LINESIZE] = '\0';
  (void) memset(hexbuf, 0x20, DUMPBUF_LINESIZE*3);
  (void) memset(chrbuf, 0x20, DUMPBUF_LINESIZE);
  h = hexbuf;
  c = chrbuf;

  while (i < len)
  {
    ch = buf[i];

    if (l == DUMPBUF_LINESIZE)
    {
      PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("%s%s\n", hexbuf, chrbuf));
      (void) memset(hexbuf, 0x20, DUMPBUF_LINESIZE*3);
      (void) memset(chrbuf, 0x20, DUMPBUF_LINESIZE);
      h = hexbuf;
      c = chrbuf;
      l = 0;
    }

    
    *h++ = hex[(ch >> 4) & 0xf];
    *h++ = hex[ch & 0xf];
    h++;
        
    
    if ((ch >= 0x20) && (ch <= 0x7e))
      *c++ = ch;
    else
      *c++ = '.';
    i++; l++;
  }
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("%s%s\n", hexbuf, chrbuf));
}

#define DEBUG_DUMP_BUFFER(buf,len) nsDumpBuffer(buf,len)
#else
#define DEBUG_DUMP_BUFFER(buf,len)
#endif

static bool
isNonSSLErrorThatWeAllowToRetry(int32_t err, bool withInitialCleartext)
{
  switch (err)
  {
    case PR_CONNECT_RESET_ERROR:
      if (!withInitialCleartext)
        return true;
      break;
    
    case PR_END_OF_FILE_ERROR:
      return true;
  }

  return false;
}

static bool
isTLSIntoleranceError(int32_t err, bool withInitialCleartext)
{
  
  
  
  
  
  
  
  

  if (isNonSSLErrorThatWeAllowToRetry(err, withInitialCleartext))
    return true;

  switch (err)
  {
    case SSL_ERROR_BAD_MAC_ALERT:
    case SSL_ERROR_BAD_MAC_READ:
    case SSL_ERROR_HANDSHAKE_FAILURE_ALERT:
    case SSL_ERROR_HANDSHAKE_UNEXPECTED_ALERT:
    case SSL_ERROR_CLIENT_KEY_EXCHANGE_FAILURE:
    case SSL_ERROR_ILLEGAL_PARAMETER_ALERT:
    case SSL_ERROR_NO_CYPHER_OVERLAP:
    case SSL_ERROR_BAD_SERVER:
    case SSL_ERROR_BAD_BLOCK_PADDING:
    case SSL_ERROR_UNSUPPORTED_VERSION:
    case SSL_ERROR_PROTOCOL_VERSION_ALERT:
    case SSL_ERROR_RX_MALFORMED_FINISHED:
    case SSL_ERROR_BAD_HANDSHAKE_HASH_VALUE:
    case SSL_ERROR_DECODE_ERROR_ALERT:
    case SSL_ERROR_RX_UNKNOWN_ALERT:
      return true;
  }
  
  return false;
}

class SSLErrorRunnable : public SyncRunnableBase
{
 public:
  SSLErrorRunnable(nsNSSSocketInfo * infoObject, 
                   ::mozilla::psm::SSLErrorMessageType errtype, 
                   PRErrorCode errorCode)
    : mInfoObject(infoObject)
    , mErrType(errtype)
    , mErrorCode(errorCode)
  {
  }

  virtual void RunOnTargetThread()
  {
    nsHandleSSLError(mInfoObject, mErrType, mErrorCode);
  }
  
  RefPtr<nsNSSSocketInfo> mInfoObject;
  ::mozilla::psm::SSLErrorMessageType mErrType;
  const PRErrorCode mErrorCode;
};

namespace {

int32_t checkHandshake(int32_t bytesTransfered, bool wasReading,
                       PRFileDesc* ssl_layer_fd,
                       nsNSSSocketInfo *socketInfo)
{
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  
  
  
  
  
  
  
  
  

  
  
  
  

  bool handleHandshakeResultNow;
  socketInfo->GetHandshakePending(&handleHandshakeResultNow);

  bool wantRetry = false;

  if (0 > bytesTransfered) {
    int32_t err = PR_GetError();

    if (handleHandshakeResultNow) {
      if (PR_WOULD_BLOCK_ERROR == err) {
        socketInfo->SetHandshakeInProgress(true);
        return bytesTransfered;
      }

      if (!wantRetry 
          && isTLSIntoleranceError(err, socketInfo->GetHasCleartextPhase()))
      {
        nsSSLIOLayerHelpers& helpers = socketInfo->SharedState().IOLayerHelpers();
        wantRetry = helpers.rememberPossibleTLSProblemSite(socketInfo);
      }
    }
    
    
    
    
    
    
    
    
    
    
    if (!wantRetry && (IS_SSL_ERROR(err) || IS_SEC_ERROR(err)) &&
        !socketInfo->GetErrorCode()) {
      RefPtr<SyncRunnableBase> runnable(new SSLErrorRunnable(socketInfo,
                                                             PlainErrorMessage,
                                                             err));
      (void) runnable->DispatchToMainThreadAndWait();
    }
  }
  else if (wasReading && 0 == bytesTransfered) 
  {
    if (handleHandshakeResultNow)
    {
      if (!wantRetry 
          && !socketInfo->GetHasCleartextPhase()) 
      {
        nsSSLIOLayerHelpers& helpers = socketInfo->SharedState().IOLayerHelpers();
        wantRetry = helpers.rememberPossibleTLSProblemSite(socketInfo);
      }
    }
  }

  if (wantRetry) {
    
    PR_SetError(PR_CONNECT_RESET_ERROR, 0);
    if (wasReading)
      bytesTransfered = -1;
  }

  
  
  
  if (handleHandshakeResultNow) {
    socketInfo->SetHandshakePending(false);
    socketInfo->SetHandshakeInProgress(false);
  }
  
  return bytesTransfered;
}

}

static int16_t
nsSSLIOLayerPoll(PRFileDesc * fd, int16_t in_flags, int16_t *out_flags)
{
  nsNSSShutDownPreventionLock locker;

  if (!out_flags)
  {
    NS_WARNING("nsSSLIOLayerPoll called with null out_flags");
    return 0;
  }

  *out_flags = 0;

  nsNSSSocketInfo * socketInfo =
    getSocketInfoIfRunning(fd, not_reading_or_writing, locker);

  if (!socketInfo) {
    
    
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
            ("[%p] polling SSL socket right after certificate verification failed "
                  "or NSS shutdown or SDR logout %d\n",
             fd, (int) in_flags));

    NS_ASSERTION(in_flags & PR_POLL_EXCEPT,
                 "caller did not poll for EXCEPT (canceled)");
    
    
    
    *out_flags = in_flags | PR_POLL_EXCEPT; 
    return in_flags;
  }

  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
         (socketInfo->IsWaitingForCertVerification()
            ?  "[%p] polling SSL socket during certificate verification using lower %d\n"
            :  "[%p] poll SSL socket using lower %d\n",
         fd, (int) in_flags));

  
  if (socketInfo->HandshakeTimeout()) {
    NS_WARNING("SSL handshake timed out");
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("[%p] handshake timed out\n", fd));
    NS_ASSERTION(in_flags & PR_POLL_EXCEPT,
                 "caller did not poll for EXCEPT (handshake timeout)");
    *out_flags = in_flags | PR_POLL_EXCEPT;
    socketInfo->SetCanceled(PR_CONNECT_RESET_ERROR, PlainErrorMessage);
    return in_flags;
  }

  
  
  
  
  int16_t result = fd->lower->methods->poll(fd->lower, in_flags, out_flags);
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("[%p] poll SSL socket returned %d\n",
                                    (void*)fd, (int) result));
  return result;
}

bool nsSSLIOLayerHelpers::nsSSLIOLayerInitialized = false;
PRDescIdentity nsSSLIOLayerHelpers::nsSSLIOLayerIdentity;
PRIOMethods nsSSLIOLayerHelpers::nsSSLIOLayerMethods;

nsSSLIOLayerHelpers::nsSSLIOLayerHelpers()
: mutex(nullptr)
, mTLSIntolerantSites(nullptr)
, mTLSTolerantSites(nullptr)
, mRenegoUnrestrictedSites(nullptr)
, mTreatUnsafeNegotiationAsBroken(false)
, mWarnLevelMissingRFC5746(1)
{
}

static int _PSM_InvalidInt(void)
{
    PR_ASSERT(!"I/O method is invalid");
    PR_SetError(PR_INVALID_METHOD_ERROR, 0);
    return -1;
}

static int64_t _PSM_InvalidInt64(void)
{
    PR_ASSERT(!"I/O method is invalid");
    PR_SetError(PR_INVALID_METHOD_ERROR, 0);
    return -1;
}

static PRStatus _PSM_InvalidStatus(void)
{
    PR_ASSERT(!"I/O method is invalid");
    PR_SetError(PR_INVALID_METHOD_ERROR, 0);
    return PR_FAILURE;
}

static PRFileDesc *_PSM_InvalidDesc(void)
{
    PR_ASSERT(!"I/O method is invalid");
    PR_SetError(PR_INVALID_METHOD_ERROR, 0);
    return nullptr;
}

static PRStatus PSMGetsockname(PRFileDesc *fd, PRNetAddr *addr)
{
  nsNSSShutDownPreventionLock locker;
  if (!getSocketInfoIfRunning(fd, not_reading_or_writing, locker))
    return PR_FAILURE;

  return fd->lower->methods->getsockname(fd->lower, addr);
}

static PRStatus PSMGetpeername(PRFileDesc *fd, PRNetAddr *addr)
{
  nsNSSShutDownPreventionLock locker;
  if (!getSocketInfoIfRunning(fd, not_reading_or_writing, locker))
    return PR_FAILURE;

  return fd->lower->methods->getpeername(fd->lower, addr);
}

static PRStatus PSMGetsocketoption(PRFileDesc *fd, PRSocketOptionData *data)
{
  nsNSSShutDownPreventionLock locker;
  if (!getSocketInfoIfRunning(fd, not_reading_or_writing, locker))
    return PR_FAILURE;

  return fd->lower->methods->getsocketoption(fd, data);
}

static PRStatus PSMSetsocketoption(PRFileDesc *fd,
                                   const PRSocketOptionData *data)
{
  nsNSSShutDownPreventionLock locker;
  if (!getSocketInfoIfRunning(fd, not_reading_or_writing, locker))
    return PR_FAILURE;

  return fd->lower->methods->setsocketoption(fd, data);
}

static int32_t PSMRecv(PRFileDesc *fd, void *buf, int32_t amount,
    int flags, PRIntervalTime timeout)
{
  nsNSSShutDownPreventionLock locker;
  nsNSSSocketInfo *socketInfo = getSocketInfoIfRunning(fd, reading, locker);
  if (!socketInfo)
    return -1;

  if (flags != PR_MSG_PEEK && flags != 0) {
    PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
    return -1;
  }

  int32_t bytesRead = fd->lower->methods->recv(fd->lower, buf, amount, flags,
                                               timeout);

  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("[%p] read %d bytes\n", (void*)fd, bytesRead));

#ifdef DEBUG_SSL_VERBOSE
  DEBUG_DUMP_BUFFER((unsigned char*)buf, bytesRead);
#endif

  return checkHandshake(bytesRead, true, fd, socketInfo);
}

static int32_t PSMSend(PRFileDesc *fd, const void *buf, int32_t amount,
    int flags, PRIntervalTime timeout)
{
  nsNSSShutDownPreventionLock locker;
  nsNSSSocketInfo *socketInfo = getSocketInfoIfRunning(fd, writing, locker);
  if (!socketInfo)
    return -1;

  if (flags != 0) {
    PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
    return -1;
  }

#ifdef DEBUG_SSL_VERBOSE
  DEBUG_DUMP_BUFFER((unsigned char*)buf, amount);
#endif

  int32_t bytesWritten = fd->lower->methods->send(fd->lower, buf, amount,
                                                  flags, timeout);

  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("[%p] wrote %d bytes\n",
         fd, bytesWritten));

  return checkHandshake(bytesWritten, false, fd, socketInfo);
}

static int32_t
nsSSLIOLayerRead(PRFileDesc* fd, void* buf, int32_t amount)
{
  return PSMRecv(fd, buf, amount, 0, PR_INTERVAL_NO_TIMEOUT);
}

static int32_t
nsSSLIOLayerWrite(PRFileDesc* fd, const void* buf, int32_t amount)
{
  return PSMSend(fd, buf, amount, 0, PR_INTERVAL_NO_TIMEOUT);
}

static PRStatus PSMConnectcontinue(PRFileDesc *fd, int16_t out_flags)
{
  nsNSSShutDownPreventionLock locker;
  if (!getSocketInfoIfRunning(fd, not_reading_or_writing, locker)) {
    return PR_FAILURE;
  }

  return fd->lower->methods->connectcontinue(fd, out_flags);
}

static int PSMAvailable(void)
{
  
  PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
  return -1;
}

static int64_t PSMAvailable64(void)
{
  
  PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
  return -1;
}

namespace {
class PrefObserver : public nsIObserver {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  PrefObserver(nsSSLIOLayerHelpers* aOwner) : mOwner(aOwner) {}
  virtual ~PrefObserver() {}
private:
  nsSSLIOLayerHelpers* mOwner;
};
} 

NS_IMPL_THREADSAFE_ISUPPORTS1(PrefObserver, nsIObserver)

NS_IMETHODIMP
PrefObserver::Observe(nsISupports *aSubject, const char *aTopic, 
                      const PRUnichar *someData)
{
  if (nsCRT::strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID) == 0) {
    NS_ConvertUTF16toUTF8 prefName(someData);

    if (prefName.Equals("security.ssl.renego_unrestricted_hosts")) {
      nsCString unrestricted_hosts;
      Preferences::GetCString("security.ssl.renego_unrestricted_hosts", &unrestricted_hosts);
      if (!unrestricted_hosts.IsEmpty()) {
        mOwner->setRenegoUnrestrictedSites(unrestricted_hosts);
      }
    } else if (prefName.Equals("security.ssl.treat_unsafe_negotiation_as_broken")) {
      bool enabled;
      Preferences::GetBool("security.ssl.treat_unsafe_negotiation_as_broken", &enabled);
      mOwner->setTreatUnsafeNegotiationAsBroken(enabled);
    } else if (prefName.Equals("security.ssl.warn_missing_rfc5746")) {
      int32_t warnLevel = 1;
      Preferences::GetInt("security.ssl.warn_missing_rfc5746", &warnLevel);
      mOwner->setWarnLevelMissingRFC5746(warnLevel);
    }
  }
  return NS_OK;
}

nsSSLIOLayerHelpers::~nsSSLIOLayerHelpers()
{
  Preferences::RemoveObserver(mPrefObserver, "security.ssl.renego_unrestricted_hosts");
  Preferences::RemoveObserver(mPrefObserver, "security.ssl.treat_unsafe_negotiation_as_broken");
  Preferences::RemoveObserver(mPrefObserver, "security.ssl.warn_missing_rfc5746");
}

nsresult nsSSLIOLayerHelpers::Init()
{
  if (!nsSSLIOLayerInitialized) {
    nsSSLIOLayerInitialized = true;
    nsSSLIOLayerIdentity = PR_GetUniqueIdentity("NSS layer");
    nsSSLIOLayerMethods  = *PR_GetDefaultIOMethods();

    nsSSLIOLayerMethods.available = (PRAvailableFN)PSMAvailable;
    nsSSLIOLayerMethods.available64 = (PRAvailable64FN)PSMAvailable64;
    nsSSLIOLayerMethods.fsync = (PRFsyncFN)_PSM_InvalidStatus;
    nsSSLIOLayerMethods.seek = (PRSeekFN)_PSM_InvalidInt;
    nsSSLIOLayerMethods.seek64 = (PRSeek64FN)_PSM_InvalidInt64;
    nsSSLIOLayerMethods.fileInfo = (PRFileInfoFN)_PSM_InvalidStatus;
    nsSSLIOLayerMethods.fileInfo64 = (PRFileInfo64FN)_PSM_InvalidStatus;
    nsSSLIOLayerMethods.writev = (PRWritevFN)_PSM_InvalidInt;
    nsSSLIOLayerMethods.accept = (PRAcceptFN)_PSM_InvalidDesc;
    nsSSLIOLayerMethods.bind = (PRBindFN)_PSM_InvalidStatus;
    nsSSLIOLayerMethods.listen = (PRListenFN)_PSM_InvalidStatus;
    nsSSLIOLayerMethods.shutdown = (PRShutdownFN)_PSM_InvalidStatus;
    nsSSLIOLayerMethods.recvfrom = (PRRecvfromFN)_PSM_InvalidInt;
    nsSSLIOLayerMethods.sendto = (PRSendtoFN)_PSM_InvalidInt;
    nsSSLIOLayerMethods.acceptread = (PRAcceptreadFN)_PSM_InvalidInt;
    nsSSLIOLayerMethods.transmitfile = (PRTransmitfileFN)_PSM_InvalidInt;
    nsSSLIOLayerMethods.sendfile = (PRSendfileFN)_PSM_InvalidInt;

    nsSSLIOLayerMethods.getsockname = PSMGetsockname;
    nsSSLIOLayerMethods.getpeername = PSMGetpeername;
    nsSSLIOLayerMethods.getsocketoption = PSMGetsocketoption;
    nsSSLIOLayerMethods.setsocketoption = PSMSetsocketoption;
    nsSSLIOLayerMethods.recv = PSMRecv;
    nsSSLIOLayerMethods.send = PSMSend;
    nsSSLIOLayerMethods.connectcontinue = PSMConnectcontinue;

    nsSSLIOLayerMethods.connect = nsSSLIOLayerConnect;
    nsSSLIOLayerMethods.close = nsSSLIOLayerClose;
    nsSSLIOLayerMethods.write = nsSSLIOLayerWrite;
    nsSSLIOLayerMethods.read = nsSSLIOLayerRead;
    nsSSLIOLayerMethods.poll = nsSSLIOLayerPoll;
  }

  mutex = new Mutex("nsSSLIOLayerHelpers.mutex");

  mTLSIntolerantSites = new nsTHashtable<nsCStringHashKey>();
  mTLSIntolerantSites->Init(1);

  mTLSTolerantSites = new nsTHashtable<nsCStringHashKey>();
  
  
  
  mTLSTolerantSites->Init(16);

  mRenegoUnrestrictedSites = new nsTHashtable<nsCStringHashKey>();
  mRenegoUnrestrictedSites->Init(1);

  nsCString unrestricted_hosts;
  Preferences::GetCString("security.ssl.renego_unrestricted_hosts", &unrestricted_hosts);
  if (!unrestricted_hosts.IsEmpty()) {
    setRenegoUnrestrictedSites(unrestricted_hosts);
  }

  bool enabled = false;
  Preferences::GetBool("security.ssl.treat_unsafe_negotiation_as_broken", &enabled);
  setTreatUnsafeNegotiationAsBroken(enabled);

  int32_t warnLevel = 1;
  Preferences::GetInt("security.ssl.warn_missing_rfc5746", &warnLevel);
  setWarnLevelMissingRFC5746(warnLevel);

  mPrefObserver = new PrefObserver(this);
  Preferences::AddStrongObserver(mPrefObserver,
                                 "security.ssl.renego_unrestricted_hosts");
  Preferences::AddStrongObserver(mPrefObserver,
                                 "security.ssl.treat_unsafe_negotiation_as_broken");
  Preferences::AddStrongObserver(mPrefObserver,
                                 "security.ssl.warn_missing_rfc5746");

  return NS_OK;
}

void nsSSLIOLayerHelpers::clearStoredData()
{
  mRenegoUnrestrictedSites->Clear();
  mTLSTolerantSites->Clear();
  mTLSIntolerantSites->Clear();
}

void nsSSLIOLayerHelpers::addIntolerantSite(const nsCString &str)
{
  MutexAutoLock lock(*mutex);
  
  if (!mTLSTolerantSites->Contains(str))
    mTLSIntolerantSites->PutEntry(str);
}

void nsSSLIOLayerHelpers::removeIntolerantSite(const nsCString &str)
{
  MutexAutoLock lock(*mutex);
  mTLSIntolerantSites->RemoveEntry(str);
}

bool nsSSLIOLayerHelpers::isKnownAsIntolerantSite(const nsCString &str)
{
  MutexAutoLock lock(*mutex);
  return mTLSIntolerantSites->Contains(str);
}

void nsSSLIOLayerHelpers::setRenegoUnrestrictedSites(const nsCString &str)
{
  MutexAutoLock lock(*mutex);
  
  if (mRenegoUnrestrictedSites) {
    delete mRenegoUnrestrictedSites;
    mRenegoUnrestrictedSites = nullptr;
  }

  mRenegoUnrestrictedSites = new nsTHashtable<nsCStringHashKey>();
  if (!mRenegoUnrestrictedSites)
    return;
  
  mRenegoUnrestrictedSites->Init(1);
  
  nsCCharSeparatedTokenizer toker(str, ',');

  while (toker.hasMoreTokens()) {
    const nsCSubstring &host = toker.nextToken();
    if (!host.IsEmpty()) {
      mRenegoUnrestrictedSites->PutEntry(host);
    }
  }
}

bool nsSSLIOLayerHelpers::isRenegoUnrestrictedSite(const nsCString &str)
{
  MutexAutoLock lock(*mutex);
  return mRenegoUnrestrictedSites->Contains(str);
}

void nsSSLIOLayerHelpers::setTreatUnsafeNegotiationAsBroken(bool broken)
{
  MutexAutoLock lock(*mutex);
  mTreatUnsafeNegotiationAsBroken = broken;
}

bool nsSSLIOLayerHelpers::treatUnsafeNegotiationAsBroken()
{
  MutexAutoLock lock(*mutex);
  return mTreatUnsafeNegotiationAsBroken;
}

void nsSSLIOLayerHelpers::setWarnLevelMissingRFC5746(int32_t level)
{
  MutexAutoLock lock(*mutex);
  mWarnLevelMissingRFC5746 = level;
}

int32_t nsSSLIOLayerHelpers::getWarnLevelMissingRFC5746()
{
  MutexAutoLock lock(*mutex);
  return mWarnLevelMissingRFC5746;
}

nsresult
nsSSLIOLayerNewSocket(int32_t family,
                      const char *host,
                      int32_t port,
                      const char *proxyHost,
                      int32_t proxyPort,
                      PRFileDesc **fd,
                      nsISupports** info,
                      bool forSTARTTLS,
                      uint32_t flags)
{

  PRFileDesc* sock = PR_OpenTCPSocket(family);
  if (!sock) return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv = nsSSLIOLayerAddToSocket(family, host, port, proxyHost, proxyPort,
                                        sock, info, forSTARTTLS, flags);
  if (NS_FAILED(rv)) {
    PR_Close(sock);
    return rv;
  }

  *fd = sock;
  return NS_OK;
}













SECStatus nsConvertCANamesToStrings(PLArenaPool* arena, char** caNameStrings,
                                      CERTDistNames* caNames)
{
    SECItem* dername;
    SECStatus rv;
    int headerlen;
    uint32_t contentlen;
    SECItem newitem;
    int n;
    char* namestring;

    for (n = 0; n < caNames->nnames; n++) {
        newitem.data = nullptr;
        dername = &caNames->names[n];

        rv = DER_Lengths(dername, &headerlen, &contentlen);

        if (rv != SECSuccess) {
            goto loser;
        }

        if (headerlen + contentlen != dername->len) {
            




            if (dername->len <= 127) {
                newitem.data = (unsigned char *) PR_Malloc(dername->len + 2);
                if (!newitem.data) {
                    goto loser;
                }
                newitem.data[0] = (unsigned char)0x30;
                newitem.data[1] = (unsigned char)dername->len;
                (void)memcpy(&newitem.data[2], dername->data, dername->len);
            }
            else if (dername->len <= 255) {
                newitem.data = (unsigned char *) PR_Malloc(dername->len + 3);
                if (!newitem.data) {
                    goto loser;
                }
                newitem.data[0] = (unsigned char)0x30;
                newitem.data[1] = (unsigned char)0x81;
                newitem.data[2] = (unsigned char)dername->len;
                (void)memcpy(&newitem.data[3], dername->data, dername->len);
            }
            else {
                
                newitem.data = (unsigned char *) PR_Malloc(dername->len + 4);
                if (!newitem.data) {
                    goto loser;
                }
                newitem.data[0] = (unsigned char)0x30;
                newitem.data[1] = (unsigned char)0x82;
                newitem.data[2] = (unsigned char)((dername->len >> 8) & 0xff);
                newitem.data[3] = (unsigned char)(dername->len & 0xff);
                memcpy(&newitem.data[4], dername->data, dername->len);
            }
            dername = &newitem;
        }

        namestring = CERT_DerNameToAscii(dername);
        if (!namestring) {
            
            caNameStrings[n] = const_cast<char*>("");
        }
        else {
            caNameStrings[n] = PORT_ArenaStrdup(arena, namestring);
            PR_Free(namestring);
            if (!caNameStrings[n]) {
                goto loser;
            }
        }

        if (newitem.data) {
            PR_Free(newitem.data);
        }
    }

    return SECSuccess;
loser:
    if (newitem.data) {
        PR_Free(newitem.data);
    }
    return SECFailure;
}















typedef struct {
    SECItem derConstraint;
    SECItem derPort;
    CERTGeneralName* constraint; 
    int port; 
} CERTCertificateScopeEntry;

typedef struct {
    CERTCertificateScopeEntry** entries;
} certCertificateScopeOfUse;


static const SEC_ASN1Template cert_CertificateScopeEntryTemplate[] = {
    { SEC_ASN1_SEQUENCE, 
      0, nullptr, sizeof(CERTCertificateScopeEntry) },
    { SEC_ASN1_ANY,
      offsetof(CERTCertificateScopeEntry, derConstraint) },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_INTEGER,
      offsetof(CERTCertificateScopeEntry, derPort) },
    { 0 }
};

static const SEC_ASN1Template cert_CertificateScopeOfUseTemplate[] = {
    { SEC_ASN1_SEQUENCE_OF, 0, cert_CertificateScopeEntryTemplate }
};

#if 0




static
SECStatus cert_DecodeScopeOfUseEntries(PRArenaPool* arena, SECItem* extData,
                                       CERTCertificateScopeEntry*** entries,
                                       int* numEntries)
{
    certCertificateScopeOfUse* scope = nullptr;
    SECStatus rv = SECSuccess;
    int i;

    *entries = nullptr; 
    *numEntries = 0; 

    scope = (certCertificateScopeOfUse*)
        PORT_ArenaZAlloc(arena, sizeof(certCertificateScopeOfUse));
    if (!scope) {
        goto loser;
    }

    rv = SEC_ASN1DecodeItem(arena, (void*)scope, 
                            cert_CertificateScopeOfUseTemplate, extData);
    if (rv != SECSuccess) {
        goto loser;
    }

    *entries = scope->entries;
    PR_ASSERT(*entries);

    
    for (i = 0; (*entries)[i]; i++) ;
    *numEntries = i;

    


    for (i = 0; i < *numEntries; i++) {
        (*entries)[i]->constraint = 
            CERT_DecodeGeneralName(arena, &((*entries)[i]->derConstraint), 
                                   nullptr);
        if ((*entries)[i]->derPort.data) {
            (*entries)[i]->port = 
                (int)DER_GetInteger(&((*entries)[i]->derPort));
        }
        else {
            (*entries)[i]->port = 0;
        }
    }

    goto done;
loser:
    if (rv == SECSuccess) {
        rv = SECFailure;
    }
done:
    return rv;
}

static SECStatus cert_DecodeCertIPAddress(SECItem* genname, 
                                          uint32_t* constraint, uint32_t* mask)
{
    
    *constraint = 0;
    *mask = 0;

    PR_ASSERT(genname->data);
    if (!genname->data) {
        return SECFailure;
    }
    if (genname->len != 8) {
        
        return SECFailure;
    }

    
    *constraint = PR_ntohl((uint32_t)(*genname->data));
    *mask = PR_ntohl((uint32_t)(*(genname->data + 4)));

    return SECSuccess;
}

static char* _str_to_lower(char* string)
{
#ifdef XP_WIN
    return _strlwr(string);
#else
    int i;
    for (i = 0; string[i] != '\0'; i++) {
        string[i] = tolower(string[i]);
    }
    return string;
#endif
}







static bool CERT_MatchesScopeOfUse(CERTCertificate* cert, char* hostname,
                                     char* hostIP, int port)
{
    bool rv = true; 
    SECStatus srv;
    SECItem extData;
    PLArenaPool* arena = nullptr;
    CERTCertificateScopeEntry** entries = nullptr;
    
    int numEntries = 0;
    int i;
    char* hostLower = nullptr;
    uint32_t hostIPAddr = 0;

    PR_ASSERT(cert && hostname && hostIP);

    
    srv = CERT_FindCertExtension(cert, SEC_OID_NS_CERT_EXT_SCOPE_OF_USE,
                                 &extData);
    if (srv != SECSuccess) {
        



        goto done;
    }

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (!arena) {
        goto done;
    }

    


    srv = cert_DecodeScopeOfUseEntries(arena, &extData, &entries, &numEntries);
    if (srv != SECSuccess) {
        





        goto done;
    }

    
    for (i = 0; i < numEntries; i++) {
        


        CERTGeneralName* genname = entries[i]->constraint;

        
        if (!genname) {
            
            continue;
        }

        switch (genname->type) {
        case certDNSName: {
            


            char* pattern = nullptr;
            char* substring = nullptr;

            
            genname->name.other.data[genname->name.other.len] = '\0';
            pattern = _str_to_lower((char*)genname->name.other.data);

            if (!hostLower) {
                
                hostLower = _str_to_lower(PL_strdup(hostname));
            }

            
            if (((substring = strstr(hostLower, pattern)) != nullptr) &&
                
                (strlen(substring) == strlen(pattern)) &&
                
                ((substring == hostLower) || (*(substring-1) == '.'))) {
                


                rv = true;
            }
            else {
                rv = false;
            }
            
            break;
        }
        case certIPAddress: {
            uint32_t constraint;
            uint32_t mask;
            PRNetAddr addr;
            
            if (hostIPAddr == 0) {
                
                PR_StringToNetAddr(hostIP, &addr);
                hostIPAddr = addr.inet.ip;
            }

            if (cert_DecodeCertIPAddress(&(genname->name.other), &constraint, 
                                         &mask) != SECSuccess) {
                continue;
            }
            if ((hostIPAddr & mask) == (constraint & mask)) {
                rv = true;
            }
            else {
                rv = false;
            }
            break;
        }
        default:
            
            continue; 
        }

        if (!rv) {
            
            continue;
        }

        
        if ((entries[i]->port != 0) && (port != entries[i]->port)) {
            
            rv = false;
            continue;
        }

        
        PR_ASSERT(rv);
        break;
    }
done:
    
    if (arena) {
        PORT_FreeArena(arena, false);
    }
    if (hostLower) {
        PR_Free(hostLower);
    }
    return rv;
}
#endif

















nsresult nsGetUserCertChoice(SSM_UserCertChoice* certChoice)
{
	char *mode = nullptr;
	nsresult ret;

	NS_ENSURE_ARG_POINTER(certChoice);

	nsCOMPtr<nsIPrefBranch> pref = do_GetService(NS_PREFSERVICE_CONTRACTID);

	ret = pref->GetCharPref("security.default_personal_cert", &mode);
	if (NS_FAILED(ret)) {
		goto loser;
	}

    if (PL_strcmp(mode, "Select Automatically") == 0) {
		*certChoice = AUTO;
	}
    else if (PL_strcmp(mode, "Ask Every Time") == 0) {
        *certChoice = ASK;
    }
    else {
      
      
		  *certChoice = ASK;
	}

loser:
	if (mode) {
		nsMemory::Free(mode);
	}
	return ret;
}

static bool hasExplicitKeyUsageNonRepudiation(CERTCertificate *cert)
{
  
  if (!cert->extensions)
    return false;

  SECStatus srv;
  SECItem keyUsageItem;
  keyUsageItem.data = nullptr;

  srv = CERT_FindKeyUsageExtension(cert, &keyUsageItem);
  if (srv == SECFailure)
    return false;

  unsigned char keyUsage = keyUsageItem.data[0];
  PORT_Free (keyUsageItem.data);

  return !!(keyUsage & KU_NON_REPUDIATION);
}

class ClientAuthDataRunnable : public SyncRunnableBase
{
public:
  ClientAuthDataRunnable(CERTDistNames* caNames,
                         CERTCertificate** pRetCert,
                         SECKEYPrivateKey** pRetKey,
                         nsNSSSocketInfo * info,
                         CERTCertificate * serverCert) 
    : mRV(SECFailure)
    , mErrorCodeToReport(SEC_ERROR_NO_MEMORY)
    , mPRetCert(pRetCert)
    , mPRetKey(pRetKey)
    , mCANames(caNames)
    , mSocketInfo(info)
    , mServerCert(serverCert)
  {
  }

  SECStatus mRV;                        
  PRErrorCode mErrorCodeToReport;       
  CERTCertificate** const mPRetCert;    
  SECKEYPrivateKey** const mPRetKey;    
protected:
  virtual void RunOnTargetThread();
private:
  CERTDistNames* const mCANames;        
  nsNSSSocketInfo * const mSocketInfo;  
  CERTCertificate * const mServerCert;  
};
















SECStatus nsNSS_SSLGetClientAuthData(void* arg, PRFileDesc* socket,
								   CERTDistNames* caNames,
								   CERTCertificate** pRetCert,
								   SECKEYPrivateKey** pRetKey)
{
  nsNSSShutDownPreventionLock locker;

  if (!socket || !caNames || !pRetCert || !pRetKey) {
    PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
    return SECFailure;
  }

  RefPtr<nsNSSSocketInfo> info(
    reinterpret_cast<nsNSSSocketInfo*>(socket->higher->secret));

  CERTCertificate* serverCert = SSL_PeerCertificate(socket);
  if (!serverCert) {
    NS_NOTREACHED("Missing server certificate should have been detected during "
                  "server cert authentication.");
    PR_SetError(SSL_ERROR_NO_CERTIFICATE, 0);
    return SECFailure;
  }

  if (info->GetJoined()) {
    
    
    

    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
           ("[%p] Not returning client cert due to previous join\n", socket));
    *pRetCert = nullptr;
    *pRetKey = nullptr;
    return SECSuccess;
  }

  
  RefPtr<ClientAuthDataRunnable> runnable(
    new ClientAuthDataRunnable(caNames, pRetCert, pRetKey, info, serverCert));
  nsresult rv = runnable->DispatchToMainThreadAndWait();
  if (NS_FAILED(rv)) {
    PR_SetError(SEC_ERROR_NO_MEMORY, 0);
    return SECFailure;
  }
  
  if (runnable->mRV != SECSuccess) {
    PR_SetError(runnable->mErrorCodeToReport, 0);
  } else if (*runnable->mPRetCert || *runnable->mPRetKey) {
    
    info->SetSentClientCert();
  }

  return runnable->mRV;
}

void ClientAuthDataRunnable::RunOnTargetThread()
{
  PLArenaPool* arena = nullptr;
  char** caNameStrings;
  ScopedCERTCertificate cert;
  ScopedSECKEYPrivateKey privKey;
  ScopedCERTCertList certList;
  CERTCertListNode* node;
  ScopedCERTCertNicknames nicknames;
  char* extracted = nullptr;
  int keyError = 0; 
  SSM_UserCertChoice certChoice;
  int32_t NumberOfCerts = 0;
  void * wincx = mSocketInfo;
  nsresult rv;

  
  arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
  if (!arena) {
    goto loser;
  }

  caNameStrings = (char**)PORT_ArenaAlloc(arena, 
                                          sizeof(char*)*(mCANames->nnames));
  if (!caNameStrings) {
    goto loser;
  }

  mRV = nsConvertCANamesToStrings(arena, caNameStrings, mCANames);
  if (mRV != SECSuccess) {
    goto loser;
  }

  
  if (NS_FAILED(nsGetUserCertChoice(&certChoice))) {
    goto loser;
  }

  	
  if (certChoice == AUTO) {
    

    
    certList = CERT_FindUserCertsByUsage(CERT_GetDefaultCertDB(), 
                                         certUsageSSLClient, false,
                                         true, wincx);
    if (!certList) {
      goto noCert;
    }

    
    mRV = CERT_FilterCertListByCANames(certList, mCANames->nnames,
                                       caNameStrings, certUsageSSLClient);
    if (mRV != SECSuccess) {
      goto noCert;
    }

    
    node = CERT_LIST_HEAD(certList);
    if (CERT_LIST_END(node, certList)) {
      goto noCert;
    }

    ScopedCERTCertificate low_prio_nonrep_cert;

    
    while (!CERT_LIST_END(node, certList)) {
      


#if 0		
      if (!CERT_MatchesScopeOfUse(node->cert, mSocketInfo->GetHostName,
                                  info->GetHostIP, info->GetHostPort)) {
          node = CERT_LIST_NEXT(node);
          continue;
      }
#endif

      privKey = PK11_FindKeyByAnyCert(node->cert, wincx);
      if (privKey) {
        if (hasExplicitKeyUsageNonRepudiation(node->cert)) {
          privKey = nullptr;
          
          if (!low_prio_nonrep_cert) 
            low_prio_nonrep_cert = CERT_DupCertificate(node->cert);
        }
        else {
          
          cert = CERT_DupCertificate(node->cert);
          break;
        }
      }
      keyError = PR_GetError();
      if (keyError == SEC_ERROR_BAD_PASSWORD) {
          
          goto loser;
      }

      node = CERT_LIST_NEXT(node);
    }

    if (!cert && low_prio_nonrep_cert) {
      cert = low_prio_nonrep_cert.forget();
      privKey = PK11_FindKeyByAnyCert(cert, wincx);
    }

    if (!cert) {
        goto noCert;
    }
  }
  else { 
    

    nsXPIDLCString hostname;
    mSocketInfo->GetHostName(getter_Copies(hostname));

    RefPtr<nsClientAuthRememberService> cars =
        mSocketInfo->SharedState().GetClientAuthRememberService();

    bool hasRemembered = false;
    nsCString rememberedDBKey;
    if (cars) {
      bool found;
      rv = cars->HasRememberedDecision(hostname, mServerCert,
                                                rememberedDBKey, &found);
      if (NS_SUCCEEDED(rv) && found) {
        hasRemembered = true;
      }
    }

    bool canceled = false;

if (hasRemembered)
{
    if (rememberedDBKey.IsEmpty())
    {
      canceled = true;
    }
    else
    {
      nsCOMPtr<nsIX509CertDB> certdb;
      certdb = do_GetService(NS_X509CERTDB_CONTRACTID);
      if (certdb)
      {
        nsCOMPtr<nsIX509Cert> found_cert;
        nsresult find_rv = 
          certdb->FindCertByDBKey(rememberedDBKey.get(), nullptr,
                                  getter_AddRefs(found_cert));
        if (NS_SUCCEEDED(find_rv) && found_cert) {
          nsNSSCertificate *obj_cert = reinterpret_cast<nsNSSCertificate *>(found_cert.get());
          if (obj_cert) {
            cert = obj_cert->GetCert();

#ifdef DEBUG_kaie
            nsAutoString nick, nickWithSerial, details;
            if (NS_SUCCEEDED(obj_cert->FormatUIStrings(nick, 
                                                       nickWithSerial, 
                                                       details))) {
              NS_LossyConvertUTF16toASCII asc(nickWithSerial);
              fprintf(stderr, "====> remembered serial %s\n", asc.get());
            }
#endif

          }
        }
        
        if (!cert) {
          hasRemembered = false;
        }
      }
    }
}

if (!hasRemembered)
{
    
    nsIClientAuthDialogs *dialogs = nullptr;
    int32_t selectedIndex = -1;
    PRUnichar **certNicknameList = nullptr;
    PRUnichar **certDetailsList = nullptr;

    
    
    certList = CERT_FindUserCertsByUsage(CERT_GetDefaultCertDB(), 
                                         certUsageSSLClient, false, 
                                         false, wincx);
    if (!certList) {
      goto noCert;
    }

    if (mCANames->nnames != 0) {
      


      mRV = CERT_FilterCertListByCANames(certList, mCANames->nnames, 
                                        caNameStrings, 
                                        certUsageSSLClient);
      if (mRV != SECSuccess) {
        goto loser;
      }
    }

    if (CERT_LIST_END(CERT_LIST_HEAD(certList), certList)) {
      
      goto noCert;
    }

    
    node = CERT_LIST_HEAD(certList);
    while (!CERT_LIST_END(node, certList)) {
      ++NumberOfCerts;
#if 0 
      if (!CERT_MatchesScopeOfUse(node->cert, conn->hostName,
                                  conn->hostIP, conn->port)) {
        CERTCertListNode* removed = node;
        node = CERT_LIST_NEXT(removed);
        CERT_RemoveCertListNode(removed);
      }
      else {
        node = CERT_LIST_NEXT(node);
      }
#endif
      node = CERT_LIST_NEXT(node);
    }
    if (CERT_LIST_END(CERT_LIST_HEAD(certList), certList)) {
      goto noCert;
    }

    nicknames = getNSSCertNicknamesFromCertList(certList);

    if (!nicknames) {
      goto loser;
    }

    NS_ASSERTION(nicknames->numnicknames == NumberOfCerts, "nicknames->numnicknames != NumberOfCerts");

    
    char *ccn = CERT_GetCommonName(&mServerCert->subject);
    void *v = ccn;
    voidCleaner ccnCleaner(v);
    NS_ConvertUTF8toUTF16 cn(ccn);

    int32_t port;
    mSocketInfo->GetPort(&port);

    nsString cn_host_port;
    if (ccn && strcmp(ccn, hostname) == 0) {
      cn_host_port.Append(cn);
      cn_host_port.AppendLiteral(":");
      cn_host_port.AppendInt(port);
    }
    else {
      cn_host_port.Append(cn);
      cn_host_port.AppendLiteral(" (");
      cn_host_port.AppendLiteral(":");
      cn_host_port.AppendInt(port);
      cn_host_port.AppendLiteral(")");
    }

    char *corg = CERT_GetOrgName(&mServerCert->subject);
    NS_ConvertUTF8toUTF16 org(corg);
    if (corg) PORT_Free(corg);

    char *cissuer = CERT_GetOrgName(&mServerCert->issuer);
    NS_ConvertUTF8toUTF16 issuer(cissuer);
    if (cissuer) PORT_Free(cissuer);

    certNicknameList = (PRUnichar **)nsMemory::Alloc(sizeof(PRUnichar *) * nicknames->numnicknames);
    if (!certNicknameList)
      goto loser;
    certDetailsList = (PRUnichar **)nsMemory::Alloc(sizeof(PRUnichar *) * nicknames->numnicknames);
    if (!certDetailsList) {
      nsMemory::Free(certNicknameList);
      goto loser;
    }

    int32_t CertsToUse;
    for (CertsToUse = 0, node = CERT_LIST_HEAD(certList);
         !CERT_LIST_END(node, certList) && CertsToUse < nicknames->numnicknames;
         node = CERT_LIST_NEXT(node)
        )
    {
      RefPtr<nsNSSCertificate> tempCert(nsNSSCertificate::Create(node->cert));

      if (!tempCert)
        continue;
      
      NS_ConvertUTF8toUTF16 i_nickname(nicknames->nicknames[CertsToUse]);
      nsAutoString nickWithSerial, details;
      
      if (NS_FAILED(tempCert->FormatUIStrings(i_nickname, nickWithSerial, details)))
        continue;

      certNicknameList[CertsToUse] = ToNewUnicode(nickWithSerial);
      if (!certNicknameList[CertsToUse])
        continue;
      certDetailsList[CertsToUse] = ToNewUnicode(details);
      if (!certDetailsList[CertsToUse]) {
        nsMemory::Free(certNicknameList[CertsToUse]);
        continue;
      }

      ++CertsToUse;
    }

    
    nsresult rv = getNSSDialogs((void**)&dialogs, 
                                NS_GET_IID(nsIClientAuthDialogs),
                                NS_CLIENTAUTHDIALOGS_CONTRACTID);

    if (NS_FAILED(rv)) {
      NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(CertsToUse, certNicknameList);
      NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(CertsToUse, certDetailsList);
      goto loser;
    }

    {
      nsPSMUITracker tracker;
      if (tracker.isUIForbidden()) {
        rv = NS_ERROR_NOT_AVAILABLE;
      }
      else {
        rv = dialogs->ChooseCertificate(mSocketInfo, cn_host_port.get(),
                                        org.get(), issuer.get(), 
                                        (const PRUnichar**)certNicknameList,
                                        (const PRUnichar**)certDetailsList,
                                        CertsToUse, &selectedIndex, &canceled);
      }
    }

    NS_RELEASE(dialogs);
    NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(CertsToUse, certNicknameList);
    NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(CertsToUse, certDetailsList);
    
    if (NS_FAILED(rv)) goto loser;

    
    bool wantRemember = false;
    mSocketInfo->GetRememberClientAuthCertificate(&wantRemember);

    int i;
    if (!canceled)
    for (i = 0, node = CERT_LIST_HEAD(certList);
         !CERT_LIST_END(node, certList);
         ++i, node = CERT_LIST_NEXT(node)) {

      if (i == selectedIndex) {
        cert = CERT_DupCertificate(node->cert);
        break;
      }
    }

    if (cars && wantRemember) {
      cars->RememberDecision(hostname, mServerCert,
                             canceled ? nullptr : cert.get());
    }
}

    if (canceled) { rv = NS_ERROR_NOT_AVAILABLE; goto loser; }

    if (!cert) {
      goto loser;
    }

    
    privKey = PK11_FindKeyByAnyCert(cert, wincx);
    if (!privKey) {
      keyError = PR_GetError();
      if (keyError == SEC_ERROR_BAD_PASSWORD) {
          
          goto loser;
      }
      else {
          goto noCert;
      }
    }
  }
  goto done;

noCert:
loser:
  if (mRV == SECSuccess) {
    mRV = SECFailure;
  }
done:
  int error = PR_GetError();

  if (extracted) {
    PR_Free(extracted);
  }
  if (arena) {
    PORT_FreeArena(arena, false);
  }

  *mPRetCert = cert.forget();
  *mPRetKey = privKey.forget();

  if (mRV == SECFailure) {
    mErrorCodeToReport = error;
  }
}

static PRFileDesc*
nsSSLIOLayerImportFD(PRFileDesc *fd,
                     nsNSSSocketInfo *infoObject,
                     const char *host)
{
  nsNSSShutDownPreventionLock locker;
  PRFileDesc* sslSock = SSL_ImportFD(nullptr, fd);
  if (!sslSock) {
    NS_ASSERTION(false, "NSS: Error importing socket");
    return nullptr;
  }
  SSL_SetPKCS11PinArg(sslSock, (nsIInterfaceRequestor*)infoObject);
  SSL_HandshakeCallback(sslSock, HandshakeCallback, infoObject);

  
  uint32_t flags = 0;
  infoObject->GetProviderFlags(&flags);
  if (flags & nsISocketProvider::ANONYMOUS_CONNECT) {
      SSL_GetClientAuthDataHook(sslSock, nullptr, infoObject);
  } else {
      SSL_GetClientAuthDataHook(sslSock, 
                            (SSLGetClientAuthData)nsNSS_SSLGetClientAuthData,
                            infoObject);
  }
  if (SECSuccess != SSL_AuthCertificateHook(sslSock, AuthCertificateHook,
                                            infoObject)) {
    NS_NOTREACHED("failed to configure AuthCertificateHook");
    goto loser;
  }

  if (SECSuccess != SSL_SetURL(sslSock, host)) {
    NS_NOTREACHED("SSL_SetURL failed");
    goto loser;
  }

  
  
  EnsureServerVerificationInitialized();

  return sslSock;
loser:
  if (sslSock) {
    PR_Close(sslSock);
  }
  return nullptr;
}

static nsresult
nsSSLIOLayerSetOptions(PRFileDesc *fd, bool forSTARTTLS, 
                       const char *proxyHost, const char *host, int32_t port,
                       nsNSSSocketInfo *infoObject)
{
  nsNSSShutDownPreventionLock locker;
  if (forSTARTTLS || proxyHost) {
    if (SECSuccess != SSL_OptionSet(fd, SSL_SECURITY, false)) {
      return NS_ERROR_FAILURE;
    }
    infoObject->SetHasCleartextPhase(true);
  }

  
  
  nsAutoCString key;
  key = nsDependentCString(host) + NS_LITERAL_CSTRING(":") + nsPrintfCString("%d", port);

  if (infoObject->SharedState().IOLayerHelpers().isKnownAsIntolerantSite(key)) {
    if (SECSuccess != SSL_OptionSet(fd, SSL_ENABLE_TLS, false))
      return NS_ERROR_FAILURE;

    infoObject->SetAllowTLSIntoleranceTimeout(false);
      
    
    
    
    
    
    
  }

  PRBool enabled;
  if (SECSuccess != SSL_OptionGet(fd, SSL_ENABLE_SSL3, &enabled)) {
    return NS_ERROR_FAILURE;
  }
  infoObject->SetSSL3Enabled(enabled);
  if (SECSuccess != SSL_OptionGet(fd, SSL_ENABLE_TLS, &enabled)) {
    return NS_ERROR_FAILURE;
  }
  infoObject->SetTLSEnabled(enabled);

  if (SECSuccess != SSL_OptionSet(fd, SSL_HANDSHAKE_AS_CLIENT, true)) {
    return NS_ERROR_FAILURE;
  }

  nsSSLIOLayerHelpers& ioHelpers = infoObject->SharedState().IOLayerHelpers();
  if (ioHelpers.isRenegoUnrestrictedSite(nsDependentCString(host))) {
    if (SECSuccess != SSL_OptionSet(fd, SSL_REQUIRE_SAFE_NEGOTIATION, false)) {
      return NS_ERROR_FAILURE;
    }
    if (SECSuccess != SSL_OptionSet(fd, SSL_ENABLE_RENEGOTIATION, SSL_RENEGOTIATE_UNRESTRICTED)) {
      return NS_ERROR_FAILURE;
    }
  }

  
  
  uint32_t flags = infoObject->GetProviderFlags();
  nsAutoCString peerId;
  if (flags & nsISocketProvider::ANONYMOUS_CONNECT) { 
    peerId.Append("anon:");
  }
  if (flags & nsISocketProvider::NO_PERMANENT_STORAGE) {
    peerId.Append("private:");
  }
  peerId.Append(host);
  peerId.Append(':');
  peerId.AppendInt(port);
  if (SECSuccess != SSL_SetSockPeerID(fd, peerId.get())) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

nsresult
nsSSLIOLayerAddToSocket(int32_t family,
                        const char* host,
                        int32_t port,
                        const char* proxyHost,
                        int32_t proxyPort,
                        PRFileDesc* fd,
                        nsISupports** info,
                        bool forSTARTTLS,
                        uint32_t providerFlags)
{
  nsNSSShutDownPreventionLock locker;
  PRFileDesc* layer = nullptr;
  nsresult rv;
  PRStatus stat;

  SharedSSLState* sharedState =
    providerFlags & nsISocketProvider::NO_PERMANENT_STORAGE ? PrivateSSLState() : PublicSSLState();
  nsNSSSocketInfo* infoObject = new nsNSSSocketInfo(*sharedState, providerFlags);
  if (!infoObject) return NS_ERROR_FAILURE;
  
  NS_ADDREF(infoObject);
  infoObject->SetForSTARTTLS(forSTARTTLS);
  infoObject->SetHostName(host);
  infoObject->SetPort(port);

  PRFileDesc *sslSock = nsSSLIOLayerImportFD(fd, infoObject, host);
  if (!sslSock) {
    NS_ASSERTION(false, "NSS: Error importing socket");
    goto loser;
  }

  infoObject->SetFileDescPtr(sslSock);

  rv = nsSSLIOLayerSetOptions(sslSock, forSTARTTLS, proxyHost, host, port,
                              infoObject);

  if (NS_FAILED(rv))
    goto loser;

  
  layer = PR_CreateIOLayerStub(nsSSLIOLayerHelpers::nsSSLIOLayerIdentity,
                               &nsSSLIOLayerHelpers::nsSSLIOLayerMethods);
  if (!layer)
    goto loser;
  
  layer->secret = (PRFilePrivate*) infoObject;
  stat = PR_PushIOLayer(sslSock, PR_GetLayersIdentity(sslSock), layer);
  
  if (stat == PR_FAILURE) {
    goto loser;
  }
  
  nsNSSShutDownList::trackSSLSocketCreate();

  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("[%p] Socket set up\n", (void*)sslSock));
  infoObject->QueryInterface(NS_GET_IID(nsISupports), (void**) (info));

  
  if (forSTARTTLS || proxyHost) {
    infoObject->SetHandshakePending(false);
  }

  infoObject->SharedState().NoteSocketCreated();

  return NS_OK;
 loser:
  NS_IF_RELEASE(infoObject);
  if (layer) {
    layer->dtor(layer);
  }
  return NS_ERROR_FAILURE;
}
