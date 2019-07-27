





#include "nsNSSIOLayer.h"

#include "pkix/pkixtypes.h"
#include "nsNSSComponent.h"
#include "mozilla/BinarySearch.h"
#include "mozilla/Casting.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/UniquePtr.h"
#include "mozilla/Telemetry.h"

#include "prlog.h"
#include "prmem.h"
#include "prnetdb.h"
#include "nsIPrefService.h"
#include "nsIClientAuthDialogs.h"
#include "nsClientAuthRemember.h"

#include "nsNetUtil.h"
#include "nsPrintfCString.h"
#include "SSLServerCertVerification.h"
#include "nsNSSCertHelper.h"

#include "nsCharSeparatedTokenizer.h"
#include "nsIConsoleService.h"
#include "PSMRunnable.h"
#include "ScopedNSSTypes.h"
#include "SharedSSLState.h"
#include "mozilla/Preferences.h"
#include "nsContentUtils.h"
#include "NSSCertDBTrustDomain.h"
#include "NSSErrorsService.h"

#include "ssl.h"
#include "sslproto.h"
#include "secerr.h"
#include "sslerr.h"
#include "secder.h"
#include "keyhi.h"

#include <algorithm>

#include "IntolerantFallbackList.inc"

using namespace mozilla;
using namespace mozilla::psm;


                            


                       
                       
                       
                       
                       

namespace {

void
getSiteKey(const nsACString& hostName, uint16_t port,
            nsCSubstring& key)
{
  key = hostName;
  key.AppendASCII(":");
  key.AppendInt(port);
}


typedef enum {ASK, AUTO} SSM_UserCertChoice;






static const bool FALSE_START_REQUIRE_NPN_DEFAULT = false;

} 

#ifdef PR_LOGGING
extern PRLogModuleInfo* gPIPNSSLog;
#endif

nsNSSSocketInfo::nsNSSSocketInfo(SharedSSLState& aState, uint32_t providerFlags)
  : mFd(nullptr),
    mCertVerificationState(before_cert_verification),
    mSharedState(aState),
    mForSTARTTLS(false),
    mHandshakePending(true),
    mRememberClientAuthCertificate(false),
    mPreliminaryHandshakeDone(false),
    mNPNCompleted(false),
    mFalseStartCallbackCalled(false),
    mFalseStarted(false),
    mIsFullHandshake(false),
    mHandshakeCompleted(false),
    mJoined(false),
    mSentClientCert(false),
    mNotedTimeUntilReady(false),
    mFailedVerification(false),
    mKEAUsed(nsISSLSocketControl::KEY_EXCHANGE_UNKNOWN),
    mKEAKeyBits(0),
    mSSLVersionUsed(nsISSLSocketControl::SSL_VERSION_UNKNOWN),
    mMACAlgorithmUsed(nsISSLSocketControl::SSL_MAC_UNKNOWN),
    mBypassAuthentication(false),
    mProviderFlags(providerFlags),
    mSocketCreationTimestamp(TimeStamp::Now()),
    mPlaintextBytesRead(0),
    mClientCert(nullptr)
{
  mTLSVersionRange.min = 0;
  mTLSVersionRange.max = 0;
}

nsNSSSocketInfo::~nsNSSSocketInfo()
{
}

NS_IMPL_ISUPPORTS_INHERITED(nsNSSSocketInfo, TransportSecurityInfo,
                            nsISSLSocketControl,
                            nsIClientAuthUserDecision)

NS_IMETHODIMP
nsNSSSocketInfo::GetProviderFlags(uint32_t* aProviderFlags)
{
  *aProviderFlags = mProviderFlags;
  return NS_OK;
}

NS_IMETHODIMP
nsNSSSocketInfo::GetKEAUsed(int16_t* aKea)
{
  *aKea = mKEAUsed;
  return NS_OK;
}

NS_IMETHODIMP
nsNSSSocketInfo::GetKEAKeyBits(uint32_t* aKeyBits)
{
  *aKeyBits = mKEAKeyBits;
  return NS_OK;
}

NS_IMETHODIMP
nsNSSSocketInfo::GetSSLVersionUsed(int16_t* aSSLVersionUsed)
{
  *aSSLVersionUsed = mSSLVersionUsed;
  return NS_OK;
}

NS_IMETHODIMP
nsNSSSocketInfo::GetSSLVersionOffered(int16_t* aSSLVersionOffered)
{
  *aSSLVersionOffered = mTLSVersionRange.max;
  return NS_OK;
}

NS_IMETHODIMP
nsNSSSocketInfo::GetMACAlgorithmUsed(int16_t* aMac)
{
  *aMac = mMACAlgorithmUsed;
  return NS_OK;
}

NS_IMETHODIMP
nsNSSSocketInfo::GetClientCert(nsIX509Cert** aClientCert)
{
  NS_ENSURE_ARG_POINTER(aClientCert);
  *aClientCert = mClientCert;
  NS_IF_ADDREF(*aClientCert);
  return NS_OK;
}

NS_IMETHODIMP
nsNSSSocketInfo::SetClientCert(nsIX509Cert* aClientCert)
{
  mClientCert = aClientCert;
  return NS_OK;
}

NS_IMETHODIMP
nsNSSSocketInfo::GetBypassAuthentication(bool* arg)
{
  *arg = mBypassAuthentication;
  return NS_OK;
}

NS_IMETHODIMP
nsNSSSocketInfo::SetBypassAuthentication(bool arg)
{
  mBypassAuthentication = arg;
  return NS_OK;
}

NS_IMETHODIMP
nsNSSSocketInfo::GetFailedVerification(bool* arg)
{
  *arg = mFailedVerification;
  return NS_OK;
}

NS_IMETHODIMP
nsNSSSocketInfo::GetAuthenticationName(nsACString& aAuthenticationName)
{
  aAuthenticationName = GetHostName();
  return NS_OK;
}

NS_IMETHODIMP
nsNSSSocketInfo::SetAuthenticationName(const nsACString& aAuthenticationName)
{
  return SetHostName(PromiseFlatCString(aAuthenticationName).get());
}

NS_IMETHODIMP
nsNSSSocketInfo::GetAuthenticationPort(int32_t* aAuthenticationPort)
{
  return GetPort(aAuthenticationPort);
}

NS_IMETHODIMP
nsNSSSocketInfo::SetAuthenticationPort(int32_t aAuthenticationPort)
{
  return SetPort(aAuthenticationPort);
}

NS_IMETHODIMP
nsNSSSocketInfo::GetRememberClientAuthCertificate(bool* aRemember)
{
  NS_ENSURE_ARG_POINTER(aRemember);
  *aRemember = mRememberClientAuthCertificate;
  return NS_OK;
}

NS_IMETHODIMP
nsNSSSocketInfo::SetRememberClientAuthCertificate(bool aRemember)
{
  mRememberClientAuthCertificate = aRemember;
  return NS_OK;
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

void
nsNSSSocketInfo::NoteTimeUntilReady()
{
  if (mNotedTimeUntilReady)
    return;

  mNotedTimeUntilReady = true;

  
  Telemetry::AccumulateTimeDelta(Telemetry::SSL_TIME_UNTIL_READY,
                                 mSocketCreationTimestamp, TimeStamp::Now());
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
         ("[%p] nsNSSSocketInfo::NoteTimeUntilReady\n", mFd));
}

void
nsNSSSocketInfo::SetHandshakeCompleted()
{
  if (!mHandshakeCompleted) {
    enum HandshakeType {
      Resumption = 1,
      FalseStarted = 2,
      ChoseNotToFalseStart = 3,
      NotAllowedToFalseStart = 4,
    };

    HandshakeType handshakeType = !IsFullHandshake() ? Resumption
                                : mFalseStarted ? FalseStarted
                                : mFalseStartCallbackCalled ? ChoseNotToFalseStart
                                : NotAllowedToFalseStart;

    
    Telemetry::AccumulateTimeDelta(Telemetry::SSL_TIME_UNTIL_HANDSHAKE_FINISHED,
                                   mSocketCreationTimestamp, TimeStamp::Now());

    
    
    Telemetry::Accumulate(Telemetry::SSL_RESUMED_SESSION,
                          handshakeType == Resumption);
    Telemetry::Accumulate(Telemetry::SSL_HANDSHAKE_TYPE, handshakeType);
  }


    
    
    
    PRFileDesc* poppedPlaintext =
      PR_GetIdentitiesLayer(mFd, nsSSLIOLayerHelpers::nsSSLPlaintextLayerIdentity);
    if (poppedPlaintext) {
      PR_PopIOLayer(mFd, nsSSLIOLayerHelpers::nsSSLPlaintextLayerIdentity);
      poppedPlaintext->dtor(poppedPlaintext);
    }

    mHandshakeCompleted = true;

    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
           ("[%p] nsNSSSocketInfo::SetHandshakeCompleted\n", (void*) mFd));

    mIsFullHandshake = false; 
}

void
nsNSSSocketInfo::SetNegotiatedNPN(const char* value, uint32_t length)
{
  if (!value) {
    mNegotiatedNPN.Truncate();
  } else {
    mNegotiatedNPN.Assign(value, length);
  }
  mNPNCompleted = true;
}

NS_IMETHODIMP
nsNSSSocketInfo::GetNegotiatedNPN(nsACString& aNegotiatedNPN)
{
  if (!mNPNCompleted)
    return NS_ERROR_NOT_CONNECTED;

  aNegotiatedNPN = mNegotiatedNPN;
  return NS_OK;
}

NS_IMETHODIMP
nsNSSSocketInfo::IsAcceptableForHost(const nsACString& hostname, bool* _retval)
{
  
  
  if (hostname.Equals(GetHostName())) {
    *_retval = true;
    return NS_OK;
  }

  
  
  if (!mHandshakeCompleted || !SSLStatus() || !SSLStatus()->HasServerCert()) {
    return NS_OK;
  }

  
  
  
  if (SSLStatus()->mHaveCertErrorBits)
    return NS_OK;

  
  
  
  if (mSentClientCert)
    return NS_OK;

  
  

  ScopedCERTCertificate nssCert;

  nsCOMPtr<nsIX509Cert> cert;
  if (NS_FAILED(SSLStatus()->GetServerCert(getter_AddRefs(cert)))) {
    return NS_OK;
  }
  if (cert) {
    nssCert = cert->GetCert();
  }

  if (!nssCert) {
    return NS_OK;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  RefPtr<SharedCertVerifier> certVerifier(GetDefaultCertVerifier());
  if (!certVerifier) {
    return NS_OK;
  }
  nsAutoCString hostnameFlat(PromiseFlatCString(hostname));
  CertVerifier::Flags flags = CertVerifier::FLAG_LOCAL_ONLY;
  SECStatus rv = certVerifier->VerifySSLServerCert(nssCert, nullptr,
                                                   mozilla::pkix::Now(),
                                                   nullptr, hostnameFlat.get(),
                                                   false, flags, nullptr,
                                                   nullptr);
  if (rv != SECSuccess) {
    return NS_OK;
  }

  
  *_retval = true;
  return NS_OK;
}

NS_IMETHODIMP
nsNSSSocketInfo::JoinConnection(const nsACString& npnProtocol,
                                const nsACString& hostname,
                                int32_t port,
                                bool* _retval)
{
  *_retval = false;

  
  if (port != GetPort())
    return NS_OK;

  
  if (!mNPNCompleted || !mNegotiatedNPN.Equals(npnProtocol))
    return NS_OK;

  IsAcceptableForHost(hostname, _retval);

  if (*_retval) {
    
    mJoined = true;
  }
  return NS_OK;
}

bool
nsNSSSocketInfo::GetForSTARTTLS()
{
  return mForSTARTTLS;
}

void
nsNSSSocketInfo::SetForSTARTTLS(bool aForSTARTTLS)
{
  mForSTARTTLS = aForSTARTTLS;
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
nsNSSSocketInfo::SetNPNList(nsTArray<nsCString>& protocolArray)
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
        reinterpret_cast<const unsigned char*>(npnList.get()),
        npnList.Length()) != SECSuccess)
    return NS_ERROR_FAILURE;

  return NS_OK;
}

nsresult
nsNSSSocketInfo::ActivateSSL()
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

nsresult
nsNSSSocketInfo::GetFileDescPtr(PRFileDesc** aFilePtr)
{
  *aFilePtr = mFd;
  return NS_OK;
}

nsresult
nsNSSSocketInfo::SetFileDescPtr(PRFileDesc* aFilePtr)
{
  mFd = aFilePtr;
  return NS_OK;
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
    mFailedVerification = true;
    SetCanceled(errorCode, errorMessageType);
  }

  if (mPlaintextBytesRead && !errorCode) {
    Telemetry::Accumulate(Telemetry::SSL_BYTES_BEFORE_CERT_CALLBACK,
                          AssertedCast<uint32_t>(mPlaintextBytesRead));
  }

  mCertVerificationState = after_cert_verification;
}

SharedSSLState&
nsNSSSocketInfo::SharedState()
{
  return mSharedState;
}

void nsSSLIOLayerHelpers::Cleanup()
{
  MutexAutoLock lock(mutex);
  mTLSIntoleranceInfo.Clear();
  mInsecureFallbackSites.Clear();
}

static void
nsHandleSSLError(nsNSSSocketInfo* socketInfo,
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

  
  socketInfo->SetCanceled(err, PlainErrorMessage);
  nsXPIDLString errorString;
  socketInfo->GetErrorLogMessage(err, errtype, errorString);

  if (!errorString.IsEmpty()) {
    nsContentUtils::LogSimpleConsoleError(errorString, "SSL");
  }
}

namespace {

enum Operation { reading, writing, not_reading_or_writing };

int32_t checkHandshake(int32_t bytesTransfered, bool wasReading,
                       PRFileDesc* ssl_layer_fd,
                       nsNSSSocketInfo* socketInfo);

nsNSSSocketInfo*
getSocketInfoIfRunning(PRFileDesc* fd, Operation op,
                       const nsNSSShutDownPreventionLock& )
{
  if (!fd || !fd->lower || !fd->secret ||
      fd->identity != nsSSLIOLayerHelpers::nsSSLIOLayerIdentity) {
    NS_ERROR("bad file descriptor passed to getSocketInfoIfRunning");
    PR_SetError(PR_BAD_DESCRIPTOR_ERROR, 0);
    return nullptr;
  }

  nsNSSSocketInfo* socketInfo = (nsNSSSocketInfo*) fd->secret;

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
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("[%p] connecting SSL socket\n",
         (void*) fd));
  nsNSSShutDownPreventionLock locker;
  if (!getSocketInfoIfRunning(fd, not_reading_or_writing, locker))
    return PR_FAILURE;

  PRStatus status = fd->lower->methods->connect(fd->lower, addr, timeout);
  if (status != PR_SUCCESS) {
    PR_LOG(gPIPNSSLog, PR_LOG_ERROR, ("[%p] Lower layer connect error: %d\n",
                                      (void*) fd, PR_GetError()));
    return status;
  }

  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("[%p] Connect\n", (void*) fd));
  return status;
}

void
nsSSLIOLayerHelpers::rememberTolerantAtVersion(const nsACString& hostName,
                                               int16_t port, uint16_t tolerant)
{
  nsCString key;
  getSiteKey(hostName, port, key);

  MutexAutoLock lock(mutex);

  IntoleranceEntry entry;
  if (mTLSIntoleranceInfo.Get(key, &entry)) {
    entry.AssertInvariant();
    entry.tolerant = std::max(entry.tolerant, tolerant);
    if (entry.intolerant != 0 && entry.intolerant <= entry.tolerant) {
      entry.intolerant = entry.tolerant + 1;
      entry.intoleranceReason = 0; 
    }
    if (entry.strongCipherStatus == StrongCipherStatusUnknown) {
      entry.strongCipherStatus = StrongCiphersWorked;
    }
  } else {
    entry.tolerant = tolerant;
    entry.intolerant = 0;
    entry.intoleranceReason = 0;
    entry.strongCipherStatus = StrongCiphersWorked;
  }

  entry.AssertInvariant();

  mTLSIntoleranceInfo.Put(key, entry);
}

uint16_t
nsSSLIOLayerHelpers::forgetIntolerance(const nsACString& hostName,
                                       int16_t port)
{
  nsCString key;
  getSiteKey(hostName, port, key);

  MutexAutoLock lock(mutex);

  uint16_t tolerant = 0;
  IntoleranceEntry entry;
  if (mTLSIntoleranceInfo.Get(key, &entry)) {
    entry.AssertInvariant();

    tolerant = entry.tolerant;
    entry.intolerant = 0;
    entry.intoleranceReason = 0;
    if (entry.strongCipherStatus != StrongCiphersWorked) {
      entry.strongCipherStatus = StrongCipherStatusUnknown;
    }

    entry.AssertInvariant();
    mTLSIntoleranceInfo.Put(key, entry);
  }

  return tolerant;
}

bool
nsSSLIOLayerHelpers::fallbackLimitReached(const nsACString& hostName,
                                          uint16_t intolerant)
{
  if (isInsecureFallbackSite(hostName)) {
    return intolerant <= SSL_LIBRARY_VERSION_TLS_1_0;
  }
  return intolerant <= mVersionFallbackLimit;
}


bool
nsSSLIOLayerHelpers::rememberIntolerantAtVersion(const nsACString& hostName,
                                                 int16_t port,
                                                 uint16_t minVersion,
                                                 uint16_t intolerant,
                                                 PRErrorCode intoleranceReason)
{
  if (intolerant <= minVersion || fallbackLimitReached(hostName, intolerant)) {
    
    uint32_t tolerant = forgetIntolerance(hostName, port);
    
    
    if (intolerant <= tolerant) {
      return false;
    }

    uint32_t fallbackLimitBucket = 0;
    
    if (intolerant <= minVersion) {
      switch (minVersion) {
        case SSL_LIBRARY_VERSION_TLS_1_0:
          fallbackLimitBucket += 1;
          break;
        case SSL_LIBRARY_VERSION_TLS_1_1:
          fallbackLimitBucket += 2;
          break;
        case SSL_LIBRARY_VERSION_TLS_1_2:
          fallbackLimitBucket += 3;
          break;
      }
    }
    
    if (intolerant <= mVersionFallbackLimit) {
      switch (mVersionFallbackLimit) {
        case SSL_LIBRARY_VERSION_TLS_1_0:
          fallbackLimitBucket += 4;
          break;
        case SSL_LIBRARY_VERSION_TLS_1_1:
          fallbackLimitBucket += 8;
          break;
        case SSL_LIBRARY_VERSION_TLS_1_2:
          fallbackLimitBucket += 12;
          break;
      }
    }
    if (fallbackLimitBucket) {
      Telemetry::Accumulate(Telemetry::SSL_FALLBACK_LIMIT_REACHED,
                            fallbackLimitBucket);
    }

    return false;
  }

  nsCString key;
  getSiteKey(hostName, port, key);

  MutexAutoLock lock(mutex);

  IntoleranceEntry entry;
  if (mTLSIntoleranceInfo.Get(key, &entry)) {
    entry.AssertInvariant();
    if (intolerant <= entry.tolerant) {
      
      return false;
    }
    if ((entry.intolerant != 0 && intolerant >= entry.intolerant)) {
      
      return true;
    }
  } else {
    entry.tolerant = 0;
    entry.strongCipherStatus = StrongCipherStatusUnknown;
  }

  entry.intolerant = intolerant;
  entry.intoleranceReason = intoleranceReason;
  entry.AssertInvariant();
  mTLSIntoleranceInfo.Put(key, entry);

  return true;
}


bool
nsSSLIOLayerHelpers::rememberStrongCiphersFailed(const nsACString& hostName,
                                                 int16_t port,
                                                 PRErrorCode intoleranceReason)
{
  nsCString key;
  getSiteKey(hostName, port, key);

  MutexAutoLock lock(mutex);

  IntoleranceEntry entry;
  if (mTLSIntoleranceInfo.Get(key, &entry)) {
    entry.AssertInvariant();
    if (entry.strongCipherStatus != StrongCipherStatusUnknown) {
      
      return false;
    }
  } else {
    entry.tolerant = 0;
    entry.intolerant = 0;
    entry.intoleranceReason = intoleranceReason;
  }

  entry.strongCipherStatus = StrongCiphersFailed;
  entry.AssertInvariant();
  mTLSIntoleranceInfo.Put(key, entry);

  return true;
}

void
nsSSLIOLayerHelpers::adjustForTLSIntolerance(const nsACString& hostName,
                                             int16_t port,
                                              SSLVersionRange& range,
                                              StrongCipherStatus& strongCipherStatus)
{
  IntoleranceEntry entry;

  {
    nsCString key;
    getSiteKey(hostName, port, key);

    MutexAutoLock lock(mutex);
    if (!mTLSIntoleranceInfo.Get(key, &entry)) {
      return;
    }
  }

  entry.AssertInvariant();

  if (entry.intolerant != 0) {
    
    
    if (range.min < entry.intolerant) {
      range.max = entry.intolerant - 1;
    }
  }
  strongCipherStatus = entry.strongCipherStatus;
}

PRErrorCode
nsSSLIOLayerHelpers::getIntoleranceReason(const nsACString& hostName,
                                          int16_t port)
{
  IntoleranceEntry entry;

  {
    nsCString key;
    getSiteKey(hostName, port, key);

    MutexAutoLock lock(mutex);
    if (!mTLSIntoleranceInfo.Get(key, &entry)) {
      return 0;
    }
  }

  entry.AssertInvariant();
  return entry.intoleranceReason;
}

bool nsSSLIOLayerHelpers::nsSSLIOLayerInitialized = false;
PRDescIdentity nsSSLIOLayerHelpers::nsSSLIOLayerIdentity;
PRDescIdentity nsSSLIOLayerHelpers::nsSSLPlaintextLayerIdentity;
PRIOMethods nsSSLIOLayerHelpers::nsSSLIOLayerMethods;
PRIOMethods nsSSLIOLayerHelpers::nsSSLPlaintextLayerMethods;

static PRStatus
nsSSLIOLayerClose(PRFileDesc* fd)
{
  nsNSSShutDownPreventionLock locker;
  if (!fd)
    return PR_FAILURE;

  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("[%p] Shutting down socket\n",
         (void*) fd));

  nsNSSSocketInfo* socketInfo = (nsNSSSocketInfo*) fd->secret;
  NS_ASSERTION(socketInfo,"nsNSSSocketInfo was null for an fd");

  return socketInfo->CloseSocketAndDestroy(locker);
}

PRStatus
nsNSSSocketInfo::CloseSocketAndDestroy(
    const nsNSSShutDownPreventionLock& )
{
  nsNSSShutDownList::trackSSLSocketClose();

  PRFileDesc* popped = PR_PopIOLayer(mFd, PR_TOP_IO_LAYER);
  NS_ASSERTION(popped &&
               popped->identity == nsSSLIOLayerHelpers::nsSSLIOLayerIdentity,
               "SSL Layer not on top of stack");

  
  
  PRFileDesc* poppedPlaintext =
    PR_GetIdentitiesLayer(mFd, nsSSLIOLayerHelpers::nsSSLPlaintextLayerIdentity);
  if (poppedPlaintext) {
    PR_PopIOLayer(mFd, nsSSLIOLayerHelpers::nsSSLPlaintextLayerIdentity);
    poppedPlaintext->dtor(poppedPlaintext);
  }

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
nsDumpBuffer(unsigned char* buf, int len)
{
  char hexbuf[DUMPBUF_LINESIZE*3+1];
  char chrbuf[DUMPBUF_LINESIZE+1];
  static const char* hex = "0123456789abcdef";
  int i = 0;
  int l = 0;
  char ch;
  char* c;
  char* h;
  if (len == 0)
    return;
  hexbuf[DUMPBUF_LINESIZE*3] = '\0';
  chrbuf[DUMPBUF_LINESIZE] = '\0';
  (void) memset(hexbuf, 0x20, DUMPBUF_LINESIZE*3);
  (void) memset(chrbuf, 0x20, DUMPBUF_LINESIZE);
  h = hexbuf;
  c = chrbuf;

  while (i < len) {
    ch = buf[i];

    if (l == DUMPBUF_LINESIZE) {
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

    
    if ((ch >= 0x20) && (ch <= 0x7e)) {
      *c++ = ch;
    } else {
      *c++ = '.';
    }
    i++; l++;
  }
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("%s%s\n", hexbuf, chrbuf));
}

#define DEBUG_DUMP_BUFFER(buf,len) nsDumpBuffer(buf,len)
#else
#define DEBUG_DUMP_BUFFER(buf,len)
#endif

class SSLErrorRunnable : public SyncRunnableBase
{
 public:
  SSLErrorRunnable(nsNSSSocketInfo* infoObject,
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

uint32_t tlsIntoleranceTelemetryBucket(PRErrorCode err)
{
  
  
  
  switch (err) {
    case SSL_ERROR_BAD_MAC_ALERT: return 1;
    case SSL_ERROR_BAD_MAC_READ: return 2;
    case SSL_ERROR_HANDSHAKE_FAILURE_ALERT: return 3;
    case SSL_ERROR_HANDSHAKE_UNEXPECTED_ALERT: return 4;
    case SSL_ERROR_ILLEGAL_PARAMETER_ALERT: return 6;
    case SSL_ERROR_NO_CYPHER_OVERLAP: return 7;
    case SSL_ERROR_UNSUPPORTED_VERSION: return 10;
    case SSL_ERROR_PROTOCOL_VERSION_ALERT: return 11;
    case SSL_ERROR_BAD_HANDSHAKE_HASH_VALUE: return 13;
    case SSL_ERROR_DECODE_ERROR_ALERT: return 14;
    case PR_CONNECT_RESET_ERROR: return 16;
    case PR_END_OF_FILE_ERROR: return 17;
    default: return 0;
  }
}

bool
retryDueToTLSIntolerance(PRErrorCode err, nsNSSSocketInfo* socketInfo)
{
  
  
  

  SSLVersionRange range = socketInfo->GetTLSVersionRange();
  nsSSLIOLayerHelpers& helpers = socketInfo->SharedState().IOLayerHelpers();

  if (err == SSL_ERROR_UNSUPPORTED_VERSION &&
      range.min == SSL_LIBRARY_VERSION_TLS_1_0) {
    socketInfo->SetSecurityState(nsIWebProgressListener::STATE_IS_INSECURE |
                                 nsIWebProgressListener::STATE_USES_SSL_3);
  }

  if (err == SSL_ERROR_INAPPROPRIATE_FALLBACK_ALERT) {
    
    
    

    
    
    
    PRErrorCode originalReason =
      helpers.getIntoleranceReason(socketInfo->GetHostName(),
                                   socketInfo->GetPort());
    Telemetry::Accumulate(Telemetry::SSL_VERSION_FALLBACK_INAPPROPRIATE,
                          tlsIntoleranceTelemetryBucket(originalReason));

    helpers.forgetIntolerance(socketInfo->GetHostName(),
                              socketInfo->GetPort());

    return false;
  }

  
  bool fallbackLimitReached =
    helpers.fallbackLimitReached(socketInfo->GetHostName(), range.max);
  if (err == PR_CONNECT_RESET_ERROR && fallbackLimitReached) {
    return false;
  }

  if ((err == SSL_ERROR_NO_CYPHER_OVERLAP || err == PR_END_OF_FILE_ERROR ||
       err == PR_CONNECT_RESET_ERROR) &&
      (!fallbackLimitReached || helpers.mUnrestrictedRC4Fallback) &&
      nsNSSComponent::AreAnyWeakCiphersEnabled()) {
    if (helpers.rememberStrongCiphersFailed(socketInfo->GetHostName(),
                                            socketInfo->GetPort(), err)) {
      Telemetry::Accumulate(Telemetry::SSL_WEAK_CIPHERS_FALLBACK,
                            tlsIntoleranceTelemetryBucket(err));
      return true;
    }
    Telemetry::Accumulate(Telemetry::SSL_WEAK_CIPHERS_FALLBACK, 0);
  }

  
  

  
  
  if ((err == PR_CONNECT_RESET_ERROR || err == PR_END_OF_FILE_ERROR)
      && socketInfo->GetForSTARTTLS()) {
    return false;
  }

  uint32_t reason = tlsIntoleranceTelemetryBucket(err);
  if (reason == 0) {
    return false;
  }

  Telemetry::ID pre;
  Telemetry::ID post;
  switch (range.max) {
    case SSL_LIBRARY_VERSION_TLS_1_2:
      pre = Telemetry::SSL_TLS12_INTOLERANCE_REASON_PRE;
      post = Telemetry::SSL_TLS12_INTOLERANCE_REASON_POST;
      break;
    case SSL_LIBRARY_VERSION_TLS_1_1:
      pre = Telemetry::SSL_TLS11_INTOLERANCE_REASON_PRE;
      post = Telemetry::SSL_TLS11_INTOLERANCE_REASON_POST;
      break;
    case SSL_LIBRARY_VERSION_TLS_1_0:
      pre = Telemetry::SSL_TLS10_INTOLERANCE_REASON_PRE;
      post = Telemetry::SSL_TLS10_INTOLERANCE_REASON_POST;
      break;
    default:
      MOZ_CRASH("impossible TLS version");
      return false;
  }

  
  
  Telemetry::Accumulate(pre, reason);

  if (!helpers.rememberIntolerantAtVersion(socketInfo->GetHostName(),
                                           socketInfo->GetPort(),
                                           range.min, range.max, err)) {
    return false;
  }

  Telemetry::Accumulate(post, reason);

  return true;
}

int32_t
checkHandshake(int32_t bytesTransfered, bool wasReading,
               PRFileDesc* ssl_layer_fd, nsNSSSocketInfo* socketInfo)
{
  const PRErrorCode originalError = PR_GetError();
  PRErrorCode err = originalError;

  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  
  

  bool handleHandshakeResultNow = socketInfo->IsHandshakePending();

  bool wantRetry = false;

  if (0 > bytesTransfered) {
    if (handleHandshakeResultNow) {
      if (PR_WOULD_BLOCK_ERROR == err) {
        PR_SetError(err, 0);
        return bytesTransfered;
      }

      wantRetry = retryDueToTLSIntolerance(err, socketInfo);
    }

    
    
    
    
    
    
    
    
    
    if (!wantRetry && mozilla::psm::IsNSSErrorCode(err) &&
        !socketInfo->GetErrorCode()) {
      RefPtr<SyncRunnableBase> runnable(new SSLErrorRunnable(socketInfo,
                                                             PlainErrorMessage,
                                                             err));
      (void) runnable->DispatchToMainThreadAndWait();
    }
  } else if (wasReading && 0 == bytesTransfered) {
    
    if (handleHandshakeResultNow) {
      wantRetry = retryDueToTLSIntolerance(PR_END_OF_FILE_ERROR, socketInfo);
    }
  }

  if (wantRetry) {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
           ("[%p] checkHandshake: will retry with lower max TLS version\n",
            ssl_layer_fd));
    
    err = PR_CONNECT_RESET_ERROR;
    if (wasReading)
      bytesTransfered = -1;
  }

  
  
  
  if (handleHandshakeResultNow) {
    socketInfo->SetHandshakeNotPending();
  }

  if (bytesTransfered < 0) {
    
    
    
    
    
    if (originalError != PR_WOULD_BLOCK_ERROR && !socketInfo->GetErrorCode()) {
      socketInfo->SetCanceled(originalError, PlainErrorMessage);
    }
    PR_SetError(err, 0);
  }

  return bytesTransfered;
}

}

static int16_t
nsSSLIOLayerPoll(PRFileDesc* fd, int16_t in_flags, int16_t* out_flags)
{
  nsNSSShutDownPreventionLock locker;

  if (!out_flags) {
    NS_WARNING("nsSSLIOLayerPoll called with null out_flags");
    return 0;
  }

  *out_flags = 0;

  nsNSSSocketInfo* socketInfo =
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

  
  
  
  
  int16_t result = fd->lower->methods->poll(fd->lower, in_flags, out_flags);
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("[%p] poll SSL socket returned %d\n",
                                    (void*) fd, (int) result));
  return result;
}

nsSSLIOLayerHelpers::nsSSLIOLayerHelpers()
  : mTreatUnsafeNegotiationAsBroken(false)
  , mWarnLevelMissingRFC5746(1)
  , mTLSIntoleranceInfo()
  , mFalseStartRequireNPN(false)
  , mUseStaticFallbackList(true)
  , mUnrestrictedRC4Fallback(false)
  , mVersionFallbackLimit(SSL_LIBRARY_VERSION_TLS_1_0)
  , mutex("nsSSLIOLayerHelpers.mutex")
{
}

static int
_PSM_InvalidInt(void)
{
    PR_ASSERT(!"I/O method is invalid");
    PR_SetError(PR_INVALID_METHOD_ERROR, 0);
    return -1;
}

static int64_t
_PSM_InvalidInt64(void)
{
    PR_ASSERT(!"I/O method is invalid");
    PR_SetError(PR_INVALID_METHOD_ERROR, 0);
    return -1;
}

static PRStatus
_PSM_InvalidStatus(void)
{
    PR_ASSERT(!"I/O method is invalid");
    PR_SetError(PR_INVALID_METHOD_ERROR, 0);
    return PR_FAILURE;
}

static PRFileDesc*
_PSM_InvalidDesc(void)
{
    PR_ASSERT(!"I/O method is invalid");
    PR_SetError(PR_INVALID_METHOD_ERROR, 0);
    return nullptr;
}

static PRStatus
PSMGetsockname(PRFileDesc* fd, PRNetAddr* addr)
{
  nsNSSShutDownPreventionLock locker;
  if (!getSocketInfoIfRunning(fd, not_reading_or_writing, locker))
    return PR_FAILURE;

  return fd->lower->methods->getsockname(fd->lower, addr);
}

static PRStatus
PSMGetpeername(PRFileDesc* fd, PRNetAddr* addr)
{
  nsNSSShutDownPreventionLock locker;
  if (!getSocketInfoIfRunning(fd, not_reading_or_writing, locker))
    return PR_FAILURE;

  return fd->lower->methods->getpeername(fd->lower, addr);
}

static PRStatus
PSMGetsocketoption(PRFileDesc* fd, PRSocketOptionData* data)
{
  nsNSSShutDownPreventionLock locker;
  if (!getSocketInfoIfRunning(fd, not_reading_or_writing, locker))
    return PR_FAILURE;

  return fd->lower->methods->getsocketoption(fd, data);
}

static PRStatus
PSMSetsocketoption(PRFileDesc* fd, const PRSocketOptionData* data)
{
  nsNSSShutDownPreventionLock locker;
  if (!getSocketInfoIfRunning(fd, not_reading_or_writing, locker))
    return PR_FAILURE;

  return fd->lower->methods->setsocketoption(fd, data);
}

static int32_t
PSMRecv(PRFileDesc* fd, void* buf, int32_t amount, int flags,
        PRIntervalTime timeout)
{
  nsNSSShutDownPreventionLock locker;
  nsNSSSocketInfo* socketInfo = getSocketInfoIfRunning(fd, reading, locker);
  if (!socketInfo)
    return -1;

  if (flags != PR_MSG_PEEK && flags != 0) {
    PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
    return -1;
  }

  int32_t bytesRead = fd->lower->methods->recv(fd->lower, buf, amount, flags,
                                               timeout);

  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("[%p] read %d bytes\n", (void*) fd,
         bytesRead));

#ifdef DEBUG_SSL_VERBOSE
  DEBUG_DUMP_BUFFER((unsigned char*) buf, bytesRead);
#endif

  return checkHandshake(bytesRead, true, fd, socketInfo);
}

static int32_t
PSMSend(PRFileDesc* fd, const void* buf, int32_t amount, int flags,
        PRIntervalTime timeout)
{
  nsNSSShutDownPreventionLock locker;
  nsNSSSocketInfo* socketInfo = getSocketInfoIfRunning(fd, writing, locker);
  if (!socketInfo)
    return -1;

  if (flags != 0) {
    PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
    return -1;
  }

#ifdef DEBUG_SSL_VERBOSE
  DEBUG_DUMP_BUFFER((unsigned char*) buf, amount);
#endif

  int32_t bytesWritten = fd->lower->methods->send(fd->lower, buf, amount,
                                                  flags, timeout);

  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("[%p] wrote %d bytes\n",
         fd, bytesWritten));

  return checkHandshake(bytesWritten, false, fd, socketInfo);
}

static PRStatus
PSMBind(PRFileDesc* fd, const PRNetAddr *addr)
{
  nsNSSShutDownPreventionLock locker;
  if (!getSocketInfoIfRunning(fd, not_reading_or_writing, locker))
    return PR_FAILURE;

  return fd->lower->methods->bind(fd->lower, addr);
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

static PRStatus
PSMConnectcontinue(PRFileDesc* fd, int16_t out_flags)
{
  nsNSSShutDownPreventionLock locker;
  if (!getSocketInfoIfRunning(fd, not_reading_or_writing, locker)) {
    return PR_FAILURE;
  }

  return fd->lower->methods->connectcontinue(fd, out_flags);
}

static int
PSMAvailable(void)
{
  
  PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
  return -1;
}

static int64_t
PSMAvailable64(void)
{
  
  PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
  return -1;
}

namespace {

class PrefObserver : public nsIObserver {
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIOBSERVER
  explicit PrefObserver(nsSSLIOLayerHelpers* aOwner) : mOwner(aOwner) {}

protected:
  virtual ~PrefObserver() {}
private:
  nsSSLIOLayerHelpers* mOwner;
};

} 

NS_IMPL_ISUPPORTS(PrefObserver, nsIObserver)

NS_IMETHODIMP
PrefObserver::Observe(nsISupports* aSubject, const char* aTopic,
                      const char16_t* someData)
{
  if (nsCRT::strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID) == 0) {
    NS_ConvertUTF16toUTF8 prefName(someData);

    if (prefName.EqualsLiteral("security.ssl.treat_unsafe_negotiation_as_broken")) {
      bool enabled;
      Preferences::GetBool("security.ssl.treat_unsafe_negotiation_as_broken", &enabled);
      mOwner->setTreatUnsafeNegotiationAsBroken(enabled);
    } else if (prefName.EqualsLiteral("security.ssl.warn_missing_rfc5746")) {
      int32_t warnLevel = 1;
      Preferences::GetInt("security.ssl.warn_missing_rfc5746", &warnLevel);
      mOwner->setWarnLevelMissingRFC5746(warnLevel);
    } else if (prefName.EqualsLiteral("security.ssl.false_start.require-npn")) {
      mOwner->mFalseStartRequireNPN =
        Preferences::GetBool("security.ssl.false_start.require-npn",
                             FALSE_START_REQUIRE_NPN_DEFAULT);
    } else if (prefName.EqualsLiteral("security.tls.version.fallback-limit")) {
      mOwner->loadVersionFallbackLimit();
    } else if (prefName.EqualsLiteral("security.tls.insecure_fallback_hosts")) {
      nsCString insecureFallbackHosts;
      Preferences::GetCString("security.tls.insecure_fallback_hosts", &insecureFallbackHosts);
      mOwner->setInsecureFallbackSites(insecureFallbackHosts);
    } else if (prefName.EqualsLiteral("security.tls.insecure_fallback_hosts.use_static_list")) {
      mOwner->mUseStaticFallbackList =
        Preferences::GetBool("security.tls.insecure_fallback_hosts.use_static_list", true);
    } else if (prefName.EqualsLiteral("security.tls.unrestricted_rc4_fallback")) {
      mOwner->mUnrestrictedRC4Fallback =
        Preferences::GetBool("security.tls.unrestricted_rc4_fallback", false);
    }
  }
  return NS_OK;
}

static int32_t
PlaintextRecv(PRFileDesc* fd, void* buf, int32_t amount, int flags,
              PRIntervalTime timeout)
{
  
  
  nsNSSSocketInfo* socketInfo = nullptr;

  int32_t bytesRead = fd->lower->methods->recv(fd->lower, buf, amount, flags,
                                               timeout);
  if (fd->identity == nsSSLIOLayerHelpers::nsSSLPlaintextLayerIdentity)
    socketInfo = (nsNSSSocketInfo*) fd->secret;

  if ((bytesRead > 0) && socketInfo)
    socketInfo->AddPlaintextBytesRead(bytesRead);
  return bytesRead;
}

nsSSLIOLayerHelpers::~nsSSLIOLayerHelpers()
{
  
  
  if (mPrefObserver) {
    Preferences::RemoveObserver(mPrefObserver,
        "security.ssl.treat_unsafe_negotiation_as_broken");
    Preferences::RemoveObserver(mPrefObserver,
        "security.ssl.warn_missing_rfc5746");
    Preferences::RemoveObserver(mPrefObserver,
        "security.ssl.false_start.require-npn");
    Preferences::RemoveObserver(mPrefObserver,
        "security.tls.version.fallback-limit");
    Preferences::RemoveObserver(mPrefObserver,
        "security.tls.insecure_fallback_hosts");
    Preferences::RemoveObserver(mPrefObserver,
        "security.tls.unrestricted_rc4_fallback");
  }
}

nsresult
nsSSLIOLayerHelpers::Init()
{
  if (!nsSSLIOLayerInitialized) {
    nsSSLIOLayerInitialized = true;
    nsSSLIOLayerIdentity = PR_GetUniqueIdentity("NSS layer");
    nsSSLIOLayerMethods  = *PR_GetDefaultIOMethods();

    nsSSLIOLayerMethods.available = (PRAvailableFN) PSMAvailable;
    nsSSLIOLayerMethods.available64 = (PRAvailable64FN) PSMAvailable64;
    nsSSLIOLayerMethods.fsync = (PRFsyncFN) _PSM_InvalidStatus;
    nsSSLIOLayerMethods.seek = (PRSeekFN) _PSM_InvalidInt;
    nsSSLIOLayerMethods.seek64 = (PRSeek64FN) _PSM_InvalidInt64;
    nsSSLIOLayerMethods.fileInfo = (PRFileInfoFN) _PSM_InvalidStatus;
    nsSSLIOLayerMethods.fileInfo64 = (PRFileInfo64FN) _PSM_InvalidStatus;
    nsSSLIOLayerMethods.writev = (PRWritevFN) _PSM_InvalidInt;
    nsSSLIOLayerMethods.accept = (PRAcceptFN) _PSM_InvalidDesc;
    nsSSLIOLayerMethods.listen = (PRListenFN) _PSM_InvalidStatus;
    nsSSLIOLayerMethods.shutdown = (PRShutdownFN) _PSM_InvalidStatus;
    nsSSLIOLayerMethods.recvfrom = (PRRecvfromFN) _PSM_InvalidInt;
    nsSSLIOLayerMethods.sendto = (PRSendtoFN) _PSM_InvalidInt;
    nsSSLIOLayerMethods.acceptread = (PRAcceptreadFN) _PSM_InvalidInt;
    nsSSLIOLayerMethods.transmitfile = (PRTransmitfileFN) _PSM_InvalidInt;
    nsSSLIOLayerMethods.sendfile = (PRSendfileFN) _PSM_InvalidInt;

    nsSSLIOLayerMethods.getsockname = PSMGetsockname;
    nsSSLIOLayerMethods.getpeername = PSMGetpeername;
    nsSSLIOLayerMethods.getsocketoption = PSMGetsocketoption;
    nsSSLIOLayerMethods.setsocketoption = PSMSetsocketoption;
    nsSSLIOLayerMethods.recv = PSMRecv;
    nsSSLIOLayerMethods.send = PSMSend;
    nsSSLIOLayerMethods.connectcontinue = PSMConnectcontinue;
    nsSSLIOLayerMethods.bind = PSMBind;

    nsSSLIOLayerMethods.connect = nsSSLIOLayerConnect;
    nsSSLIOLayerMethods.close = nsSSLIOLayerClose;
    nsSSLIOLayerMethods.write = nsSSLIOLayerWrite;
    nsSSLIOLayerMethods.read = nsSSLIOLayerRead;
    nsSSLIOLayerMethods.poll = nsSSLIOLayerPoll;

    nsSSLPlaintextLayerIdentity = PR_GetUniqueIdentity("Plaintxext PSM layer");
    nsSSLPlaintextLayerMethods  = *PR_GetDefaultIOMethods();
    nsSSLPlaintextLayerMethods.recv = PlaintextRecv;
  }

  bool enabled = false;
  Preferences::GetBool("security.ssl.treat_unsafe_negotiation_as_broken", &enabled);
  setTreatUnsafeNegotiationAsBroken(enabled);

  int32_t warnLevel = 1;
  Preferences::GetInt("security.ssl.warn_missing_rfc5746", &warnLevel);
  setWarnLevelMissingRFC5746(warnLevel);

  mFalseStartRequireNPN =
    Preferences::GetBool("security.ssl.false_start.require-npn",
                         FALSE_START_REQUIRE_NPN_DEFAULT);
  loadVersionFallbackLimit();
  nsCString insecureFallbackHosts;
  Preferences::GetCString("security.tls.insecure_fallback_hosts", &insecureFallbackHosts);
  setInsecureFallbackSites(insecureFallbackHosts);
  mUseStaticFallbackList =
    Preferences::GetBool("security.tls.insecure_fallback_hosts.use_static_list", true);
  mUnrestrictedRC4Fallback =
    Preferences::GetBool("security.tls.unrestricted_rc4_fallback", false);

  mPrefObserver = new PrefObserver(this);
  Preferences::AddStrongObserver(mPrefObserver,
                                 "security.ssl.treat_unsafe_negotiation_as_broken");
  Preferences::AddStrongObserver(mPrefObserver,
                                 "security.ssl.warn_missing_rfc5746");
  Preferences::AddStrongObserver(mPrefObserver,
                                 "security.ssl.false_start.require-npn");
  Preferences::AddStrongObserver(mPrefObserver,
                                 "security.tls.version.fallback-limit");
  Preferences::AddStrongObserver(mPrefObserver,
                                 "security.tls.insecure_fallback_hosts");
  Preferences::AddStrongObserver(mPrefObserver,
                                 "security.tls.unrestricted_rc4_fallback");
  return NS_OK;
}

void
nsSSLIOLayerHelpers::loadVersionFallbackLimit()
{
  
  uint32_t limit = Preferences::GetUint("security.tls.version.fallback-limit",
                                        3); 
  SSLVersionRange defaults = { SSL_LIBRARY_VERSION_TLS_1_2,
                               SSL_LIBRARY_VERSION_TLS_1_2 };
  SSLVersionRange filledInRange;
  nsNSSComponent::FillTLSVersionRange(filledInRange, limit, limit, defaults);

  mVersionFallbackLimit = filledInRange.max;
}

void
nsSSLIOLayerHelpers::clearStoredData()
{
  MutexAutoLock lock(mutex);
  mInsecureFallbackSites.Clear();
  mTLSIntoleranceInfo.Clear();
}

void
nsSSLIOLayerHelpers::setInsecureFallbackSites(const nsCString& str)
{
  MutexAutoLock lock(mutex);

  mInsecureFallbackSites.Clear();

  if (str.IsEmpty()) {
    return;
  }

  nsCCharSeparatedTokenizer toker(str, ',');

  while (toker.hasMoreTokens()) {
    const nsCSubstring& host = toker.nextToken();
    if (!host.IsEmpty()) {
      mInsecureFallbackSites.PutEntry(host);
    }
  }
}

struct FallbackListComparator
{
  explicit FallbackListComparator(const char* aTarget)
    : mTarget(aTarget)
  {}

  int operator()(const char* aVal) const {
    return strcmp(mTarget, aVal);
  }

private:
  const char* mTarget;
};

static const char* const kFallbackWildcardList[] =
{
  ".kuronekoyamato.co.jp", 
  ".userstorage.mega.co.nz", 
  ".wildcard.test",
};

bool
nsSSLIOLayerHelpers::isInsecureFallbackSite(const nsACString& hostname)
{
  size_t match;
  if (mUseStaticFallbackList) {
    const char* host = PromiseFlatCString(hostname).get();
    if (BinarySearchIf(kIntolerantFallbackList, 0,
          ArrayLength(kIntolerantFallbackList),
          FallbackListComparator(host), &match)) {
      return true;
    }
    for (size_t i = 0; i < ArrayLength(kFallbackWildcardList); ++i) {
      size_t hostLen = hostname.Length();
      const char* target = kFallbackWildcardList[i];
      size_t targetLen = strlen(target);
      if (hostLen > targetLen &&
          !memcmp(host + hostLen - targetLen, target, targetLen)) {
        return true;
      }
    }
  }
  MutexAutoLock lock(mutex);
  return mInsecureFallbackSites.Contains(hostname);
}

void
nsSSLIOLayerHelpers::setTreatUnsafeNegotiationAsBroken(bool broken)
{
  MutexAutoLock lock(mutex);
  mTreatUnsafeNegotiationAsBroken = broken;
}

bool
nsSSLIOLayerHelpers::treatUnsafeNegotiationAsBroken()
{
  MutexAutoLock lock(mutex);
  return mTreatUnsafeNegotiationAsBroken;
}

void
nsSSLIOLayerHelpers::setWarnLevelMissingRFC5746(int32_t level)
{
  MutexAutoLock lock(mutex);
  mWarnLevelMissingRFC5746 = level;
}

int32_t
nsSSLIOLayerHelpers::getWarnLevelMissingRFC5746()
{
  MutexAutoLock lock(mutex);
  return mWarnLevelMissingRFC5746;
}

nsresult
nsSSLIOLayerNewSocket(int32_t family,
                      const char* host,
                      int32_t port,
                      const char* proxyHost,
                      int32_t proxyPort,
                      PRFileDesc** fd,
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









static SECStatus
nsConvertCANamesToStrings(PLArenaPool* arena, char** caNameStrings,
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
                newitem.data = (unsigned char*) PR_Malloc(dername->len + 2);
                if (!newitem.data) {
                    goto loser;
                }
                newitem.data[0] = (unsigned char) 0x30;
                newitem.data[1] = (unsigned char) dername->len;
                (void) memcpy(&newitem.data[2], dername->data, dername->len);
            } else if (dername->len <= 255) {
                newitem.data = (unsigned char*) PR_Malloc(dername->len + 3);
                if (!newitem.data) {
                    goto loser;
                }
                newitem.data[0] = (unsigned char) 0x30;
                newitem.data[1] = (unsigned char) 0x81;
                newitem.data[2] = (unsigned char) dername->len;
                (void) memcpy(&newitem.data[3], dername->data, dername->len);
            } else {
                
                newitem.data = (unsigned char*) PR_Malloc(dername->len + 4);
                if (!newitem.data) {
                    goto loser;
                }
                newitem.data[0] = (unsigned char) 0x30;
                newitem.data[1] = (unsigned char) 0x82;
                newitem.data[2] = (unsigned char) ((dername->len >> 8) & 0xff);
                newitem.data[3] = (unsigned char) (dername->len & 0xff);
                memcpy(&newitem.data[4], dername->data, dername->len);
            }
            dername = &newitem;
        }

        namestring = CERT_DerNameToAscii(dername);
        if (!namestring) {
            
            caNameStrings[n] = const_cast<char*>("");
        } else {
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








nsresult
nsGetUserCertChoice(SSM_UserCertChoice* certChoice)
{
  char* mode = nullptr;
  nsresult ret;

  NS_ENSURE_ARG_POINTER(certChoice);

  nsCOMPtr<nsIPrefBranch> pref = do_GetService(NS_PREFSERVICE_CONTRACTID);

  ret = pref->GetCharPref("security.default_personal_cert", &mode);
  if (NS_FAILED(ret)) {
    goto loser;
  }

  if (PL_strcmp(mode, "Select Automatically") == 0) {
    *certChoice = AUTO;
  } else if (PL_strcmp(mode, "Ask Every Time") == 0) {
    *certChoice = ASK;
  } else {
    
    
    *certChoice = ASK;
  }

loser:
  if (mode) {
    free(mode);
  }
  return ret;
}

static bool
hasExplicitKeyUsageNonRepudiation(CERTCertificate* cert)
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
                         nsNSSSocketInfo* info,
                         CERTCertificate* serverCert)
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
  nsNSSSocketInfo* const mSocketInfo;   
  CERTCertificate* const mServerCert;   
};











SECStatus
nsNSS_SSLGetClientAuthData(void* arg, PRFileDesc* socket,
                           CERTDistNames* caNames, CERTCertificate** pRetCert,
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

void
ClientAuthDataRunnable::RunOnTargetThread()
{
  PLArenaPool* arena = nullptr;
  char** caNameStrings;
  ScopedCERTCertificate cert;
  ScopedSECKEYPrivateKey privKey;
  ScopedCERTCertList certList;
  CERTCertListNode* node;
  ScopedCERTCertNicknames nicknames;
  int keyError = 0; 
  SSM_UserCertChoice certChoice;
  int32_t NumberOfCerts = 0;
  void* wincx = mSocketInfo;
  nsresult rv;

  nsCOMPtr<nsIX509Cert> socketClientCert;
  mSocketInfo->GetClientCert(getter_AddRefs(socketClientCert));

  
  
  if (socketClientCert) {
    cert = socketClientCert->GetCert();
    if (!cert) {
      goto loser;
    }

    
    privKey = PK11_FindKeyByAnyCert(cert.get(), wincx);
    if (!privKey) {
      goto loser;
    }

    *mPRetCert = cert.forget();
    *mPRetKey = privKey.forget();
    mRV = SECSuccess;
    return;
  }

  
  arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
  if (!arena) {
    goto loser;
  }

  caNameStrings = (char**) PORT_ArenaAlloc(arena,
                                           sizeof(char*) * (mCANames->nnames));
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

    
    mRV = CERT_FilterCertListByCANames(certList.get(), mCANames->nnames,
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
      
      
      privKey = PK11_FindKeyByAnyCert(node->cert, wincx);
      if (privKey) {
        if (hasExplicitKeyUsageNonRepudiation(node->cert)) {
          privKey = nullptr;
          
          if (!low_prio_nonrep_cert) { 
            low_prio_nonrep_cert = CERT_DupCertificate(node->cert);
          }
        } else {
          
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
      privKey = PK11_FindKeyByAnyCert(cert.get(), wincx);
    }

    if (!cert) {
      goto noCert;
    }
  } else { 
    

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

    if (hasRemembered) {
      if (rememberedDBKey.IsEmpty()) {
        canceled = true;
      } else {
        nsCOMPtr<nsIX509CertDB> certdb;
        certdb = do_GetService(NS_X509CERTDB_CONTRACTID);
        if (certdb) {
          nsCOMPtr<nsIX509Cert> found_cert;
          nsresult find_rv =
            certdb->FindCertByDBKey(rememberedDBKey.get(), nullptr,
            getter_AddRefs(found_cert));
          if (NS_SUCCEEDED(find_rv) && found_cert) {
            nsNSSCertificate* obj_cert =
              reinterpret_cast<nsNSSCertificate*>(found_cert.get());
            if (obj_cert) {
              cert = obj_cert->GetCert();
            }
          }

          if (!cert) {
            hasRemembered = false;
          }
        }
      }
    }

    if (!hasRemembered) {
      
      nsIClientAuthDialogs* dialogs = nullptr;
      int32_t selectedIndex = -1;
      char16_t** certNicknameList = nullptr;
      char16_t** certDetailsList = nullptr;

      
      
      certList = CERT_FindUserCertsByUsage(CERT_GetDefaultCertDB(),
        certUsageSSLClient, false,
        false, wincx);
      if (!certList) {
        goto noCert;
      }

      if (mCANames->nnames != 0) {
        
        mRV = CERT_FilterCertListByCANames(certList.get(),
                                           mCANames->nnames,
                                           caNameStrings,
                                           certUsageSSLClient);
        if (mRV != SECSuccess) {
          goto loser;
        }
      }

      if (CERT_LIST_END(CERT_LIST_HEAD(certList), certList)) {
        
        goto noCert;
      }

      
      node = CERT_LIST_HEAD(certList.get());
      while (!CERT_LIST_END(node, certList.get())) {
        ++NumberOfCerts;
        node = CERT_LIST_NEXT(node);
      }
      if (CERT_LIST_END(CERT_LIST_HEAD(certList.get()), certList.get())) {
        goto noCert;
      }

      nicknames = getNSSCertNicknamesFromCertList(certList.get());

      if (!nicknames) {
        goto loser;
      }

      NS_ASSERTION(nicknames->numnicknames == NumberOfCerts, "nicknames->numnicknames != NumberOfCerts");

      
      UniquePtr<char, void(&)(void*)>
        ccn(CERT_GetCommonName(&mServerCert->subject), PORT_Free);
      NS_ConvertUTF8toUTF16 cn(ccn.get());

      int32_t port;
      mSocketInfo->GetPort(&port);

      nsString cn_host_port;
      if (ccn && strcmp(ccn.get(), hostname) == 0) {
        cn_host_port.Append(cn);
        cn_host_port.Append(':');
        cn_host_port.AppendInt(port);
      } else {
        cn_host_port.Append(cn);
        cn_host_port.AppendLiteral(" (");
        cn_host_port.Append(':');
        cn_host_port.AppendInt(port);
        cn_host_port.Append(')');
      }

      char* corg = CERT_GetOrgName(&mServerCert->subject);
      NS_ConvertUTF8toUTF16 org(corg);
      if (corg) PORT_Free(corg);

      char* cissuer = CERT_GetOrgName(&mServerCert->issuer);
      NS_ConvertUTF8toUTF16 issuer(cissuer);
      if (cissuer) PORT_Free(cissuer);

      certNicknameList =
        (char16_t**)moz_xmalloc(sizeof(char16_t*)* nicknames->numnicknames);
      if (!certNicknameList)
        goto loser;
      certDetailsList =
        (char16_t**)moz_xmalloc(sizeof(char16_t*)* nicknames->numnicknames);
      if (!certDetailsList) {
        free(certNicknameList);
        goto loser;
      }

      int32_t CertsToUse;
      for (CertsToUse = 0, node = CERT_LIST_HEAD(certList);
        !CERT_LIST_END(node, certList) && CertsToUse < nicknames->numnicknames;
        node = CERT_LIST_NEXT(node)
        ) {
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
          free(certNicknameList[CertsToUse]);
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
        } else {
          rv = dialogs->ChooseCertificate(mSocketInfo, cn_host_port.get(),
            org.get(), issuer.get(),
            (const char16_t**)certNicknameList,
            (const char16_t**)certDetailsList,
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

    
    privKey = PK11_FindKeyByAnyCert(cert.get(), wincx);
    if (!privKey) {
      keyError = PR_GetError();
      if (keyError == SEC_ERROR_BAD_PASSWORD) {
        
        goto loser;
      } else {
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
nsSSLIOLayerImportFD(PRFileDesc* fd,
                     nsNSSSocketInfo* infoObject,
                     const char* host)
{
  nsNSSShutDownPreventionLock locker;
  PRFileDesc* sslSock = SSL_ImportFD(nullptr, fd);
  if (!sslSock) {
    NS_ASSERTION(false, "NSS: Error importing socket");
    return nullptr;
  }
  SSL_SetPKCS11PinArg(sslSock, (nsIInterfaceRequestor*) infoObject);
  SSL_HandshakeCallback(sslSock, HandshakeCallback, infoObject);
  SSL_SetCanFalseStartCallback(sslSock, CanFalseStartCallback, infoObject);

  
  uint32_t flags = 0;
  infoObject->GetProviderFlags(&flags);
  if (flags & nsISocketProvider::ANONYMOUS_CONNECT) {
      SSL_GetClientAuthDataHook(sslSock, nullptr, infoObject);
  } else {
      SSL_GetClientAuthDataHook(sslSock,
                            (SSLGetClientAuthData) nsNSS_SSLGetClientAuthData,
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
nsSSLIOLayerSetOptions(PRFileDesc* fd, bool forSTARTTLS,
                       const char* proxyHost, const char* host, int32_t port,
                       nsNSSSocketInfo* infoObject)
{
  nsNSSShutDownPreventionLock locker;
  if (forSTARTTLS || proxyHost) {
    if (SECSuccess != SSL_OptionSet(fd, SSL_SECURITY, false)) {
      return NS_ERROR_FAILURE;
    }
  }

  SSLVersionRange range;
  if (SSL_VersionRangeGet(fd, &range) != SECSuccess) {
    return NS_ERROR_FAILURE;
  }

  uint16_t maxEnabledVersion = range.max;
  StrongCipherStatus strongCiphersStatus = StrongCipherStatusUnknown;
  infoObject->SharedState().IOLayerHelpers()
    .adjustForTLSIntolerance(infoObject->GetHostName(), infoObject->GetPort(),
                             range, strongCiphersStatus);
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
         ("[%p] nsSSLIOLayerSetOptions: using TLS version range (0x%04x,0x%04x)%s\n",
          fd, static_cast<unsigned int>(range.min),
              static_cast<unsigned int>(range.max),
          strongCiphersStatus == StrongCiphersFailed ? " with weak ciphers" : ""));

  if (SSL_VersionRangeSet(fd, &range) != SECSuccess) {
    return NS_ERROR_FAILURE;
  }
  infoObject->SetTLSVersionRange(range);

  if (strongCiphersStatus == StrongCiphersFailed) {
    nsNSSComponent::UseWeakCiphersOnSocket(fd);
  }

  
  
  if (range.max < maxEnabledVersion) {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
           ("[%p] nsSSLIOLayerSetOptions: enabling TLS_FALLBACK_SCSV\n", fd));
    if (SECSuccess != SSL_OptionSet(fd, SSL_ENABLE_FALLBACK_SCSV, true)) {
      return NS_ERROR_FAILURE;
    }
  }

  bool enabled = infoObject->SharedState().IsOCSPStaplingEnabled();
  if (SECSuccess != SSL_OptionSet(fd, SSL_ENABLE_OCSP_STAPLING, enabled)) {
    return NS_ERROR_FAILURE;
  }

  if (SECSuccess != SSL_OptionSet(fd, SSL_HANDSHAKE_AS_CLIENT, true)) {
    return NS_ERROR_FAILURE;
  }

  
  
  uint32_t flags = infoObject->GetProviderFlags();
  nsAutoCString peerId;
  if (flags & nsISocketProvider::ANONYMOUS_CONNECT) { 
    peerId.AppendLiteral("anon:");
  }
  if (flags & nsISocketProvider::NO_PERMANENT_STORAGE) {
    peerId.AppendLiteral("private:");
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
  PRFileDesc* plaintextLayer = nullptr;
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

  
  
  plaintextLayer = PR_CreateIOLayerStub(nsSSLIOLayerHelpers::nsSSLPlaintextLayerIdentity,
                                        &nsSSLIOLayerHelpers::nsSSLPlaintextLayerMethods);
  if (plaintextLayer) {
    plaintextLayer->secret = (PRFilePrivate*) infoObject;
    stat = PR_PushIOLayer(fd, PR_TOP_IO_LAYER, plaintextLayer);
    if (stat == PR_FAILURE) {
      plaintextLayer->dtor(plaintextLayer);
      plaintextLayer = nullptr;
    }
  }

  PRFileDesc* sslSock = nsSSLIOLayerImportFD(fd, infoObject, host);
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

  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("[%p] Socket set up\n", (void*) sslSock));
  infoObject->QueryInterface(NS_GET_IID(nsISupports), (void**) (info));

  
  if (forSTARTTLS || proxyHost) {
    infoObject->SetHandshakeNotPending();
  }

  infoObject->SharedState().NoteSocketCreated();

  return NS_OK;
 loser:
  NS_IF_RELEASE(infoObject);
  if (layer) {
    layer->dtor(layer);
  }
  if (plaintextLayer) {
    PR_PopIOLayer(fd, nsSSLIOLayerHelpers::nsSSLPlaintextLayerIdentity);
    plaintextLayer->dtor(plaintextLayer);
  }
  return NS_ERROR_FAILURE;
}
