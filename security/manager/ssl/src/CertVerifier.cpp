



#include "CertVerifier.h"
#include "nsNSSComponent.h"
#include "nsServiceManagerUtils.h"
#include "cert.h"
#include "secerr.h"


#ifdef PR_LOGGING
extern PRLogModuleInfo* gPIPNSSLog;
#endif

namespace mozilla { namespace psm {

extern SECStatus getFirstEVPolicy(CERTCertificate *cert, SECOidTag &outOidTag);
extern CERTCertList* getRootsForOid(SECOidTag oid_tag);

const CertVerifier::Flags CertVerifier::FLAG_LOCAL_ONLY = 1;

CertVerifier::CertVerifier(missing_cert_download_config mcdc,
                           crl_download_config cdc,
                           ocsp_download_config odc,
                           ocsp_strict_config osc,
                           any_revo_fresh_config arfc,
                           const char *firstNetworkRevocationMethod)
  : mMissingCertDownloadEnabled(mcdc == missing_cert_download_on)
  , mCRLDownloadEnabled(cdc == crl_download_allowed)
  , mOCSPDownloadEnabled(odc == ocsp_on)
  , mOCSPStrict(osc == ocsp_strict)
  , mRequireRevocationInfo(arfc == any_revo_strict)
  , mCRLFirst(firstNetworkRevocationMethod != nullptr &&
              !strcmp("crl", firstNetworkRevocationMethod))
{
  MOZ_COUNT_CTOR(CertVerifier);
}

CertVerifier::~CertVerifier()
{
  MOZ_COUNT_DTOR(CertVerifier);
}


static SECStatus
ClassicVerifyCert(CERTCertificate * cert,
                  const SECCertificateUsage usage,
                  const PRTime time,
                  nsIInterfaceRequestor * pinArg,
                   CERTCertList **validationChain,
                   CERTVerifyLog *verifyLog)
{
  SECStatus rv;
  SECCertUsage enumUsage;
  if (validationChain) {
    switch(usage){
      case  certificateUsageSSLClient:
        enumUsage = certUsageSSLClient;
        break;
      case  certificateUsageSSLServer:
        enumUsage = certUsageSSLServer;
        break;
      case certificateUsageSSLServerWithStepUp:
        enumUsage = certUsageSSLServerWithStepUp;
        break;
      case certificateUsageSSLCA:
        enumUsage = certUsageSSLCA;
        break;
      case certificateUsageEmailSigner:
        enumUsage = certUsageEmailSigner;
        break;
      case certificateUsageEmailRecipient:
        enumUsage = certUsageEmailRecipient;
        break;
      case certificateUsageObjectSigner:
        enumUsage = certUsageObjectSigner;
        break;
      case certificateUsageUserCertImport:
        enumUsage = certUsageUserCertImport;
        break;
      case certificateUsageVerifyCA:
        enumUsage = certUsageVerifyCA;
        break;
      case certificateUsageProtectedObjectSigner:
        enumUsage = certUsageProtectedObjectSigner;
        break;
      case certificateUsageStatusResponder:
        enumUsage = certUsageStatusResponder;
        break;
      case certificateUsageAnyCA:
        enumUsage = certUsageAnyCA;
        break;
       default:
        return SECFailure;
    }
  }
  if (usage == certificateUsageSSLServer) {
    




    rv = CERT_VerifyCert(CERT_GetDefaultCertDB(), cert, true,
                         certUsageSSLServer, time, pinArg, verifyLog);
  } else {
    rv = CERT_VerifyCertificate(CERT_GetDefaultCertDB(), cert, true,
                                usage, time, pinArg,
                                verifyLog, nullptr);
  }
  if (rv == SECSuccess && validationChain) {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("VerifyCert: getting chain in 'classic' \n"));
    *validationChain = CERT_GetCertChainFromCert(cert, time, enumUsage);
    if (!*validationChain) {
      rv = SECFailure;
    }
  }
  return rv;
}

SECStatus
CertVerifier::VerifyCert(CERTCertificate * cert,
                         const SECCertificateUsage usage,
                         const PRTime time,
                         nsIInterfaceRequestor * pinArg,
                         const Flags flags,
                          CERTCertList **validationChain,
                          SECOidTag *evOidPolicy,
                          CERTVerifyLog *verifyLog)
{
  if (!cert) {
    PORT_SetError(SEC_ERROR_INVALID_ARGS);
    return SECFailure;
  }
  if (validationChain) {
    *validationChain = nullptr;
  }
  if (evOidPolicy) {
    *evOidPolicy = SEC_OID_UNKNOWN;
  }

  ScopedCERTCertList trustAnchors;
  SECStatus rv;
  SECOidTag evPolicy = SEC_OID_UNKNOWN;

#ifdef NSS_NO_LIBPKIX
  return ClassicVerifyCert(cert, usage, time, pinArg, validationChain,
                           verifyLog);
#else
  
  if (usage == certificateUsageSSLServer) {
    SECStatus srv = getFirstEVPolicy(cert, evPolicy);
    if (srv == SECSuccess) {
      if (evPolicy != SEC_OID_UNKNOWN) {
        trustAnchors = getRootsForOid(evPolicy);
      }
      if (!trustAnchors) {
        return SECFailure;
      }
    } else {
      
      evPolicy = SEC_OID_UNKNOWN;
    }
  }
  
  MOZ_ASSERT_IF(evPolicy != SEC_OID_UNKNOWN, trustAnchors);

  size_t i = 0;
  size_t validationChainLocation = 0;
  size_t validationTrustAnchorLocation = 0;
  CERTValOutParam cvout[4];
  if (verifyLog) {
     cvout[i].type = cert_po_errorLog;
     cvout[i].value.pointer.log = verifyLog;
     ++i;
  }
  if (validationChain) {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("VerifyCert: setting up validation chain outparam.\n"));
    validationChainLocation = i;
    cvout[i].type = cert_po_certList;
    cvout[i].value.pointer.cert = nullptr;
    ++i;
    validationTrustAnchorLocation = i;
    cvout[i].type = cert_po_trustAnchor;
    cvout[i].value.pointer.chain = nullptr;
    ++i;
  }
  cvout[i].type = cert_po_end;

  CERTRevocationFlags rev;

  CERTRevocationMethodIndex revPreferredMethods[2];
  rev.leafTests.preferred_methods =
  rev.chainTests.preferred_methods = revPreferredMethods;

  uint64_t revFlagsPerMethod[2];
  rev.leafTests.cert_rev_flags_per_method =
  rev.chainTests.cert_rev_flags_per_method = revFlagsPerMethod;
  rev.leafTests.number_of_preferred_methods =
  rev.chainTests.number_of_preferred_methods = 1;

  rev.leafTests.number_of_defined_methods =
  rev.chainTests.number_of_defined_methods = cert_revocation_method_ocsp + 1;

  const bool localOnly = flags & FLAG_LOCAL_ONLY;
  CERTValInParam cvin[6];

  
  cvin[0].type = cert_pi_useAIACertFetch;
  cvin[0].value.scalar.b = mMissingCertDownloadEnabled && !localOnly;
  cvin[1].type = cert_pi_revocationFlags;
  cvin[1].value.pointer.revocation = &rev;
  cvin[2].type = cert_pi_date;
  cvin[2].value.scalar.time = time;
  i = 3;
  const size_t evParamLocation = i;

  if (evPolicy != SEC_OID_UNKNOWN) {
    
    
    
    uint64_t revMethodFlags =
      CERT_REV_M_TEST_USING_THIS_METHOD
      | ((mOCSPDownloadEnabled && !localOnly) ?
          CERT_REV_M_ALLOW_NETWORK_FETCHING : CERT_REV_M_FORBID_NETWORK_FETCHING)
      | CERT_REV_M_ALLOW_IMPLICIT_DEFAULT_SOURCE
      | CERT_REV_M_REQUIRE_INFO_ON_MISSING_SOURCE
      | CERT_REV_M_IGNORE_MISSING_FRESH_INFO
      | CERT_REV_M_STOP_TESTING_ON_FRESH_INFO;
 
    rev.leafTests.cert_rev_flags_per_method[cert_revocation_method_crl] =
    rev.chainTests.cert_rev_flags_per_method[cert_revocation_method_crl] = revMethodFlags;
    rev.leafTests.cert_rev_flags_per_method[cert_revocation_method_ocsp] =
    rev.chainTests.cert_rev_flags_per_method[cert_revocation_method_ocsp] = revMethodFlags;

    rev.leafTests.cert_rev_method_independent_flags =
    rev.chainTests.cert_rev_method_independent_flags =
      
      CERT_REV_MI_TEST_ALL_LOCAL_INFORMATION_FIRST
      
      |  CERT_REV_MI_REQUIRE_SOME_FRESH_INFO_AVAILABLE
      ;

    rev.leafTests.preferred_methods[0] =
    rev.chainTests.preferred_methods[0] = cert_revocation_method_ocsp;

    cvin[i].type = cert_pi_policyOID;
    cvin[i].value.arraySize = 1;
    cvin[i].value.array.oids = &evPolicy;
    ++i;
    MOZ_ASSERT(trustAnchors);
    cvin[i].type = cert_pi_trustAnchors;
    cvin[i].value.pointer.chain = trustAnchors;
    ++i;

    cvin[i].type = cert_pi_end;

    rv = CERT_PKIXVerifyCert(cert, usage, cvin, cvout, pinArg);
    if (rv == SECSuccess) {
      if (evOidPolicy) {
        *evOidPolicy = evPolicy;
      }
      PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
             ("VerifyCert: successful CERT_PKIXVerifyCert(ev) \n"));
      goto pkix_done;
    }

    if (validationChain && *validationChain) {
      
      
      MOZ_ASSERT(false,
                 "certPKIXVerifyCert returned failure AND a validationChain");
      CERT_DestroyCertList(*validationChain);
      *validationChain = nullptr;
    }

    if (verifyLog) {
      
      CERTVerifyLogNode *i_node;
      for (i_node = verifyLog->head; i_node; i_node = i_node->next) {
         
         if (i_node->cert) {
           CERT_DestroyCertificate(i_node->cert);
         }
         
      }
      verifyLog->count = 0;
      verifyLog->head = nullptr;
      verifyLog->tail = nullptr;
    }

  }

  if (!nsNSSComponent::globalConstFlagUsePKIXVerification){
    
    
    return ClassicVerifyCert(cert, usage, time, pinArg, validationChain,
                             verifyLog);
  }

  
  rev.leafTests.cert_rev_flags_per_method[cert_revocation_method_crl] =
  rev.chainTests.cert_rev_flags_per_method[cert_revocation_method_crl] =
    
    CERT_REV_M_IGNORE_IMPLICIT_DEFAULT_SOURCE

    
    | CERT_REV_M_CONTINUE_TESTING_ON_FRESH_INFO

    
    | CERT_REV_M_IGNORE_MISSING_FRESH_INFO

    
    | CERT_REV_M_TEST_USING_THIS_METHOD

    
    | CERT_REV_M_SKIP_TEST_ON_MISSING_SOURCE

    
    | ((mCRLDownloadEnabled && !localOnly) ?
        CERT_REV_M_ALLOW_NETWORK_FETCHING : CERT_REV_M_FORBID_NETWORK_FETCHING)
    ;

  rev.leafTests.cert_rev_flags_per_method[cert_revocation_method_ocsp] =
  rev.chainTests.cert_rev_flags_per_method[cert_revocation_method_ocsp] =
    
      CERT_REV_M_TEST_USING_THIS_METHOD

    
    | CERT_REV_M_ALLOW_IMPLICIT_DEFAULT_SOURCE

    
    | CERT_REV_M_SKIP_TEST_ON_MISSING_SOURCE

    
    | (mOCSPStrict ?
       CERT_REV_M_FAIL_ON_MISSING_FRESH_INFO : CERT_REV_M_IGNORE_MISSING_FRESH_INFO)

    
    | CERT_REV_M_STOP_TESTING_ON_FRESH_INFO

    
    | ((mOCSPDownloadEnabled && !localOnly) ?
        CERT_REV_M_ALLOW_NETWORK_FETCHING : CERT_REV_M_FORBID_NETWORK_FETCHING)
    ;

  rev.leafTests.preferred_methods[0] =
  rev.chainTests.preferred_methods[0] =
    mCRLFirst ? cert_revocation_method_crl : cert_revocation_method_ocsp;

  rev.leafTests.cert_rev_method_independent_flags =
  rev.chainTests.cert_rev_method_independent_flags =
    
    CERT_REV_MI_TEST_ALL_LOCAL_INFORMATION_FIRST

    
    | (mRequireRevocationInfo ?
       CERT_REV_MI_REQUIRE_SOME_FRESH_INFO_AVAILABLE : CERT_REV_MI_NO_OVERALL_INFO_REQUIREMENT)
    ;

  
  cvin[evParamLocation].type = cert_pi_end;

  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("VerifyCert: calling CERT_PKIXVerifyCert(dv) \n"));
  rv = CERT_PKIXVerifyCert(cert, usage, cvin, cvout, pinArg);

pkix_done:
  if (validationChain) {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("VerifyCert: validation chain requested\n"));
    ScopedCERTCertificate trustAnchor(cvout[validationTrustAnchorLocation].value.pointer.cert);

    if (rv == SECSuccess) {
      if (! cvout[validationChainLocation].value.pointer.chain) {
        PR_SetError(PR_UNKNOWN_ERROR, 0);
        return SECFailure;
      }
      PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("VerifyCert: I have a chain\n"));
      *validationChain = cvout[validationChainLocation].value.pointer.chain;
      if (trustAnchor) {
        
        
        
        if (!CERT_CompareCerts(trustAnchor, cert)) {
          PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("VerifyCert:  adding issuer to tail for display\n"));
          
          ScopedCERTCertificate tempCert(CERT_DupCertificate(trustAnchor));
          rv = CERT_AddCertToListTail(*validationChain, tempCert);
          if (rv == SECSuccess) {
            tempCert.forget(); 
          } else {
            CERT_DestroyCertList(*validationChain);
            *validationChain = nullptr;
          }
        }
      }
    } else {
      
      if (cvout[validationChainLocation].value.pointer.chain) {
        CERT_DestroyCertList(cvout[validationChainLocation].value.pointer.chain);
      }
    }
  }

  return rv;
#endif
}

TemporaryRef<CertVerifier>
GetDefaultCertVerifier()
{
  static NS_DEFINE_CID(kNSSComponentCID, NS_NSSCOMPONENT_CID);

  nsCOMPtr<nsINSSComponent> nssComponent(do_GetService(kNSSComponentCID));
  RefPtr<CertVerifier> certVerifier;
  if (nssComponent) {
    (void) nssComponent->GetDefaultCertVerifier(certVerifier);
  }
  return certVerifier;
}

} } 
