































































































#include "SSLServerCertVerification.h"
#include "CertVerifier.h"
#include "nsIBadCertListener2.h"
#include "nsICertOverrideService.h"
#include "nsIStrictTransportSecurityService.h"
#include "nsNSSComponent.h"
#include "nsNSSCleaner.h"
#include "nsRecentBadCerts.h"
#include "nsNSSIOLayer.h"
#include "nsNSSShutDown.h"

#include "mozilla/Assertions.h"
#include "mozilla/Mutex.h"
#include "mozilla/Telemetry.h"
#include "nsIThreadPool.h"
#include "nsNetUtil.h"
#include "nsXPCOMCIDInternal.h"
#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsIConsoleService.h"
#include "PSMRunnable.h"
#include "SharedSSLState.h"

#include "ssl.h"
#include "secerr.h"
#include "secport.h"
#include "sslerr.h"
#include "ocsp.h"

#ifdef PR_LOGGING
extern PRLogModuleInfo* gPIPNSSLog;
#endif

namespace mozilla { namespace psm {

namespace {

NS_DEFINE_CID(kNSSComponentCID, NS_NSSCOMPONENT_CID);

NSSCleanupAutoPtrClass(CERTCertificate, CERT_DestroyCertificate)
NSSCleanupAutoPtrClass_WithParam(PLArenaPool, PORT_FreeArena, FalseParam, false)


nsIThreadPool * gCertVerificationThreadPool = nullptr;




Mutex *gSSLVerificationTelemetryMutex = nullptr;

} 











void
InitializeSSLServerCertVerificationThreads()
{
  gSSLVerificationTelemetryMutex = new Mutex("SSLVerificationTelemetryMutex");
  
  
  
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
}

namespace {

void
LogInvalidCertError(TransportSecurityInfo *socketInfo, 
                    const nsACString &host, 
                    const nsACString &hostWithPort,
                    int32_t port,
                    PRErrorCode errorCode,
                    ::mozilla::psm::SSLErrorMessageType errorMessageType,
                    nsIX509Cert* ix509)
{
  nsString message;
  socketInfo->GetErrorLogMessage(errorCode, errorMessageType, message);
  
  if (!message.IsEmpty()) {
    nsCOMPtr<nsIConsoleService> console;
    console = do_GetService(NS_CONSOLESERVICE_CONTRACTID);
    if (console) {
      console->LogStringMessage(message.get());
    }
  }
}







class SSLServerCertVerificationResult : public nsRunnable
{
public:
  NS_DECL_NSIRUNNABLE

  SSLServerCertVerificationResult(TransportSecurityInfo * infoObject,
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
  CertErrorRunnable(const void * fdForLogging,
                    nsIX509Cert * cert,
                    TransportSecurityInfo * infoObject,
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
  SSLServerCertVerificationResult *CheckCertOverrides();
  
  const void * const mFdForLogging; 
  const nsCOMPtr<nsIX509Cert> mCert;
  const RefPtr<TransportSecurityInfo> mInfoObject;
  const PRErrorCode mDefaultErrorCodeToReport;
  const uint32_t mCollectedErrors;
  const PRErrorCode mErrorCodeTrust;
  const PRErrorCode mErrorCodeMismatch;
  const PRErrorCode mErrorCodeExpired;
  const uint32_t mProviderFlags;
};

SSLServerCertVerificationResult *
CertErrorRunnable::CheckCertOverrides()
{
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("[%p][%p] top of CheckCertOverrides\n",
                                    mFdForLogging, this));

  if (!NS_IsMainThread()) {
    NS_ERROR("CertErrorRunnable::CheckCertOverrides called off main thread");
    return new SSLServerCertVerificationResult(mInfoObject,
                                               mDefaultErrorCodeToReport);
  }

  int32_t port;
  mInfoObject->GetPort(&port);

  nsCString hostWithPortString;
  hostWithPortString.AppendASCII(mInfoObject->GetHostName());
  hostWithPortString.AppendLiteral(":");
  hostWithPortString.AppendInt(port);

  uint32_t remaining_display_errors = mCollectedErrors;

  nsresult nsrv;

  
  
  
  bool strictTransportSecurityEnabled = false;
  nsCOMPtr<nsIStrictTransportSecurityService> stss
    = do_GetService(NS_STSSERVICE_CONTRACTID, &nsrv);
  if (NS_SUCCEEDED(nsrv)) {
    nsCOMPtr<nsISSLSocketControl> sslSocketControl = do_QueryInterface(
      NS_ISUPPORTS_CAST(nsITransportSecurityInfo*, mInfoObject));
    nsrv = stss->IsStsHost(mInfoObject->GetHostName(),
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
        nsIInterfaceRequestor *csi
          = static_cast<nsIInterfaceRequestor*>(mInfoObject);
        bool suppressMessage = false; 
        nsrv = bcl->NotifyCertProblem(csi, mInfoObject->SSLStatus(),
                                      hostWithPortString, &suppressMessage);
      }
    }
  }

  nsCOMPtr<nsIX509CertDB> certdb = do_GetService(NS_X509CERTDB_CONTRACTID);
  nsCOMPtr<nsIRecentBadCerts> recentBadCertsService;
  if (certdb) {
    bool isPrivate = mProviderFlags & nsISocketProvider::NO_PERMANENT_STORAGE;
    certdb->GetRecentBadCerts(isPrivate, getter_AddRefs(recentBadCertsService));
  }
 
  if (recentBadCertsService) {
    NS_ConvertUTF8toUTF16 hostWithPortStringUTF16(hostWithPortString);
    recentBadCertsService->AddBadCert(hostWithPortStringUTF16,
                                      mInfoObject->SSLStatus());
  }

  
  PRErrorCode errorCodeToReport = mErrorCodeTrust    ? mErrorCodeTrust
                                : mErrorCodeMismatch ? mErrorCodeMismatch
                                : mErrorCodeExpired  ? mErrorCodeExpired
                                : mDefaultErrorCodeToReport;
                                
  SSLServerCertVerificationResult *result = 
    new SSLServerCertVerificationResult(mInfoObject, 
                                        errorCodeToReport,
                                        Telemetry::HistogramCount,
                                        -1,
                                        OverridableCertErrorMessage);

  LogInvalidCertError(mInfoObject,
                      nsDependentCString(mInfoObject->GetHostName()),
                      hostWithPortString,
                      port,
                      result->mErrorCode,
                      result->mErrorMessageType,
                      mCert);

  return result;
}

void 
CertErrorRunnable::RunOnTargetThread()
{
  MOZ_ASSERT(NS_IsMainThread());

  mResult = CheckCertOverrides();
  
  MOZ_ASSERT(mResult);
}



CertErrorRunnable *
CreateCertErrorRunnable(PRErrorCode defaultErrorCodeToReport,
                        TransportSecurityInfo * infoObject,
                        CERTCertificate * cert,
                        const void * fdForLogging,
                        uint32_t providerFlags,
                        PRTime now)
{
  MOZ_ASSERT(infoObject);
  MOZ_ASSERT(cert);
  
  
  if (defaultErrorCodeToReport == SEC_ERROR_REVOKED_CERTIFICATE) {
    PR_SetError(SEC_ERROR_REVOKED_CERTIFICATE, 0);
    return nullptr;
  }

  if (defaultErrorCodeToReport == 0) {
    NS_ERROR("No error code set during certificate validation failure.");
    PR_SetError(PR_INVALID_STATE_ERROR, 0);
    return nullptr;
  }

  RefPtr<nsNSSCertificate> nssCert(nsNSSCertificate::Create(cert));
  if (!nssCert) {
    NS_ERROR("nsNSSCertificate::Create failed");
    PR_SetError(SEC_ERROR_NO_MEMORY, 0);
    return nullptr;
  }

  SECStatus srv;

  RefPtr<CertVerifier> certVerifier(GetDefaultCertVerifier());
  if (!certVerifier) {
    NS_ERROR("GetDefaultCerVerifier failed");
    PR_SetError(defaultErrorCodeToReport, 0);
    return nullptr;
  }
  
  PLArenaPool *log_arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
  PLArenaPoolCleanerFalseParam log_arena_cleaner(log_arena);
  if (!log_arena) {
    NS_ERROR("PORT_NewArena failed");
    return nullptr; 
  }

  CERTVerifyLog * verify_log = PORT_ArenaZNew(log_arena, CERTVerifyLog);
  if (!verify_log) {
    NS_ERROR("PORT_ArenaZNew failed");
    return nullptr; 
  }
  CERTVerifyLogContentsCleaner verify_log_cleaner(verify_log);
  verify_log->arena = log_arena;

  srv = certVerifier->VerifyCert(cert, certificateUsageSSLServer, now,
                                 infoObject, 0, nullptr, nullptr, verify_log);

  
  
  
  
  

  PRErrorCode errorCodeMismatch = 0;
  PRErrorCode errorCodeTrust = 0;
  PRErrorCode errorCodeExpired = 0;

  uint32_t collected_errors = 0;

  if (infoObject->IsCertIssuerBlacklisted()) {
    collected_errors |= nsICertOverrideService::ERROR_UNTRUSTED;
    errorCodeTrust = defaultErrorCodeToReport;
  }

  
  if (CERT_VerifyCertName(cert, infoObject->GetHostName()) != SECSuccess) {
    collected_errors |= nsICertOverrideService::ERROR_MISMATCH;
    errorCodeMismatch = SSL_ERROR_BAD_CERT_DOMAIN;
  }

  CERTVerifyLogNode *i_node;
  for (i_node = verify_log->head; i_node; i_node = i_node->next)
  {
    switch (i_node->error)
    {
      case SEC_ERROR_UNKNOWN_ISSUER:
      case SEC_ERROR_CA_CERT_INVALID:
      case SEC_ERROR_UNTRUSTED_ISSUER:
      case SEC_ERROR_EXPIRED_ISSUER_CERTIFICATE:
      case SEC_ERROR_UNTRUSTED_CERT:
      case SEC_ERROR_INADEQUATE_KEY_USAGE:
      case SEC_ERROR_CERT_SIGNATURE_ALGORITHM_DISABLED:
        
        collected_errors |= nsICertOverrideService::ERROR_UNTRUSTED;
        if (errorCodeTrust == SECSuccess) {
          errorCodeTrust = i_node->error;
        }
        break;
      case SSL_ERROR_BAD_CERT_DOMAIN:
        collected_errors |= nsICertOverrideService::ERROR_MISMATCH;
        if (errorCodeMismatch == SECSuccess) {
          errorCodeMismatch = i_node->error;
        }
        break;
      case SEC_ERROR_EXPIRED_CERTIFICATE:
        collected_errors |= nsICertOverrideService::ERROR_TIME;
        if (errorCodeExpired == SECSuccess) {
          errorCodeExpired = i_node->error;
        }
        break;
      default:
        PR_SetError(i_node->error, 0);
        return nullptr;
    }
  }

  if (!collected_errors)
  {
    
    
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
  CertErrorRunnableRunnable(CertErrorRunnable * certErrorRunnable)
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
  
  static SECStatus Dispatch(const void * fdForLogging,
                            TransportSecurityInfo * infoObject,
                            CERTCertificate * serverCert,
                            uint32_t providerFlags);
private:
  NS_DECL_NSIRUNNABLE

  
  SSLServerCertVerificationJob(const void * fdForLogging,
                               TransportSecurityInfo * infoObject, 
                               CERTCertificate * cert,
                               uint32_t providerFlags);
  const void * const mFdForLogging;
  const RefPtr<TransportSecurityInfo> mInfoObject;
  const ScopedCERTCertificate mCert;
  const uint32_t mProviderFlags;
  const TimeStamp mJobStartTime;
};

SSLServerCertVerificationJob::SSLServerCertVerificationJob(
    const void * fdForLogging, TransportSecurityInfo * infoObject,
    CERTCertificate * cert, uint32_t providerFlags)
  : mFdForLogging(fdForLogging)
  , mInfoObject(infoObject)
  , mCert(CERT_DupCertificate(cert))
  , mProviderFlags(providerFlags)
  , mJobStartTime(TimeStamp::Now())
{
}

SECStatus
PSM_SSL_PKIX_AuthCertificate(CERTCertificate *peerCert,
                             nsIInterfaceRequestor * pinarg,
                             const char * hostname,
                             CERTCertList **validationChain,
                             SECOidTag *evOidPolicy)
{
    RefPtr<CertVerifier> certVerifier(GetDefaultCertVerifier());
    if (!certVerifier) {
      PR_SetError(PR_INVALID_STATE_ERROR, 0);
      return SECFailure;
    }

    SECStatus rv = certVerifier->VerifyCert(peerCert,
                                            certificateUsageSSLServer, PR_Now(),
                                            pinarg, 0, validationChain , evOidPolicy);

    if (rv == SECSuccess) {
        



        if (hostname && hostname[0])
            rv = CERT_VerifyCertName(peerCert, hostname);
        else
            rv = SECFailure;
        if (rv != SECSuccess)
            PORT_SetError(SSL_ERROR_BAD_CERT_DOMAIN);
    }
        
    return rv;
}

struct nsSerialBinaryBlacklistEntry
{
  unsigned int len;
  const char *binary_serial;
};


static struct nsSerialBinaryBlacklistEntry myUTNBlacklistEntries[] = {
  { 17, "\x00\x92\x39\xd5\x34\x8f\x40\xd1\x69\x5a\x74\x54\x70\xe1\xf2\x3f\x43" },
  { 17, "\x00\xd8\xf3\x5f\x4e\xb7\x87\x2b\x2d\xab\x06\x92\xe3\x15\x38\x2f\xb0" },
  { 16, "\x72\x03\x21\x05\xc5\x0c\x08\x57\x3d\x8e\xa5\x30\x4e\xfe\xe8\xb0" },
  { 17, "\x00\xb0\xb7\x13\x3e\xd0\x96\xf9\xb5\x6f\xae\x91\xc8\x74\xbd\x3a\xc0" },
  { 16, "\x39\x2a\x43\x4f\x0e\x07\xdf\x1f\x8a\xa3\x05\xde\x34\xe0\xc2\x29" },
  { 16, "\x3e\x75\xce\xd4\x6b\x69\x30\x21\x21\x88\x30\xae\x86\xa8\x2a\x71" },
  { 17, "\x00\xe9\x02\x8b\x95\x78\xe4\x15\xdc\x1a\x71\x0a\x2b\x88\x15\x44\x47" },
  { 17, "\x00\xd7\x55\x8f\xda\xf5\xf1\x10\x5b\xb2\x13\x28\x2b\x70\x77\x29\xa3" },
  { 16, "\x04\x7e\xcb\xe9\xfc\xa5\x5f\x7b\xd0\x9e\xae\x36\xe1\x0c\xae\x1e" },
  { 17, "\x00\xf5\xc8\x6a\xf3\x61\x62\xf1\x3a\x64\xf5\x4f\x6d\xc9\x58\x7c\x06" },
  { 0, 0 } 
};



PRErrorCode
PSM_SSL_DigiNotarTreatAsRevoked(CERTCertificate * serverCert,
                                CERTCertList * serverCertChain)
{
  
  
  
  
  PRTime cutoff = 0;
  PRStatus status = PR_ParseTimeString("01-JUL-2011 00:00", true, &cutoff);
  if (status != PR_SUCCESS) {
    NS_ASSERTION(status == PR_SUCCESS, "PR_ParseTimeString failed");
    
  } else {
    PRTime notBefore = 0, notAfter = 0;
    if (CERT_GetCertTimes(serverCert, &notBefore, &notAfter) == SECSuccess &&
           notBefore < cutoff) {
      
      return 0;
    }
  }
  
  for (CERTCertListNode *node = CERT_LIST_HEAD(serverCertChain);
       !CERT_LIST_END(node, serverCertChain);
       node = CERT_LIST_NEXT(node)) {
    if (node->cert->issuerName &&
        strstr(node->cert->issuerName, "CN=DigiNotar")) {
      return SEC_ERROR_REVOKED_CERTIFICATE;
    }
  }
  
  return 0;
}


PRErrorCode
PSM_SSL_BlacklistDigiNotar(CERTCertificate * serverCert,
                           CERTCertList * serverCertChain)
{
  bool isDigiNotarIssuedCert = false;

  for (CERTCertListNode *node = CERT_LIST_HEAD(serverCertChain);
       !CERT_LIST_END(node, serverCertChain);
       node = CERT_LIST_NEXT(node)) {
    if (!node->cert->issuerName)
      continue;

    if (strstr(node->cert->issuerName, "CN=DigiNotar")) {
      isDigiNotarIssuedCert = true;
    }
  }

  if (isDigiNotarIssuedCert) {
    
    PRErrorCode revoked_code = PSM_SSL_DigiNotarTreatAsRevoked(serverCert, serverCertChain);
    return (revoked_code != 0) ? revoked_code : SEC_ERROR_UNTRUSTED_ISSUER;
  }

  return 0;
}













static SECStatus
BlockServerCertChangeForSpdy(nsNSSSocketInfo *infoObject,
                             CERTCertificate *serverCert)
{
  
  
  nsCOMPtr<nsIX509Cert> cert;
  nsCOMPtr<nsIX509Cert2> cert2;

  RefPtr<nsSSLStatus> status(infoObject->SSLStatus());
  if (!status) {
    
    
    
    return SECSuccess;
  }
  
  status->GetServerCert(getter_AddRefs(cert));
  cert2 = do_QueryInterface(cert);
  if (!cert2) {
    NS_NOTREACHED("every nsSSLStatus must have a cert"
                  "that implements nsIX509Cert2");
    PR_SetError(SEC_ERROR_LIBRARY_FAILURE, 0);
    return SECFailure;
  }

  
  nsAutoCString negotiatedNPN;
  nsresult rv = infoObject->GetNegotiatedNPN(negotiatedNPN);
  NS_ASSERTION(NS_SUCCEEDED(rv),
               "GetNegotiatedNPN() failed during renegotiation");

  if (NS_SUCCEEDED(rv) && !StringBeginsWith(negotiatedNPN,
                                            NS_LITERAL_CSTRING("spdy/")))
    return SECSuccess;

  
  if (NS_FAILED(rv)) {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
           ("BlockServerCertChangeForSpdy failed GetNegotiatedNPN() call."
            " Assuming spdy.\n"));
  }

  
  ScopedCERTCertificate c(cert2->GetCert());
  NS_ASSERTION(c, "very bad and hopefully impossible state");
  bool sameCert = CERT_CompareCerts(c, serverCert);
  if (sameCert)
    return SECSuccess;

  
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
         ("SPDY Refused to allow new cert during renegotiation\n"));
  PR_SetError(SSL_ERROR_RENEGOTIATION_NOT_ALLOWED, 0);
  return SECFailure;
}

SECStatus
AuthCertificate(TransportSecurityInfo * infoObject, CERTCertificate * cert,
                uint32_t providerFlags)
{
  if (cert->serialNumber.data &&
      cert->issuerName &&
      !strcmp(cert->issuerName, 
        "CN=UTN-USERFirst-Hardware,OU=http://www.usertrust.com,O=The USERTRUST Network,L=Salt Lake City,ST=UT,C=US")) {

    unsigned char *server_cert_comparison_start = cert->serialNumber.data;
    unsigned int server_cert_comparison_len = cert->serialNumber.len;

    while (server_cert_comparison_len) {
      if (*server_cert_comparison_start != 0)
        break;

      ++server_cert_comparison_start;
      --server_cert_comparison_len;
    }

    nsSerialBinaryBlacklistEntry *walk = myUTNBlacklistEntries;
    for ( ; walk && walk->len; ++walk) {

      unsigned char *locked_cert_comparison_start = (unsigned char*)walk->binary_serial;
      unsigned int locked_cert_comparison_len = walk->len;
      
      while (locked_cert_comparison_len) {
        if (*locked_cert_comparison_start != 0)
          break;
        
        ++locked_cert_comparison_start;
        --locked_cert_comparison_len;
      }

      if (server_cert_comparison_len == locked_cert_comparison_len &&
          !memcmp(server_cert_comparison_start, locked_cert_comparison_start, locked_cert_comparison_len)) {
        PR_SetError(SEC_ERROR_REVOKED_CERTIFICATE, 0);
        return SECFailure;
      }
    }
  }

  CERTCertList *verifyCertChain = nullptr;
  SECOidTag evOidPolicy;
  SECStatus rv = PSM_SSL_PKIX_AuthCertificate(cert, infoObject,
                                              infoObject->GetHostName(),
                                              &verifyCertChain,
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

  ScopedCERTCertList certList(verifyCertChain);

  if (!certList) {
    rv = SECFailure;
  } else {
    PRErrorCode blacklistErrorCode;
    if (rv == SECSuccess) { 
      blacklistErrorCode = PSM_SSL_BlacklistDigiNotar(cert, certList);
    } else { 
      PRErrorCode savedErrorCode = PORT_GetError();
      
      blacklistErrorCode = PSM_SSL_DigiNotarTreatAsRevoked(cert, certList);
      if (blacklistErrorCode == 0) {
        
        PORT_SetError(savedErrorCode);
      }
    }
      
    if (blacklistErrorCode != 0) {
      infoObject->SetCertIssuerBlacklisted();
      PORT_SetError(blacklistErrorCode);
      rv = SECFailure;
    }
  }

  if (rv == SECSuccess) {
    
    
    if (!(providerFlags & nsISocketProvider::NO_PERMANENT_STORAGE)) {
      for (CERTCertListNode *node = CERT_LIST_HEAD(certList);
           !CERT_LIST_END(node, certList);
           node = CERT_LIST_NEXT(node)) {

        if (node->cert->slot) {
          
          continue;
        }

        if (node->cert->isperm) {
          
          continue;
        }

        if (node->cert == cert) {
          
          
          continue;
        }

        
        char* nickname = nsNSSCertificate::defaultServerNickname(node->cert);
        if (nickname && *nickname) {
          ScopedPK11SlotInfo slot(PK11_GetInternalKeySlot());
          if (slot) {
            PK11_ImportCert(slot, node->cert, CK_INVALID_HANDLE, 
                            nickname, false);
          }
        }
        PR_FREEIF(nickname);
      }
    }

    
    
    
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
SSLServerCertVerificationJob::Dispatch(const void * fdForLogging,
                                       TransportSecurityInfo * infoObject,
                                       CERTCertificate * serverCert,
                                       uint32_t providerFlags)
{
  
  if (!infoObject || !serverCert) {
    NS_ERROR("Invalid parameters for SSL server cert validation");
    PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
    return SECFailure;
  }
  
  RefPtr<SSLServerCertVerificationJob> job(
    new SSLServerCertVerificationJob(fdForLogging, infoObject, serverCert,
                                     providerFlags));

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
    
    
    PR_SetError(0, 0); 
    SECStatus rv = AuthCertificate(mInfoObject, mCert, mProviderFlags);
    if (rv == SECSuccess) {
      uint32_t interval = (uint32_t) ((TimeStamp::Now() - mJobStartTime).ToMilliseconds());
      Telemetry::ID telemetryID;
#ifndef NSS_NO_LIBPKIX
      if(nsNSSComponent::globalConstFlagUsePKIXVerification){
        telemetryID = Telemetry::SSL_SUCCESFUL_CERT_VALIDATION_TIME_LIBPKIX;
      }
      else{
#endif
        telemetryID = Telemetry::SSL_SUCCESFUL_CERT_VALIDATION_TIME_CLASSIC;
#ifndef NSS_NO_LIBPKIX
      }
#endif
      RefPtr<SSLServerCertVerificationResult> restart(
        new SSLServerCertVerificationResult(mInfoObject, 0,
                                            telemetryID, interval));
      restart->Dispatch();
      return NS_OK;
    }

    
    
    error = PR_GetError();
    {
      TimeStamp now = TimeStamp::Now();
      Telemetry::ID telemetryID;
#ifndef NSS_NO_LIBPKIX
      if(nsNSSComponent::globalConstFlagUsePKIXVerification){
        telemetryID = Telemetry::SSL_INITIAL_FAILED_CERT_VALIDATION_TIME_LIBPKIX;
      }
      else{
#endif
        telemetryID = Telemetry::SSL_INITIAL_FAILED_CERT_VALIDATION_TIME_CLASSIC;
#ifndef NSS_NO_LIBPKIX
      }
#endif
      MutexAutoLock telemetryMutex(*gSSLVerificationTelemetryMutex);
      Telemetry::AccumulateTimeDelta(telemetryID,
                                     mJobStartTime,
                                     now);
    }
    if (error != 0) {
      RefPtr<CertErrorRunnable> runnable(CreateCertErrorRunnable(
        error, mInfoObject, mCert, mFdForLogging, mProviderFlags, PR_Now()));
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
AuthCertificateHook(void *arg, PRFileDesc *fd, PRBool checkSig, PRBool isServer)
{
  

  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
         ("[%p] starting AuthCertificateHook\n", fd));

  
  
  NS_ASSERTION(checkSig, "AuthCertificateHook: checkSig unexpectedly false");

  
  
  NS_ASSERTION(!isServer, "AuthCertificateHook: isServer unexpectedly true");

  nsNSSSocketInfo *socketInfo = static_cast<nsNSSSocketInfo*>(arg);
  
  if (socketInfo) {
    
    socketInfo->SetFirstServerHelloReceived();
  }

  ScopedCERTCertificate serverCert(SSL_PeerCertificate(fd));

  if (!checkSig || isServer || !socketInfo || !serverCert) {
      PR_SetError(PR_INVALID_STATE_ERROR, 0);
      return SECFailure;
  }

  
  
  PRTime now = PR_Now();
  PRBool enabled;
  if (SECSuccess != SSL_OptionGet(fd, SSL_ENABLE_OCSP_STAPLING, &enabled)) {
    return SECFailure;
  }
  if (enabled) {
      
      const SECItemArray *csa = SSL_PeerStapledOCSPResponses(fd);
      
      if (csa && csa->len == 1) {
          CERTCertDBHandle *handle = CERT_GetDefaultCertDB();
          SECStatus cacheResult = CERT_CacheOCSPResponseFromSideChannel(
              handle, serverCert, now, &csa->items[0], arg);
          if (cacheResult != SECSuccess) {
              return SECFailure;
          }
      }
  }

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

  uint32_t providerFlags = 0;
  socketInfo->GetProviderFlags(&providerFlags);

  if (onSTSThread) {

    
    
    
    
    socketInfo->SetCertVerificationWaiting();
    SECStatus rv = SSLServerCertVerificationJob::Dispatch(
                           static_cast<const void *>(fd), socketInfo, serverCert,
                           providerFlags);
    return rv;
  }
  
  
  
  
  
  SECStatus rv = AuthCertificate(socketInfo, serverCert, providerFlags);
  if (rv == SECSuccess) {
    return SECSuccess;
  }

  PRErrorCode error = PR_GetError();
  if (error != 0) {
    RefPtr<CertErrorRunnable> runnable(CreateCertErrorRunnable(
                    error, socketInfo, serverCert,
                    static_cast<const void *>(fd), providerFlags, now));
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

#ifndef NSS_NO_LIBPKIX
class InitializeIdentityInfo : public nsRunnable
                             , public nsNSSShutDownObject
{
private:
  NS_IMETHOD Run()
  {
    nsNSSShutDownPreventionLock nssShutdownPrevention;
    if (isAlreadyShutDown())
      return NS_OK;

    nsresult rv;
    nsCOMPtr<nsINSSComponent> inss = do_GetService(PSM_COMPONENT_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv))
      inss->EnsureIdentityInfoLoaded();
    return NS_OK;
  }

  virtual void virtualDestroyNSSReference()
  {
  }

  ~InitializeIdentityInfo()
  {
    nsNSSShutDownPreventionLock nssShutdownPrevention;
    if (!isAlreadyShutDown())
      shutdown(calledFromObject);
  }
};
#endif

void EnsureServerVerificationInitialized()
{
#ifndef NSS_NO_LIBPKIX
  
  

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
        TransportSecurityInfo * infoObject, PRErrorCode errorCode,
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
  
  ((nsNSSSocketInfo *) mInfoObject.get())
    ->SetCertVerificationResult(mErrorCode, mErrorMessageType);
  return NS_OK;
}

} } 
