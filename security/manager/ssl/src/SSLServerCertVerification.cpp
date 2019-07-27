





























































































#include "SSLServerCertVerification.h"

#include <cstring>

#include "pkix/pkixtypes.h"
#include "CertVerifier.h"
#include "CryptoTask.h"
#include "ExtendedValidation.h"
#include "NSSCertDBTrustDomain.h"
#include "nsIBadCertListener2.h"
#include "nsICertOverrideService.h"
#include "nsISiteSecurityService.h"
#include "nsNSSComponent.h"
#include "nsNSSIOLayer.h"
#include "nsNSSShutDown.h"

#include "mozilla/Assertions.h"
#include "mozilla/Mutex.h"
#include "mozilla/Telemetry.h"
#include "mozilla/unused.h"
#include "nsIThreadPool.h"
#include "nsNetUtil.h"
#include "nsXPCOMCIDInternal.h"
#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"
#include "PSMRunnable.h"
#include "SharedSSLState.h"
#include "nsContentUtils.h"

#include "ssl.h"
#include "secerr.h"
#include "secport.h"
#include "sslerr.h"

#ifdef PR_LOGGING
extern PRLogModuleInfo* gPIPNSSLog;
#endif

namespace mozilla { namespace psm {

namespace {


nsIThreadPool* gCertVerificationThreadPool = nullptr;




Mutex* gSSLVerificationTelemetryMutex = nullptr;


Mutex* gSSLVerificationPK11Mutex = nullptr;

} 











void
InitializeSSLServerCertVerificationThreads()
{
  gSSLVerificationTelemetryMutex = new Mutex("SSLVerificationTelemetryMutex");
  gSSLVerificationPK11Mutex = new Mutex("SSLVerificationPK11Mutex");
  
  
  
  nsresult rv = CallCreateInstance(NS_THREADPOOL_CONTRACTID,
                                   &gCertVerificationThreadPool);
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to create SSL cert verification threads.");
    return;
  }

  (void) gCertVerificationThreadPool->SetIdleThreadLimit(5);
  (void) gCertVerificationThreadPool->SetIdleThreadTimeout(30 * 1000);
  (void) gCertVerificationThreadPool->SetThreadLimit(5);
  (void) gCertVerificationThreadPool->SetName(NS_LITERAL_CSTRING("SSL Cert"));
}











void StopSSLServerCertVerificationThreads()
{
  if (gCertVerificationThreadPool) {
    gCertVerificationThreadPool->Shutdown();
    NS_RELEASE(gCertVerificationThreadPool);
  }
  if (gSSLVerificationTelemetryMutex) {
    delete gSSLVerificationTelemetryMutex;
    gSSLVerificationTelemetryMutex = nullptr;
  }
  if (gSSLVerificationPK11Mutex) {
    delete gSSLVerificationPK11Mutex;
    gSSLVerificationPK11Mutex = nullptr;
  }
}

namespace {

void
LogInvalidCertError(TransportSecurityInfo* socketInfo,
                    PRErrorCode errorCode,
                    ::mozilla::psm::SSLErrorMessageType errorMessageType)
{
  nsString message;
  socketInfo->GetErrorLogMessage(errorCode, errorMessageType, message);
  if (!message.IsEmpty()) {
    nsContentUtils::LogSimpleConsoleError(message, "SSL");
  }
}







class SSLServerCertVerificationResult : public nsRunnable
{
public:
  NS_DECL_NSIRUNNABLE

  SSLServerCertVerificationResult(TransportSecurityInfo* infoObject,
                                  PRErrorCode errorCode,
                                  Telemetry::ID telemetryID = Telemetry::HistogramCount,
                                  uint32_t telemetryValue = -1,
                                  SSLErrorMessageType errorMessageType =
                                      PlainErrorMessage);

  void Dispatch();
private:
  const RefPtr<TransportSecurityInfo> mInfoObject;
public:
  const PRErrorCode mErrorCode;
  const SSLErrorMessageType mErrorMessageType;
  const Telemetry::ID mTelemetryID;
  const uint32_t mTelemetryValue;
};

class CertErrorRunnable : public SyncRunnableBase
{
 public:
  CertErrorRunnable(const void* fdForLogging,
                    nsIX509Cert* cert,
                    TransportSecurityInfo* infoObject,
                    PRErrorCode defaultErrorCodeToReport,
                    uint32_t collectedErrors,
                    PRErrorCode errorCodeTrust,
                    PRErrorCode errorCodeMismatch,
                    PRErrorCode errorCodeExpired,
                    uint32_t providerFlags)
    : mFdForLogging(fdForLogging), mCert(cert), mInfoObject(infoObject),
      mDefaultErrorCodeToReport(defaultErrorCodeToReport),
      mCollectedErrors(collectedErrors),
      mErrorCodeTrust(errorCodeTrust),
      mErrorCodeMismatch(errorCodeMismatch),
      mErrorCodeExpired(errorCodeExpired),
      mProviderFlags(providerFlags)
  {
  }

  virtual void RunOnTargetThread();
  RefPtr<SSLServerCertVerificationResult> mResult; 
private:
  SSLServerCertVerificationResult* CheckCertOverrides();

  const void* const mFdForLogging; 
  const nsCOMPtr<nsIX509Cert> mCert;
  const RefPtr<TransportSecurityInfo> mInfoObject;
  const PRErrorCode mDefaultErrorCodeToReport;
  const uint32_t mCollectedErrors;
  const PRErrorCode mErrorCodeTrust;
  const PRErrorCode mErrorCodeMismatch;
  const PRErrorCode mErrorCodeExpired;
  const uint32_t mProviderFlags;
};


uint32_t
MapCertErrorToProbeValue(PRErrorCode errorCode)
{
  switch (errorCode)
  {
    case SEC_ERROR_UNKNOWN_ISSUER:                     return  2;
    case SEC_ERROR_UNTRUSTED_ISSUER:                   return  4;
    case SEC_ERROR_EXPIRED_ISSUER_CERTIFICATE:         return  5;
    case SEC_ERROR_UNTRUSTED_CERT:                     return  6;
    case SEC_ERROR_INADEQUATE_KEY_USAGE:               return  7;
    case SEC_ERROR_CERT_SIGNATURE_ALGORITHM_DISABLED:  return  8;
    case SSL_ERROR_BAD_CERT_DOMAIN:                    return  9;
    case SEC_ERROR_EXPIRED_CERTIFICATE:                return 10;
  }
  NS_WARNING("Unknown certificate error code. Does MapCertErrorToProbeValue "
             "handle everything in PRErrorCodeToOverrideType?");
  return 0;
}

SECStatus
DetermineCertOverrideErrors(CERTCertificate* cert, const char* hostName,
                            PRTime now, PRErrorCode defaultErrorCodeToReport,
                             uint32_t& collectedErrors,
                             PRErrorCode& errorCodeTrust,
                             PRErrorCode& errorCodeMismatch,
                             PRErrorCode& errorCodeExpired)
{
  MOZ_ASSERT(cert);
  MOZ_ASSERT(hostName);
  MOZ_ASSERT(collectedErrors == 0);
  MOZ_ASSERT(errorCodeTrust == 0);
  MOZ_ASSERT(errorCodeMismatch == 0);
  MOZ_ASSERT(errorCodeExpired == 0);

  
  
  
  switch (defaultErrorCodeToReport) {
    case SEC_ERROR_CERT_SIGNATURE_ALGORITHM_DISABLED:
    case SEC_ERROR_UNKNOWN_ISSUER:
    {
      collectedErrors = nsICertOverrideService::ERROR_UNTRUSTED;
      errorCodeTrust = defaultErrorCodeToReport;

      SECCertTimeValidity validity = CERT_CheckCertValidTimes(cert, now, false);
      if (validity == secCertTimeUndetermined) {
        PR_SetError(defaultErrorCodeToReport, 0);
        return SECFailure;
      }
      if (validity != secCertTimeValid) {
        collectedErrors |= nsICertOverrideService::ERROR_TIME;
        errorCodeExpired = SEC_ERROR_EXPIRED_CERTIFICATE;
      }
      break;
    }

    case SEC_ERROR_EXPIRED_CERTIFICATE:
      collectedErrors = nsICertOverrideService::ERROR_TIME;
      errorCodeExpired = SEC_ERROR_EXPIRED_CERTIFICATE;
      break;

    case SSL_ERROR_BAD_CERT_DOMAIN:
      collectedErrors = nsICertOverrideService::ERROR_MISMATCH;
      errorCodeMismatch = SSL_ERROR_BAD_CERT_DOMAIN;
      break;

    case 0:
      NS_ERROR("No error code set during certificate validation failure.");
      PR_SetError(PR_INVALID_STATE_ERROR, 0);
      return SECFailure;

    default:
      PR_SetError(defaultErrorCodeToReport, 0);
      return SECFailure;
  }

  if (defaultErrorCodeToReport != SSL_ERROR_BAD_CERT_DOMAIN) {
    if (CERT_VerifyCertName(cert, hostName) != SECSuccess) {
      if (PR_GetError() != SSL_ERROR_BAD_CERT_DOMAIN) {
        PR_SetError(defaultErrorCodeToReport, 0);
        return SECFailure;
      }

      collectedErrors |= nsICertOverrideService::ERROR_MISMATCH;
      errorCodeMismatch = SSL_ERROR_BAD_CERT_DOMAIN;
    }
  }

  return SECSuccess;
}

SSLServerCertVerificationResult*
CertErrorRunnable::CheckCertOverrides()
{
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("[%p][%p] top of CheckCertOverrides\n",
                                    mFdForLogging, this));
  
  
  unused << mFdForLogging;

  if (!NS_IsMainThread()) {
    NS_ERROR("CertErrorRunnable::CheckCertOverrides called off main thread");
    return new SSLServerCertVerificationResult(mInfoObject,
                                               mDefaultErrorCodeToReport);
  }

  int32_t port;
  mInfoObject->GetPort(&port);

  nsCString hostWithPortString;
  hostWithPortString.AppendASCII(mInfoObject->GetHostNameRaw());
  hostWithPortString.Append(':');
  hostWithPortString.AppendInt(port);

  uint32_t remaining_display_errors = mCollectedErrors;

  nsresult nsrv;

  
  
  
  bool strictTransportSecurityEnabled = false;
  nsCOMPtr<nsISiteSecurityService> sss
    = do_GetService(NS_SSSERVICE_CONTRACTID, &nsrv);
  if (NS_SUCCEEDED(nsrv)) {
    nsrv = sss->IsSecureHost(nsISiteSecurityService::HEADER_HSTS,
                             mInfoObject->GetHostNameRaw(),
                             mProviderFlags,
                             &strictTransportSecurityEnabled);
  }
  if (NS_FAILED(nsrv)) {
    return new SSLServerCertVerificationResult(mInfoObject,
                                               mDefaultErrorCodeToReport);
  }

  if (!strictTransportSecurityEnabled) {
    nsCOMPtr<nsICertOverrideService> overrideService =
      do_GetService(NS_CERTOVERRIDE_CONTRACTID);
    

    uint32_t overrideBits = 0;

    if (overrideService)
    {
      bool haveOverride;
      bool isTemporaryOverride; 
      nsCString hostString(mInfoObject->GetHostName());
      nsrv = overrideService->HasMatchingOverride(hostString, port,
                                                  mCert,
                                                  &overrideBits,
                                                  &isTemporaryOverride,
                                                  &haveOverride);
      if (NS_SUCCEEDED(nsrv) && haveOverride)
      {
       
        remaining_display_errors &= ~overrideBits;
      }
    }

    if (!remaining_display_errors) {
      
      
      
      if (mErrorCodeTrust != 0) {
        uint32_t probeValue = MapCertErrorToProbeValue(mErrorCodeTrust);
        Telemetry::Accumulate(Telemetry::SSL_CERT_ERROR_OVERRIDES, probeValue);
      }
      if (mErrorCodeMismatch != 0) {
        uint32_t probeValue = MapCertErrorToProbeValue(mErrorCodeMismatch);
        Telemetry::Accumulate(Telemetry::SSL_CERT_ERROR_OVERRIDES, probeValue);
      }
      if (mErrorCodeExpired != 0) {
        uint32_t probeValue = MapCertErrorToProbeValue(mErrorCodeExpired);
        Telemetry::Accumulate(Telemetry::SSL_CERT_ERROR_OVERRIDES, probeValue);
      }

      
      PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
             ("[%p][%p] All errors covered by override rules\n",
             mFdForLogging, this));
      return new SSLServerCertVerificationResult(mInfoObject, 0);
    }
  } else {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
           ("[%p][%p] Strict-Transport-Security is violated: untrusted "
            "transport layer\n", mFdForLogging, this));
  }

  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
         ("[%p][%p] Certificate error was not overridden\n",
         mFdForLogging, this));

  
  

  
  nsCOMPtr<nsISSLSocketControl> sslSocketControl = do_QueryInterface(
    NS_ISUPPORTS_CAST(nsITransportSecurityInfo*, mInfoObject));
  if (sslSocketControl) {
    nsCOMPtr<nsIInterfaceRequestor> cb;
    sslSocketControl->GetNotificationCallbacks(getter_AddRefs(cb));
    if (cb) {
      nsCOMPtr<nsIBadCertListener2> bcl = do_GetInterface(cb);
      if (bcl) {
        nsIInterfaceRequestor* csi
          = static_cast<nsIInterfaceRequestor*>(mInfoObject);
        bool suppressMessage = false; 
        nsrv = bcl->NotifyCertProblem(csi, mInfoObject->SSLStatus(),
                                      hostWithPortString, &suppressMessage);
      }
    }
  }

  
  PRErrorCode errorCodeToReport = mErrorCodeTrust    ? mErrorCodeTrust
                                : mErrorCodeMismatch ? mErrorCodeMismatch
                                : mErrorCodeExpired  ? mErrorCodeExpired
                                : mDefaultErrorCodeToReport;

  SSLServerCertVerificationResult* result =
    new SSLServerCertVerificationResult(mInfoObject,
                                        errorCodeToReport,
                                        Telemetry::HistogramCount,
                                        -1,
                                        OverridableCertErrorMessage);

  LogInvalidCertError(mInfoObject,
                      result->mErrorCode,
                      result->mErrorMessageType);

  return result;
}

void
CertErrorRunnable::RunOnTargetThread()
{
  MOZ_ASSERT(NS_IsMainThread());

  mResult = CheckCertOverrides();

  MOZ_ASSERT(mResult);
}



CertErrorRunnable*
CreateCertErrorRunnable(CertVerifier& certVerifier,
                        PRErrorCode defaultErrorCodeToReport,
                        TransportSecurityInfo* infoObject,
                        CERTCertificate* cert,
                        const void* fdForLogging,
                        uint32_t providerFlags,
                        PRTime now)
{
  MOZ_ASSERT(infoObject);
  MOZ_ASSERT(cert);

  uint32_t collected_errors = 0;
  PRErrorCode errorCodeTrust = 0;
  PRErrorCode errorCodeMismatch = 0;
  PRErrorCode errorCodeExpired = 0;
  if (DetermineCertOverrideErrors(cert, infoObject->GetHostNameRaw(), now,
                                  defaultErrorCodeToReport, collected_errors,
                                  errorCodeTrust, errorCodeMismatch,
                                  errorCodeExpired) != SECSuccess) {
    return nullptr;
  }

  RefPtr<nsNSSCertificate> nssCert(nsNSSCertificate::Create(cert));
  if (!nssCert) {
    NS_ERROR("nsNSSCertificate::Create failed");
    PR_SetError(SEC_ERROR_NO_MEMORY, 0);
    return nullptr;
  }

  if (!collected_errors) {
    
    
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("[%p] !collected_errors: %d\n",
           fdForLogging, static_cast<int>(defaultErrorCodeToReport)));
    PR_SetError(defaultErrorCodeToReport, 0);
    return nullptr;
  }

  infoObject->SetStatusErrorBits(*nssCert, collected_errors);

  return new CertErrorRunnable(fdForLogging,
                               static_cast<nsIX509Cert*>(nssCert.get()),
                               infoObject, defaultErrorCodeToReport,
                               collected_errors, errorCodeTrust,
                               errorCodeMismatch, errorCodeExpired,
                               providerFlags);
}










class CertErrorRunnableRunnable : public nsRunnable
{
public:
  CertErrorRunnableRunnable(CertErrorRunnable* certErrorRunnable)
    : mCertErrorRunnable(certErrorRunnable)
  {
  }
private:
  NS_IMETHOD Run()
  {
    nsresult rv = mCertErrorRunnable->DispatchToMainThreadAndWait();
    
    
    if (NS_SUCCEEDED(rv)) {
      rv = mCertErrorRunnable->mResult ? mCertErrorRunnable->mResult->Run()
                                       : NS_ERROR_UNEXPECTED;
    }
    return rv;
  }
  RefPtr<CertErrorRunnable> mCertErrorRunnable;
};

class SSLServerCertVerificationJob : public nsRunnable
{
public:
  
  static SECStatus Dispatch(const RefPtr<SharedCertVerifier>& certVerifier,
                            const void* fdForLogging,
                            TransportSecurityInfo* infoObject,
                            CERTCertificate* serverCert,
                            SECItem* stapledOCSPResponse,
                            uint32_t providerFlags,
                            PRTime time);
private:
  NS_DECL_NSIRUNNABLE

  
  SSLServerCertVerificationJob(const RefPtr<SharedCertVerifier>& certVerifier,
                               const void* fdForLogging,
                               TransportSecurityInfo* infoObject,
                               CERTCertificate* cert,
                               SECItem* stapledOCSPResponse,
                               uint32_t providerFlags,
                               PRTime time);
  const RefPtr<SharedCertVerifier> mCertVerifier;
  const void* const mFdForLogging;
  const RefPtr<TransportSecurityInfo> mInfoObject;
  const ScopedCERTCertificate mCert;
  const uint32_t mProviderFlags;
  const PRTime mTime;
  const TimeStamp mJobStartTime;
  const ScopedSECItem mStapledOCSPResponse;
};

SSLServerCertVerificationJob::SSLServerCertVerificationJob(
    const RefPtr<SharedCertVerifier>& certVerifier, const void* fdForLogging,
    TransportSecurityInfo* infoObject, CERTCertificate* cert,
    SECItem* stapledOCSPResponse, uint32_t providerFlags, PRTime time)
  : mCertVerifier(certVerifier)
  , mFdForLogging(fdForLogging)
  , mInfoObject(infoObject)
  , mCert(CERT_DupCertificate(cert))
  , mProviderFlags(providerFlags)
  , mTime(time)
  , mJobStartTime(TimeStamp::Now())
  , mStapledOCSPResponse(SECITEM_DupItem(stapledOCSPResponse))
{
}












static SECStatus
BlockServerCertChangeForSpdy(nsNSSSocketInfo* infoObject,
                             CERTCertificate* serverCert)
{
  
  
  nsCOMPtr<nsIX509Cert> cert;

  RefPtr<nsSSLStatus> status(infoObject->SSLStatus());
  if (!status) {
    
    
    
    return SECSuccess;
  }

  status->GetServerCert(getter_AddRefs(cert));
  if (!cert) {
    NS_NOTREACHED("every nsSSLStatus must have a cert"
                  "that implements nsIX509Cert");
    PR_SetError(SEC_ERROR_LIBRARY_FAILURE, 0);
    return SECFailure;
  }

  
  nsAutoCString negotiatedNPN;
  nsresult rv = infoObject->GetNegotiatedNPN(negotiatedNPN);
  NS_ASSERTION(NS_SUCCEEDED(rv),
               "GetNegotiatedNPN() failed during renegotiation");

  if (NS_SUCCEEDED(rv) && !StringBeginsWith(negotiatedNPN,
                                            NS_LITERAL_CSTRING("spdy/"))) {
    return SECSuccess;
  }
  
  if (NS_FAILED(rv)) {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
           ("BlockServerCertChangeForSpdy failed GetNegotiatedNPN() call."
            " Assuming spdy.\n"));
  }

  
  ScopedCERTCertificate c(cert->GetCert());
  NS_ASSERTION(c, "very bad and hopefully impossible state");
  bool sameCert = CERT_CompareCerts(c, serverCert);
  if (sameCert) {
    return SECSuccess;
  }

  
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
         ("SPDY Refused to allow new cert during renegotiation\n"));
  PR_SetError(SSL_ERROR_RENEGOTIATION_NOT_ALLOWED, 0);
  return SECFailure;
}

SECStatus
AuthCertificate(CertVerifier& certVerifier, TransportSecurityInfo* infoObject,
                CERTCertificate* cert, SECItem* stapledOCSPResponse,
                uint32_t providerFlags, PRTime time)
{
  MOZ_ASSERT(infoObject);
  MOZ_ASSERT(cert);

  SECStatus rv;

  
  
  bool saveIntermediates =
    !(providerFlags & nsISocketProvider::NO_PERMANENT_STORAGE);

  ScopedCERTCertList certList;
  SECOidTag evOidPolicy;
  rv = certVerifier.VerifySSLServerCert(cert, stapledOCSPResponse,
                                        time, infoObject,
                                        infoObject->GetHostNameRaw(),
                                        saveIntermediates, nullptr,
                                        &evOidPolicy);

  
  
  

  RefPtr<nsSSLStatus> status(infoObject->SSLStatus());
  RefPtr<nsNSSCertificate> nsc;

  if (!status || !status->mServerCert) {
    if( rv == SECSuccess ){
      nsc = nsNSSCertificate::Create(cert, &evOidPolicy);
    }
    else {
      nsc = nsNSSCertificate::Create(cert);
    }
  }

  if (rv == SECSuccess) {
    
    
    
    if (!status) {
      status = new nsSSLStatus();
      infoObject->SetSSLStatus(status);
    }

    if (rv == SECSuccess) {
      
      
      RememberCertErrorsTable::GetInstance().RememberCertHasError(infoObject,
                                                                  nullptr, rv);
    }
    else {
      
      RememberCertErrorsTable::GetInstance().LookupCertErrorBits(
        infoObject, status);
    }

    if (status && !status->mServerCert) {
      status->mServerCert = nsc;
      PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
             ("AuthCertificate setting NEW cert %p\n", status->mServerCert.get()));
    }
  }

  return rv;
}

 SECStatus
SSLServerCertVerificationJob::Dispatch(
  const RefPtr<SharedCertVerifier>& certVerifier,
  const void* fdForLogging,
  TransportSecurityInfo* infoObject,
  CERTCertificate* serverCert,
  SECItem* stapledOCSPResponse,
  uint32_t providerFlags,
  PRTime time)
{
  
  if (!certVerifier || !infoObject || !serverCert) {
    NS_ERROR("Invalid parameters for SSL server cert validation");
    PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
    return SECFailure;
  }

  RefPtr<SSLServerCertVerificationJob> job(
    new SSLServerCertVerificationJob(certVerifier, fdForLogging, infoObject,
                                     serverCert, stapledOCSPResponse,
                                     providerFlags, time));

  nsresult nrv;
  if (!gCertVerificationThreadPool) {
    nrv = NS_ERROR_NOT_INITIALIZED;
  } else {
    nrv = gCertVerificationThreadPool->Dispatch(job, NS_DISPATCH_NORMAL);
  }
  if (NS_FAILED(nrv)) {
    
    
    
    
    
    
    
    PRErrorCode error = nrv == NS_ERROR_OUT_OF_MEMORY
                      ? SEC_ERROR_NO_MEMORY
                      : PR_INVALID_STATE_ERROR;
    PORT_SetError(error);
    return SECFailure;
  }

  PORT_SetError(PR_WOULD_BLOCK_ERROR);
  return SECWouldBlock;
}

NS_IMETHODIMP
SSLServerCertVerificationJob::Run()
{
  

  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
          ("[%p] SSLServerCertVerificationJob::Run\n", mInfoObject.get()));

  PRErrorCode error;

  nsNSSShutDownPreventionLock nssShutdownPrevention;
  if (mInfoObject->isAlreadyShutDown()) {
    error = SEC_ERROR_USER_CANCELLED;
  } else {
    Telemetry::ID successTelemetry
      = Telemetry::SSL_SUCCESFUL_CERT_VALIDATION_TIME_MOZILLAPKIX;
    Telemetry::ID failureTelemetry
      = Telemetry::SSL_INITIAL_FAILED_CERT_VALIDATION_TIME_MOZILLAPKIX;

    
    
    PR_SetError(0, 0);
    SECStatus rv = AuthCertificate(*mCertVerifier, mInfoObject, mCert.get(),
                                   mStapledOCSPResponse, mProviderFlags,
                                   mTime);
    if (rv == SECSuccess) {
      uint32_t interval = (uint32_t) ((TimeStamp::Now() - mJobStartTime).ToMilliseconds());
      RefPtr<SSLServerCertVerificationResult> restart(
        new SSLServerCertVerificationResult(mInfoObject, 0,
                                            successTelemetry, interval));
      restart->Dispatch();
      Telemetry::Accumulate(Telemetry::SSL_CERT_ERROR_OVERRIDES, 1);
      return NS_OK;
    }

    
    
    error = PR_GetError();
    {
      TimeStamp now = TimeStamp::Now();
      MutexAutoLock telemetryMutex(*gSSLVerificationTelemetryMutex);
      Telemetry::AccumulateTimeDelta(failureTelemetry, mJobStartTime, now);
    }
    if (error != 0) {
      RefPtr<CertErrorRunnable> runnable(
          CreateCertErrorRunnable(*mCertVerifier, error, mInfoObject,
                                  mCert.get(), mFdForLogging, mProviderFlags,
                                  mTime));
      if (!runnable) {
        
        error = PR_GetError();
      } else {
        
        
        
        

        PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
                ("[%p][%p] Before dispatching CertErrorRunnable\n",
                mFdForLogging, runnable.get()));

        nsresult nrv;
        nsCOMPtr<nsIEventTarget> stsTarget
          = do_GetService(NS_SOCKETTRANSPORTSERVICE_CONTRACTID, &nrv);
        if (NS_SUCCEEDED(nrv)) {
          nrv = stsTarget->Dispatch(new CertErrorRunnableRunnable(runnable),
                                    NS_DISPATCH_NORMAL);
        }
        if (NS_SUCCEEDED(nrv)) {
          return NS_OK;
        }

        NS_ERROR("Failed to dispatch CertErrorRunnable");
        error = PR_INVALID_STATE_ERROR;
      }
    }
  }

  if (error == 0) {
    NS_NOTREACHED("no error set during certificate validation failure");
    error = PR_INVALID_STATE_ERROR;
  }

  RefPtr<SSLServerCertVerificationResult> failure(
    new SSLServerCertVerificationResult(mInfoObject, error));
  failure->Dispatch();
  return NS_OK;
}

} 




SECStatus
AuthCertificateHook(void* arg, PRFileDesc* fd, PRBool checkSig, PRBool isServer)
{
  RefPtr<SharedCertVerifier> certVerifier(GetDefaultCertVerifier());
  if (!certVerifier) {
    PR_SetError(SEC_ERROR_NOT_INITIALIZED, 0);
    return SECFailure;
  }

  

  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
         ("[%p] starting AuthCertificateHook\n", fd));

  
  
  NS_ASSERTION(checkSig, "AuthCertificateHook: checkSig unexpectedly false");

  
  
  NS_ASSERTION(!isServer, "AuthCertificateHook: isServer unexpectedly true");

  nsNSSSocketInfo* socketInfo = static_cast<nsNSSSocketInfo*>(arg);

  ScopedCERTCertificate serverCert(SSL_PeerCertificate(fd));

  if (!checkSig || isServer || !socketInfo || !serverCert) {
      PR_SetError(PR_INVALID_STATE_ERROR, 0);
      return SECFailure;
  }

  socketInfo->SetFullHandshake();

  
  
  PRTime now = PR_Now();

  if (BlockServerCertChangeForSpdy(socketInfo, serverCert) != SECSuccess)
    return SECFailure;

  bool onSTSThread;
  nsresult nrv;
  nsCOMPtr<nsIEventTarget> sts
    = do_GetService(NS_SOCKETTRANSPORTSERVICE_CONTRACTID, &nrv);
  if (NS_SUCCEEDED(nrv)) {
    nrv = sts->IsOnCurrentThread(&onSTSThread);
  }

  if (NS_FAILED(nrv)) {
    NS_ERROR("Could not get STS service or IsOnCurrentThread failed");
    PR_SetError(PR_UNKNOWN_ERROR, 0);
    return SECFailure;
  }

  
  
  
  
  const SECItemArray* csa = SSL_PeerStapledOCSPResponses(fd);
  SECItem* stapledOCSPResponse = nullptr;
  
  if (csa && csa->len == 1) {
    stapledOCSPResponse = &csa->items[0];
  }

  uint32_t providerFlags = 0;
  socketInfo->GetProviderFlags(&providerFlags);

  if (onSTSThread) {

    
    
    
    
    socketInfo->SetCertVerificationWaiting();
    SECStatus rv = SSLServerCertVerificationJob::Dispatch(
                     certVerifier, static_cast<const void*>(fd), socketInfo,
                     serverCert, stapledOCSPResponse, providerFlags, now);
    return rv;
  }

  
  
  
  

  SECStatus rv = AuthCertificate(*certVerifier, socketInfo, serverCert,
                                 stapledOCSPResponse, providerFlags, now);
  if (rv == SECSuccess) {
    Telemetry::Accumulate(Telemetry::SSL_CERT_ERROR_OVERRIDES, 1);
    return SECSuccess;
  }

  PRErrorCode error = PR_GetError();
  if (error != 0) {
    RefPtr<CertErrorRunnable> runnable(
        CreateCertErrorRunnable(*certVerifier, error, socketInfo, serverCert,
                                static_cast<const void*>(fd), providerFlags,
                                now));
    if (!runnable) {
      
      error = PR_GetError();
    } else {
      
      
      
      
      nrv = runnable->DispatchToMainThreadAndWait();
      if (NS_FAILED(nrv)) {
        NS_ERROR("Failed to dispatch CertErrorRunnable");
        PR_SetError(PR_INVALID_STATE_ERROR, 0);
        return SECFailure;
      }

      if (!runnable->mResult) {
        NS_ERROR("CertErrorRunnable did not set result");
        PR_SetError(PR_INVALID_STATE_ERROR, 0);
        return SECFailure;
      }

      if (runnable->mResult->mErrorCode == 0) {
        return SECSuccess; 
      }

      
      
      
      
      
      socketInfo->SetCanceled(runnable->mResult->mErrorCode,
                              runnable->mResult->mErrorMessageType);
      error = runnable->mResult->mErrorCode;
    }
  }

  if (error == 0) {
    NS_ERROR("error code not set");
    error = PR_UNKNOWN_ERROR;
  }

  PR_SetError(error, 0);
  return SECFailure;
}

#ifndef MOZ_NO_EV_CERTS
class InitializeIdentityInfo : public CryptoTask
{
  virtual nsresult CalculateResult() MOZ_OVERRIDE
  {
    EnsureIdentityInfoLoaded();
    return NS_OK;
  }

  virtual void ReleaseNSSResources() MOZ_OVERRIDE { } 
  virtual void CallCallback(nsresult rv) MOZ_OVERRIDE { } 
};
#endif

void EnsureServerVerificationInitialized()
{
#ifndef MOZ_NO_EV_CERTS
  
  

  static bool triggeredCertVerifierInit = false;
  if (triggeredCertVerifierInit)
    return;
  triggeredCertVerifierInit = true;

  RefPtr<InitializeIdentityInfo> initJob = new InitializeIdentityInfo();
  if (gCertVerificationThreadPool)
    gCertVerificationThreadPool->Dispatch(initJob, NS_DISPATCH_NORMAL);
#endif
}

SSLServerCertVerificationResult::SSLServerCertVerificationResult(
        TransportSecurityInfo* infoObject, PRErrorCode errorCode,
        Telemetry::ID telemetryID, uint32_t telemetryValue,
        SSLErrorMessageType errorMessageType)
  : mInfoObject(infoObject)
  , mErrorCode(errorCode)
  , mErrorMessageType(errorMessageType)
  , mTelemetryID(telemetryID)
  , mTelemetryValue(telemetryValue)
{




MOZ_ASSERT(telemetryID == Telemetry::HistogramCount || errorCode == 0);
}

void
SSLServerCertVerificationResult::Dispatch()
{
  nsresult rv;
  nsCOMPtr<nsIEventTarget> stsTarget
    = do_GetService(NS_SOCKETTRANSPORTSERVICE_CONTRACTID, &rv);
  NS_ASSERTION(stsTarget,
               "Failed to get socket transport service event target");
  rv = stsTarget->Dispatch(this, NS_DISPATCH_NORMAL);
  NS_ASSERTION(NS_SUCCEEDED(rv),
               "Failed to dispatch SSLServerCertVerificationResult");
}

NS_IMETHODIMP
SSLServerCertVerificationResult::Run()
{
  
  if (mTelemetryID != Telemetry::HistogramCount) {
     Telemetry::Accumulate(mTelemetryID, mTelemetryValue);
  }
  
  ((nsNSSSocketInfo*) mInfoObject.get())
    ->SetCertVerificationResult(mErrorCode, mErrorMessageType);
  return NS_OK;
}

} } 
