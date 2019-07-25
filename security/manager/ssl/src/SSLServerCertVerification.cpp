



































































































































#include "SSLServerCertVerification.h"
#include "nsIBadCertListener2.h"
#include "nsICertOverrideService.h"
#include "nsIStrictTransportSecurityService.h"
#include "nsNSSComponent.h"
#include "nsNSSCleaner.h"
#include "nsRecentBadCerts.h"
#include "nsNSSIOLayer.h"

#include "nsIThreadPool.h"
#include "nsXPCOMCIDInternal.h"
#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"
#include "PSMRunnable.h"

#include "ssl.h"
#include "secerr.h"
#include "secport.h"
#include "sslerr.h"

#ifdef PR_LOGGING
extern PRLogModuleInfo* gPIPNSSLog;
#endif

namespace mozilla { namespace psm {

namespace {

NS_DEFINE_CID(kNSSComponentCID, NS_NSSCOMPONENT_CID);

NSSCleanupAutoPtrClass_WithParam(PRArenaPool, PORT_FreeArena, FalseParam, false)


nsIThreadPool * gCertVerificationThreadPool = nsnull;
} 











void
InitializeSSLServerCertVerificationThreads()
{
  
  
  
  nsresult rv = CallCreateInstance(NS_THREADPOOL_CONTRACTID,
                                   &gCertVerificationThreadPool);
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to create SSL cert verification threads.");
    return;
  }

  (void) gCertVerificationThreadPool->SetIdleThreadLimit(5);
  (void) gCertVerificationThreadPool->SetIdleThreadTimeout(30 * 1000);
  (void) gCertVerificationThreadPool->SetThreadLimit(5);
}











void StopSSLServerCertVerificationThreads()
{
  if (gCertVerificationThreadPool) {
    gCertVerificationThreadPool->Shutdown();
    NS_RELEASE(gCertVerificationThreadPool);
  }
}

namespace {







class SSLServerCertVerificationResult : public nsRunnable
{
public:
  NS_DECL_NSIRUNNABLE

  SSLServerCertVerificationResult(nsNSSSocketInfo & socketInfo,
                                  PRErrorCode errorCode,
                                  SSLErrorMessageType errorMessageType = 
                                      PlainErrorMessage);

  void Dispatch();
private:
  const nsRefPtr<nsNSSSocketInfo> mSocketInfo;
  const PRErrorCode mErrorCode;
  const SSLErrorMessageType mErrorMessageType;
};

class CertErrorRunnable : public SyncRunnableBase
{
 public:
  CertErrorRunnable(const void * fdForLogging,
                    nsIX509Cert * cert,
                    nsNSSSocketInfo * infoObject,
                    PRErrorCode defaultErrorCodeToReport,
                    PRUint32 collectedErrors,
                    PRErrorCode errorCodeTrust,
                    PRErrorCode errorCodeMismatch,
                    PRErrorCode errorCodeExpired)
    : mFdForLogging(fdForLogging), mCert(cert), mInfoObject(infoObject),
      mDefaultErrorCodeToReport(defaultErrorCodeToReport),
      mCollectedErrors(collectedErrors),
      mErrorCodeTrust(errorCodeTrust),
      mErrorCodeMismatch(errorCodeMismatch),
      mErrorCodeExpired(errorCodeExpired)
  {
  }

  NS_DECL_NSIRUNNABLE
  virtual void RunOnTargetThread();
  nsCOMPtr<nsIRunnable> mResult; 
private:
  SSLServerCertVerificationResult* CheckCertOverrides();
  
  const void * const mFdForLogging; 
  const nsCOMPtr<nsIX509Cert> mCert;
  const nsRefPtr<nsNSSSocketInfo> mInfoObject;
  const PRErrorCode mDefaultErrorCodeToReport;
  const PRUint32 mCollectedErrors;
  const PRErrorCode mErrorCodeTrust;
  const PRErrorCode mErrorCodeMismatch;
  const PRErrorCode mErrorCodeExpired;
};

SSLServerCertVerificationResult *
CertErrorRunnable::CheckCertOverrides()
{
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("[%p][%p] top of CertErrorRunnable::Run\n",
                                    mFdForLogging, this));

  if (!NS_IsMainThread()) {
    NS_ERROR("CertErrorRunnable::CheckCertOverrides called off main thread");
    return new SSLServerCertVerificationResult(*mInfoObject,
                                               mDefaultErrorCodeToReport);
  }

  PRInt32 port;
  mInfoObject->GetPort(&port);

  nsCString hostWithPortString;
  hostWithPortString.AppendASCII(mInfoObject->GetHostName());
  hostWithPortString.AppendLiteral(":");
  hostWithPortString.AppendInt(port);

  PRUint32 remaining_display_errors = mCollectedErrors;

  nsresult nsrv;

  
  
  
  bool strictTransportSecurityEnabled = false;
  nsCOMPtr<nsIStrictTransportSecurityService> stss
    = do_GetService(NS_STSSERVICE_CONTRACTID, &nsrv);
  if (NS_SUCCEEDED(nsrv)) {
    nsrv = stss->IsStsHost(mInfoObject->GetHostName(),
                           &strictTransportSecurityEnabled);
  }
  if (NS_FAILED(nsrv)) {
    return new SSLServerCertVerificationResult(*mInfoObject,
                                               mDefaultErrorCodeToReport);
  }

  if (!strictTransportSecurityEnabled) {
    nsCOMPtr<nsICertOverrideService> overrideService =
      do_GetService(NS_CERTOVERRIDE_CONTRACTID);
    

    PRUint32 overrideBits = 0;

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
       
        remaining_display_errors -= overrideBits;
      }
    }

    if (!remaining_display_errors) {
      
      PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
             ("[%p][%p] All errors covered by override rules\n",
             mFdForLogging, this));
      return new SSLServerCertVerificationResult(*mInfoObject, 0);
    }
  } else {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
           ("[%p][%p] Strict-Transport-Security is violated: untrusted "
            "transport layer\n", mFdForLogging, this));
  }

  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
         ("[%p][%p] Certificate error was not overridden\n",
         mFdForLogging, this));

  
  

  
  nsCOMPtr<nsIInterfaceRequestor> cb;
  mInfoObject->GetNotificationCallbacks(getter_AddRefs(cb));
  if (cb) {
    nsCOMPtr<nsIBadCertListener2> bcl = do_GetInterface(cb);
    if (bcl) {
      nsIInterfaceRequestor *csi = static_cast<nsIInterfaceRequestor*>(mInfoObject);
      bool suppressMessage = false; 
      nsrv = bcl->NotifyCertProblem(csi, mInfoObject->SSLStatus(),
                                    hostWithPortString, &suppressMessage);
    }
  }

  nsCOMPtr<nsIRecentBadCertsService> recentBadCertsService = 
    do_GetService(NS_RECENTBADCERTS_CONTRACTID);
 
  if (recentBadCertsService) {
    NS_ConvertUTF8toUTF16 hostWithPortStringUTF16(hostWithPortString);
    recentBadCertsService->AddBadCert(hostWithPortStringUTF16,
                                      mInfoObject->SSLStatus());
  }

  
  PRErrorCode errorCodeToReport = mErrorCodeTrust    ? mErrorCodeTrust
                                : mErrorCodeMismatch ? mErrorCodeMismatch
                                : mErrorCodeExpired  ? mErrorCodeExpired
                                : mDefaultErrorCodeToReport;

  return new SSLServerCertVerificationResult(*mInfoObject, errorCodeToReport,
                                             OverridableCertErrorMessage);
}

NS_IMETHODIMP
CertErrorRunnable::Run()
{
  
  
  
  
  
  if (!NS_IsMainThread()) {
    
    
    DispatchToMainThreadAndWait(); 

    
    if (!mResult) {
      
      NS_ERROR("Did not create a SSLServerCertVerificationResult");
      mResult = new SSLServerCertVerificationResult(*mInfoObject,
                                                    PR_INVALID_STATE_ERROR);
    }
    return mResult->Run(); 
  } else {
    
    
    return SyncRunnableBase::Run(); 
  }
}

void 
CertErrorRunnable::RunOnTargetThread()
{
  
  
  
  
  
  
  
  mResult = CheckCertOverrides(); 
}








SECStatus
HandleBadCertificate(PRErrorCode defaultErrorCodeToReport,
                    nsNSSSocketInfo * socketInfo, CERTCertificate & cert,
                    const void * fdForLogging,
                    const nsNSSShutDownPreventionLock & )
{
  
  if (defaultErrorCodeToReport == SEC_ERROR_REVOKED_CERTIFICATE) {
    PR_SetError(SEC_ERROR_REVOKED_CERTIFICATE, 0);
    return SECFailure;
  }

  if (defaultErrorCodeToReport == 0) {
    NS_ERROR("No error code set during certificate validation failure.");
    PR_SetError(PR_INVALID_STATE_ERROR, 0);
    return SECFailure;
  }

  nsRefPtr<nsNSSCertificate> nssCert;
  nssCert = nsNSSCertificate::Create(&cert);
  if (!nssCert) {
    NS_ERROR("nsNSSCertificate::Create failed in DispatchCertErrorRunnable");
    PR_SetError(SEC_ERROR_NO_MEMORY, 0);
    return SECFailure;
  }

  SECStatus srv;
  nsresult nsrv;

  nsCOMPtr<nsINSSComponent> inss = do_GetService(kNSSComponentCID, &nsrv);
  if (!inss) {
    NS_ERROR("do_GetService(kNSSComponentCID) failed in DispatchCertErrorRunnable");
    PR_SetError(defaultErrorCodeToReport, 0);
    return SECFailure;
  }

  nsRefPtr<nsCERTValInParamWrapper> survivingParams;
  nsrv = inss->GetDefaultCERTValInParam(survivingParams);
  if (NS_FAILED(nsrv)) {
    NS_ERROR("GetDefaultCERTValInParam failed in DispatchCertErrorRunnable");
    PR_SetError(defaultErrorCodeToReport, 0);
    return SECFailure;
  }
  
  PRArenaPool *log_arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
  PRArenaPoolCleanerFalseParam log_arena_cleaner(log_arena);
  if (!log_arena) {
    NS_ERROR("PORT_NewArena failed in DispatchCertErrorRunnable");
    return SECFailure; 
  }

  CERTVerifyLog *verify_log = PORT_ArenaZNew(log_arena, CERTVerifyLog);
  if (!verify_log) {
    NS_ERROR("PORT_ArenaZNew failed in DispatchCertErrorRunnable");
    return SECFailure; 
  }
  CERTVerifyLogContentsCleaner verify_log_cleaner(verify_log);
  verify_log->arena = log_arena;

  if (!nsNSSComponent::globalConstFlagUsePKIXVerification) {
    srv = CERT_VerifyCertificate(CERT_GetDefaultCertDB(), &cert,
                                true, certificateUsageSSLServer,
                                PR_Now(), static_cast<void*>(socketInfo),
                                verify_log, NULL);
  }
  else {
    CERTValOutParam cvout[2];
    cvout[0].type = cert_po_errorLog;
    cvout[0].value.pointer.log = verify_log;
    cvout[1].type = cert_po_end;

    srv = CERT_PKIXVerifyCert(&cert, certificateUsageSSLServer,
                              survivingParams->GetRawPointerForNSS(),
                              cvout, static_cast<void*>(socketInfo));
  }

  
  
  
  
  

  PRErrorCode errorCodeMismatch = 0;
  PRErrorCode errorCodeTrust = 0;
  PRErrorCode errorCodeExpired = 0;

  PRUint32 collected_errors = 0;

  if (socketInfo->IsCertIssuerBlacklisted()) {
    collected_errors |= nsICertOverrideService::ERROR_UNTRUSTED;
    errorCodeTrust = defaultErrorCodeToReport;
  }

  
  if (CERT_VerifyCertName(&cert, socketInfo->GetHostName()) != SECSuccess) {
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
        return SECFailure;
    }
  }

  if (!collected_errors)
  {
    
    
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("[%p] !collected_errors: %d\n",
           fdForLogging, static_cast<int>(defaultErrorCodeToReport)));
    PR_SetError(defaultErrorCodeToReport, 0);
    return SECFailure;
  }

  socketInfo->SetStatusErrorBits(*nssCert, collected_errors);

  nsRefPtr<CertErrorRunnable> runnable =
    new CertErrorRunnable(fdForLogging, 
                          static_cast<nsIX509Cert*>(nssCert.get()),
                          socketInfo, defaultErrorCodeToReport, 
                          collected_errors, errorCodeTrust, 
                          errorCodeMismatch, errorCodeExpired);

  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
          ("[%p][%p] Before dispatching CertErrorRunnable\n",
          fdForLogging, runnable.get()));

  nsresult nrv;
  nsCOMPtr<nsIEventTarget> stsTarget
    = do_GetService(NS_SOCKETTRANSPORTSERVICE_CONTRACTID, &nrv);
  if (NS_SUCCEEDED(nrv)) {
    nrv = stsTarget->Dispatch(runnable, NS_DISPATCH_NORMAL);
  }
  if (NS_FAILED(nrv)) {
    NS_ERROR("Failed to dispatch CertErrorRunnable");
    PR_SetError(defaultErrorCodeToReport, 0);
    return SECFailure;
  }

  return SECSuccess;
}

class SSLServerCertVerificationJob : public nsRunnable
{
public:
  
  static SECStatus Dispatch(const void * fdForLogging,
                            nsNSSSocketInfo * infoObject,
                            CERTCertificate * serverCert);
private:
  NS_DECL_NSIRUNNABLE

  
  SSLServerCertVerificationJob(const void * fdForLogging,
                               nsNSSSocketInfo & socketInfo, 
                               CERTCertificate & cert);
  ~SSLServerCertVerificationJob();

  
  SECStatus AuthCertificate(const nsNSSShutDownPreventionLock & proofOfLock);

  const void * const mFdForLogging;
  const nsRefPtr<nsNSSSocketInfo> mSocketInfo;
  CERTCertificate * const mCert;
};

SSLServerCertVerificationJob::SSLServerCertVerificationJob(
    const void * fdForLogging, nsNSSSocketInfo & socketInfo,
    CERTCertificate & cert)
  : mFdForLogging(fdForLogging)
  , mSocketInfo(&socketInfo)
  , mCert(CERT_DupCertificate(&cert))
{
}

SSLServerCertVerificationJob::~SSLServerCertVerificationJob()
{
  CERT_DestroyCertificate(mCert);
}

SECStatus
PSM_SSL_PKIX_AuthCertificate(CERTCertificate *peerCert, void * pinarg,
                             const char * hostname,
                             const nsNSSShutDownPreventionLock & )
{
    SECStatus          rv;
    
    if (!nsNSSComponent::globalConstFlagUsePKIXVerification) {
        rv = CERT_VerifyCertNow(CERT_GetDefaultCertDB(), peerCert, true,
                                certUsageSSLServer, pinarg);
    }
    else {
        nsresult nsrv;
        nsCOMPtr<nsINSSComponent> inss = do_GetService(kNSSComponentCID, &nsrv);
        if (!inss)
          return SECFailure;
        nsRefPtr<nsCERTValInParamWrapper> survivingParams;
        if (NS_FAILED(inss->GetDefaultCERTValInParam(survivingParams)))
          return SECFailure;

        CERTValOutParam cvout[1];
        cvout[0].type = cert_po_end;

        rv = CERT_PKIXVerifyCert(peerCert, certificateUsageSSLServer,
                                survivingParams->GetRawPointerForNSS(),
                                cvout, pinarg);
    }

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

  nsRefPtr<nsSSLStatus> status = infoObject->SSLStatus();
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

  
  nsCAutoString negotiatedNPN;
  nsresult rv = infoObject->GetNegotiatedNPN(negotiatedNPN);
  NS_ASSERTION(NS_SUCCEEDED(rv),
               "GetNegotiatedNPN() failed during renegotiation");

  if (NS_SUCCEEDED(rv) && !negotiatedNPN.Equals(NS_LITERAL_CSTRING("spdy/2")))
    return SECSuccess;

  
  if (NS_FAILED(rv))
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
           ("BlockServerCertChangeForSpdy failed GetNegotiatedNPN() call."
            " Assuming spdy.\n"));

  
  CERTCertificate * c = cert2->GetCert();
  NS_ASSERTION(c, "very bad and hopefully impossible state");
  bool sameCert = CERT_CompareCerts(c, serverCert);
  CERT_DestroyCertificate(c);
  if (sameCert)
    return SECSuccess;

  
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
         ("SPDY Refused to allow new cert during renegotiation\n"));
  PR_SetError(SSL_ERROR_RENEGOTIATION_NOT_ALLOWED, 0);
  return SECFailure;
}

SECStatus
SSLServerCertVerificationJob::AuthCertificate(
  nsNSSShutDownPreventionLock const & nssShutdownPreventionLock)
{
  if (BlockServerCertChangeForSpdy(mSocketInfo, mCert) != SECSuccess)
    return SECFailure;

  if (mCert->serialNumber.data &&
      mCert->issuerName &&
      !strcmp(mCert->issuerName, 
        "CN=UTN-USERFirst-Hardware,OU=http://www.usertrust.com,O=The USERTRUST Network,L=Salt Lake City,ST=UT,C=US")) {

    unsigned char *server_cert_comparison_start = mCert->serialNumber.data;
    unsigned int server_cert_comparison_len = mCert->serialNumber.len;

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

  SECStatus rv = PSM_SSL_PKIX_AuthCertificate(mCert, mSocketInfo,
                                              mSocketInfo->GetHostName(),
                                              nssShutdownPreventionLock);

  
  
  

  nsRefPtr<nsSSLStatus> status = mSocketInfo->SSLStatus();
  nsRefPtr<nsNSSCertificate> nsc;

  if (!status || !status->mServerCert) {
    nsc = nsNSSCertificate::Create(mCert);
  }

  CERTCertList *certList = nsnull;
  certList = CERT_GetCertChainFromCert(mCert, PR_Now(), certUsageSSLCA);
  if (!certList) {
    rv = SECFailure;
  } else {
    PRErrorCode blacklistErrorCode;
    if (rv == SECSuccess) { 
      blacklistErrorCode = PSM_SSL_BlacklistDigiNotar(mCert, certList);
    } else { 
      PRErrorCode savedErrorCode = PORT_GetError();
      
      blacklistErrorCode = PSM_SSL_DigiNotarTreatAsRevoked(mCert, certList);
      if (blacklistErrorCode == 0) {
        
        PORT_SetError(savedErrorCode);
      }
    }
      
    if (blacklistErrorCode != 0) {
      mSocketInfo->SetCertIssuerBlacklisted();
      PORT_SetError(blacklistErrorCode);
      rv = SECFailure;
    }
  }

  if (rv == SECSuccess) {
    if (nsc) {
      bool dummyIsEV;
      nsc->GetIsExtendedValidation(&dummyIsEV); 
    }
    
    nsCOMPtr<nsINSSComponent> nssComponent;
      
    for (CERTCertListNode *node = CERT_LIST_HEAD(certList);
         !CERT_LIST_END(node, certList);
         node = CERT_LIST_NEXT(node)) {

      if (node->cert->slot) {
        
        continue;
      }

      if (node->cert->isperm) {
        
        continue;
      }
        
      if (node->cert == mCert) {
        
        
        continue;
      }

      
      char* nickname = nsNSSCertificate::defaultServerNickname(node->cert);
      if (nickname && *nickname) {
        PK11SlotInfo *slot = PK11_GetInternalKeySlot();
        if (slot) {
          PK11_ImportCert(slot, node->cert, CK_INVALID_HANDLE, 
                          nickname, false);
          PK11_FreeSlot(slot);
        }
      }
      PR_FREEIF(nickname);
    }

    if (certList) {
      CERT_DestroyCertList(certList);
    }

    
    
    
    if (!status) {
      status = new nsSSLStatus();
      mSocketInfo->SetSSLStatus(status);
    }

    if (rv == SECSuccess) {
      
      
      nsSSLIOLayerHelpers::mHostsWithCertErrors->RememberCertHasError(
        mSocketInfo, nsnull, rv);
    }
    else {
      
      nsSSLIOLayerHelpers::mHostsWithCertErrors->LookupCertErrorBits(
        mSocketInfo, status);
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
                                       nsNSSSocketInfo * socketInfo,
                                       CERTCertificate * serverCert)
{
  
  if (!socketInfo || !serverCert) {
    NS_ERROR("Invalid parameters for SSL server cert validation");
    PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
    return SECFailure;
  }
  
  nsRefPtr<SSLServerCertVerificationJob> job
    = new SSLServerCertVerificationJob(fdForLogging, *socketInfo, *serverCert);

  socketInfo->SetCertVerificationWaiting();
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
          ("[%p] SSLServerCertVerificationJob::Run\n", mSocketInfo.get()));

  PRErrorCode error;

  nsNSSShutDownPreventionLock nssShutdownPrevention;
  if (mSocketInfo->isAlreadyShutDown()) {
    error = SEC_ERROR_USER_CANCELLED;
  } else {
    
    
    PR_SetError(0, 0); 
    SECStatus rv = AuthCertificate(nssShutdownPrevention);
    if (rv == SECSuccess) {
      nsRefPtr<SSLServerCertVerificationResult> restart 
        = new SSLServerCertVerificationResult(*mSocketInfo, 0);
      restart->Dispatch();
      return NS_OK;
    }

    error = PR_GetError();
    if (error != 0) {
      rv = HandleBadCertificate(error, mSocketInfo, *mCert, mFdForLogging,
                                nssShutdownPrevention);
      if (rv == SECSuccess) {
        
        
        
        
        return NS_OK; 
      }
      
      error = PR_GetError(); 
    }
  }

  if (error == 0) {
    NS_NOTREACHED("no error set during certificate validation failure");
    error = PR_INVALID_STATE_ERROR;
  }

  nsRefPtr<SSLServerCertVerificationResult> failure
    = new SSLServerCertVerificationResult(*mSocketInfo, error);
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

  if (!checkSig || isServer) {
      PR_SetError(PR_INVALID_STATE_ERROR, 0);
      return SECFailure;
  }
      
  CERTCertificate *serverCert = SSL_PeerCertificate(fd);

  nsNSSSocketInfo *socketInfo = static_cast<nsNSSSocketInfo*>(arg);
  SECStatus rv = SSLServerCertVerificationJob::Dispatch(
                        static_cast<const void *>(fd), socketInfo, serverCert);

  CERT_DestroyCertificate(serverCert);

  return rv;
}

SSLServerCertVerificationResult::SSLServerCertVerificationResult(
        nsNSSSocketInfo & socketInfo, PRErrorCode errorCode,
        SSLErrorMessageType errorMessageType)
  : mSocketInfo(&socketInfo)
  , mErrorCode(errorCode)
  , mErrorMessageType(errorMessageType)
{
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
  
  mSocketInfo->SetCertVerificationResult(mErrorCode, mErrorMessageType);
  return NS_OK;
}

} } 
