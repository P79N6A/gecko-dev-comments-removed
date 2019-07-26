





#include "CertVerifier.h"

#include <stdint.h>

#include "insanity/pkix.h"
#include "ExtendedValidation.h"
#include "NSSCertDBTrustDomain.h"
#include "cert.h"
#include "ocsp.h"
#include "secerr.h"
#include "prerror.h"
#include "sslerr.h"



using namespace insanity::pkix;
using namespace mozilla::psm;

#ifdef PR_LOGGING
PRLogModuleInfo* gCertVerifierLog = nullptr;
#endif

namespace mozilla { namespace psm {

const CertVerifier::Flags CertVerifier::FLAG_LOCAL_ONLY = 1;
const CertVerifier::Flags CertVerifier::FLAG_MUST_BE_EV = 2;

CertVerifier::CertVerifier(implementation_config ic,
#ifndef NSS_NO_LIBPKIX
                           missing_cert_download_config mcdc,
                           crl_download_config cdc,
#endif
                           ocsp_download_config odc,
                           ocsp_strict_config osc,
                           ocsp_get_config ogc)
  : mImplementation(ic)
#ifndef NSS_NO_LIBPKIX
  , mMissingCertDownloadEnabled(mcdc == missing_cert_download_on)
  , mCRLDownloadEnabled(cdc == crl_download_allowed)
#endif
  , mOCSPDownloadEnabled(odc == ocsp_on)
  , mOCSPStrict(osc == ocsp_strict)
  , mOCSPGETEnabled(ogc == ocsp_get_enabled)
{
}

CertVerifier::~CertVerifier()
{
}

void
InitCertVerifierLog()
{
#ifdef PR_LOGGING
  if (!gCertVerifierLog) {
    gCertVerifierLog = PR_NewLogModule("certverifier");
  }
#endif
}

#if 0


static SECStatus
insertErrorIntoVerifyLog(CERTCertificate* cert, const PRErrorCode err,
                         CERTVerifyLog* verifyLog){
  CERTVerifyLogNode* node;
  node = (CERTVerifyLogNode *)PORT_ArenaAlloc(verifyLog->arena,
                                              sizeof(CERTVerifyLogNode));
  if (!node) {
    PR_SetError(PR_UNKNOWN_ERROR, 0);
    return SECFailure;
  }
  node->cert = CERT_DupCertificate(cert);
  node->error = err;
  node->depth = 0;
  node->arg = nullptr;
  
  node->prev = nullptr;
  node->next = verifyLog->head;
  if (verifyLog->head) {
    verifyLog->head->prev = node;
  }
  verifyLog->head = node;
  if (!verifyLog->tail) {
    verifyLog->tail = node;
  }
  verifyLog->count++;

  return SECSuccess;
}
#endif

SECStatus chainValidationCallback(void* state, const CERTCertList* certList,
                                  PRBool* chainOK)
{
  *chainOK = PR_FALSE;

  PR_LOG(gCertVerifierLog, PR_LOG_DEBUG, ("verifycert: Inside the Callback \n"));

  
  if (!certList) {
    PR_LOG(gCertVerifierLog, PR_LOG_DEBUG, ("verifycert: Short circuit, callback, "
                                            "sanity check failed \n"));
    PR_SetError(PR_INVALID_STATE_ERROR, 0);
    return SECFailure;
  }
  *chainOK = PR_TRUE;
  return SECSuccess;
}

static SECStatus
ClassicVerifyCert(CERTCertificate* cert,
                  const SECCertificateUsage usage,
                  const PRTime time,
                  void* pinArg,
                   ScopedCERTCertList* validationChain,
                   CERTVerifyLog* verifyLog)
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
      case certificateUsageStatusResponder:
        enumUsage = certUsageStatusResponder;
        break;
      default:
        PR_NOT_REACHED("unexpected usage");
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }
  }
  if (usage == certificateUsageSSLServer) {
    
    
    
    
    rv = CERT_VerifyCert(CERT_GetDefaultCertDB(), cert, true,
                         certUsageSSLServer, time, pinArg, verifyLog);
  } else {
    rv = CERT_VerifyCertificate(CERT_GetDefaultCertDB(), cert, true,
                                usage, time, pinArg, verifyLog, nullptr);
  }
  if (rv == SECSuccess && validationChain) {
    PR_LOG(gCertVerifierLog, PR_LOG_DEBUG, ("VerifyCert: getting chain in 'classic' \n"));
    *validationChain = CERT_GetCertChainFromCert(cert, time, enumUsage);
    if (!*validationChain) {
      rv = SECFailure;
    }
  }
  return rv;
}

#ifndef NSS_NO_LIBPKIX
static void
destroyCertListThatShouldNotExist(CERTCertList** certChain)
{
  PR_ASSERT(certChain);
  PR_ASSERT(!*certChain);
  if (certChain && *certChain) {
    
    
    CERT_DestroyCertList(*certChain);
    *certChain = nullptr;
  }
}
#endif

static SECStatus
BuildCertChainForOneKeyUsage(TrustDomain& trustDomain, CERTCertificate* cert,
                             PRTime time, KeyUsages ku1, KeyUsages ku2,
                             KeyUsages ku3, SECOidTag eku,
                             SECOidTag requiredPolicy,
                             const SECItem* stapledOCSPResponse,
                             ScopedCERTCertList& builtChain)
{
  PR_ASSERT(ku1);
  PR_ASSERT(ku2);

  SECStatus rv = BuildCertChain(trustDomain, cert, time, MustBeEndEntity,
                                ku1, eku, requiredPolicy, stapledOCSPResponse,
                                builtChain);
  if (rv != SECSuccess && ku2 &&
      PR_GetError() == SEC_ERROR_INADEQUATE_KEY_USAGE) {
    rv = BuildCertChain(trustDomain, cert, time, MustBeEndEntity,
                        ku2, eku, requiredPolicy, stapledOCSPResponse,
                        builtChain);
    if (rv != SECSuccess && ku3 &&
        PR_GetError() == SEC_ERROR_INADEQUATE_KEY_USAGE) {
      rv = BuildCertChain(trustDomain, cert, time, MustBeEndEntity,
                          ku3, eku, requiredPolicy, stapledOCSPResponse,
                          builtChain);
      if (rv != SECSuccess) {
        PR_SetError(SEC_ERROR_INADEQUATE_KEY_USAGE, 0);
      }
    }
  }
  return rv;
}

SECStatus
CertVerifier::InsanityVerifyCert(
                   CERTCertificate* cert,
                   const SECCertificateUsage usage,
                   const PRTime time,
                   void* pinArg,
                   const Flags flags,
       const SECItem* stapledOCSPResponse,
   insanity::pkix::ScopedCERTCertList* validationChain)
{
  PR_LOG(gCertVerifierLog, PR_LOG_DEBUG, ("Top of InsanityVerifyCert\n"));
  SECStatus rv;

  
  
  
  

  
  

  insanity::pkix::ScopedCERTCertList builtChain;
  switch (usage) {
    case certificateUsageSSLClient: {
      
      
      NSSCertDBTrustDomain trustDomain(trustEmail, mOCSPDownloadEnabled,
                                       mOCSPStrict, pinArg);
      rv = BuildCertChain(trustDomain, cert, time, MustBeEndEntity,
                          KU_DIGITAL_SIGNATURE,
                          SEC_OID_EXT_KEY_USAGE_CLIENT_AUTH,
                          SEC_OID_X509_ANY_POLICY,
                          stapledOCSPResponse, builtChain);
      break;
    }

    case certificateUsageSSLServer: {
      
      
      
      NSSCertDBTrustDomain trustDomain(trustSSL, mOCSPDownloadEnabled,
                                       mOCSPStrict, pinArg);
      rv = BuildCertChainForOneKeyUsage(trustDomain, cert, time,
                                        KU_DIGITAL_SIGNATURE, 
                                        KU_KEY_ENCIPHERMENT, 
                                        KU_KEY_AGREEMENT, 
                                        SEC_OID_EXT_KEY_USAGE_SERVER_AUTH,
                                        SEC_OID_X509_ANY_POLICY,
                                        stapledOCSPResponse, builtChain);
      break;
    }

    case certificateUsageSSLCA: {
      NSSCertDBTrustDomain trustDomain(trustSSL, mOCSPDownloadEnabled,
                                       mOCSPStrict, pinArg);
      rv = BuildCertChain(trustDomain, cert, time, MustBeCA,
                          KU_KEY_CERT_SIGN,
                          SEC_OID_EXT_KEY_USAGE_SERVER_AUTH,
                          SEC_OID_X509_ANY_POLICY,
                          stapledOCSPResponse, builtChain);
      break;
    }

    case certificateUsageEmailSigner: {
      NSSCertDBTrustDomain trustDomain(trustEmail, mOCSPDownloadEnabled,
                                       mOCSPStrict, pinArg);
      rv = BuildCertChain(trustDomain, cert, time, MustBeEndEntity,
                          KU_DIGITAL_SIGNATURE,
                          SEC_OID_EXT_KEY_USAGE_EMAIL_PROTECT,
                          SEC_OID_X509_ANY_POLICY,
                          stapledOCSPResponse, builtChain);
      break;
    }

    case certificateUsageEmailRecipient: {
      
      
      
      NSSCertDBTrustDomain trustDomain(trustEmail, mOCSPDownloadEnabled,
                                       mOCSPStrict, pinArg);
      rv = BuildCertChainForOneKeyUsage(trustDomain, cert, time,
                                        KU_KEY_ENCIPHERMENT, 
                                        KU_KEY_AGREEMENT, 
                                        0,
                                        SEC_OID_EXT_KEY_USAGE_EMAIL_PROTECT,
                                        SEC_OID_X509_ANY_POLICY,
                                        stapledOCSPResponse, builtChain);
      break;
    }

    case certificateUsageObjectSigner: {
      NSSCertDBTrustDomain trustDomain(trustObjectSigning,
                                       mOCSPDownloadEnabled, mOCSPStrict,
                                       pinArg);
      rv = BuildCertChain(trustDomain, cert, time, MustBeEndEntity,
                          KU_DIGITAL_SIGNATURE,
                          SEC_OID_EXT_KEY_USAGE_CODE_SIGN,
                          SEC_OID_X509_ANY_POLICY,
                          stapledOCSPResponse, builtChain);
      break;
    }

    case certificateUsageVerifyCA:
    case certificateUsageStatusResponder: {
      
      
      
      
      insanity::pkix::EndEntityOrCA endEntityOrCA;
      insanity::pkix::KeyUsages keyUsage;
      SECOidTag eku;
      if (usage == certificateUsageVerifyCA) {
        endEntityOrCA = MustBeCA;
        keyUsage = KU_KEY_CERT_SIGN;
        eku = SEC_OID_UNKNOWN;
      } else {
        endEntityOrCA = MustBeEndEntity;
        keyUsage = KU_DIGITAL_SIGNATURE;
        eku = SEC_OID_OCSP_RESPONDER;
      }

      NSSCertDBTrustDomain sslTrust(trustSSL,
                                    mOCSPDownloadEnabled, mOCSPStrict, pinArg);
      rv = BuildCertChain(sslTrust, cert, time, endEntityOrCA,
                          keyUsage, eku, SEC_OID_X509_ANY_POLICY,
                          stapledOCSPResponse, builtChain);
      if (rv == SECFailure && PR_GetError() == SEC_ERROR_UNKNOWN_ISSUER) {
        NSSCertDBTrustDomain emailTrust(trustEmail, mOCSPDownloadEnabled,
                                        mOCSPStrict, pinArg);
        rv = BuildCertChain(emailTrust, cert, time, endEntityOrCA, keyUsage,
                            eku, SEC_OID_X509_ANY_POLICY,
                            stapledOCSPResponse, builtChain);
        if (rv == SECFailure && SEC_ERROR_UNKNOWN_ISSUER) {
          NSSCertDBTrustDomain objectSigningTrust(trustObjectSigning,
                                                  mOCSPDownloadEnabled,
                                                  mOCSPStrict, pinArg);
          rv = BuildCertChain(objectSigningTrust, cert, time, endEntityOrCA,
                              keyUsage, eku, SEC_OID_X509_ANY_POLICY,
                              stapledOCSPResponse, builtChain);
        }
      }

      break;
    }

    default:
      PR_SetError(SEC_ERROR_INVALID_ARGS, 0);
      return SECFailure;
  }

  if (validationChain && rv == SECSuccess) {
    *validationChain = builtChain.release();
  }

  return rv;
}

SECStatus
CertVerifier::VerifyCert(CERTCertificate* cert,
             const SECItem* stapledOCSPResponse,
                         const SECCertificateUsage usage,
                         const PRTime time,
                         void* pinArg,
                         const Flags flags,
                          ScopedCERTCertList* validationChain,
                          SECOidTag* evOidPolicy,
                          CERTVerifyLog* verifyLog)
{
  if (!cert)
  {
    PR_NOT_REACHED("Invalid arguments to CertVerifier::VerifyCert");
    PORT_SetError(SEC_ERROR_INVALID_ARGS);
    return SECFailure;
  }
  if (validationChain) {
    *validationChain = nullptr;
  }
  if (evOidPolicy) {
    *evOidPolicy = SEC_OID_UNKNOWN;
  }

  switch(usage){
    case certificateUsageSSLClient:
    case certificateUsageSSLServer:
    case certificateUsageSSLCA:
    case certificateUsageEmailSigner:
    case certificateUsageEmailRecipient:
    case certificateUsageObjectSigner:
    case certificateUsageStatusResponder:
      break;
    default:
      PORT_SetError(SEC_ERROR_INVALID_ARGS);
      return SECFailure;
  }

  if ((flags & FLAG_MUST_BE_EV) && usage != certificateUsageSSLServer) {
      PORT_SetError(SEC_ERROR_INVALID_ARGS);
      return SECFailure;
  }

#ifndef NSS_NO_LIBPKIX
  ScopedCERTCertList trustAnchors;
  SECStatus rv;
  SECOidTag evPolicy = SEC_OID_UNKNOWN;

  
  if (usage == certificateUsageSSLServer) {
    SECStatus srv = GetFirstEVPolicy(cert, evPolicy);
    if (srv == SECSuccess) {
      if (evPolicy != SEC_OID_UNKNOWN) {
        trustAnchors = GetRootsForOid(evPolicy);
      }
      if (!trustAnchors) {
        return SECFailure;
      }
      
      
      
      if (CERT_LIST_EMPTY(trustAnchors)) {
        evPolicy = SEC_OID_UNKNOWN;
      }
    } else {
      
      if (flags & FLAG_MUST_BE_EV) {
        PORT_SetError(SEC_ERROR_EXTENSION_NOT_FOUND);
        return SECFailure;
      }
      
      evPolicy = SEC_OID_UNKNOWN;
    }
    if ((evPolicy == SEC_OID_UNKNOWN) && (flags & FLAG_MUST_BE_EV)) {
      PORT_SetError(SEC_ERROR_UNKNOWN_ISSUER);
      return SECFailure;
    }
  }

  PR_ASSERT(evPolicy == SEC_OID_UNKNOWN || trustAnchors);

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
    PR_LOG(gCertVerifierLog, PR_LOG_DEBUG, ("VerifyCert: setting up validation chain outparam.\n"));
    validationChainLocation = i;
    cvout[i].type = cert_po_certList;
    cvout[i].value.pointer.chain = nullptr;
    ++i;
    validationTrustAnchorLocation = i;
    cvout[i].type = cert_po_trustAnchor;
    cvout[i].value.pointer.cert = nullptr;
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
  CERTValInParam cvin[7];

  
  cvin[0].type = cert_pi_useAIACertFetch;
  cvin[0].value.scalar.b = mMissingCertDownloadEnabled && !localOnly;
  cvin[1].type = cert_pi_revocationFlags;
  cvin[1].value.pointer.revocation = &rev;
  cvin[2].type = cert_pi_date;
  cvin[2].value.scalar.time = time;
  i = 3;

  CERTChainVerifyCallback callbackContainer;
  if (usage == certificateUsageSSLServer) {
    callbackContainer.isChainValid = chainValidationCallback;
    callbackContainer.isChainValidArg = nullptr;
    cvin[i].type = cert_pi_chainVerifyCallback;
    cvin[i].value.pointer.chainVerifyCallback = &callbackContainer;
    ++i;
  }

  const size_t evParamLocation = i;

  if (evPolicy != SEC_OID_UNKNOWN) {
    
    
    
    uint64_t ocspRevMethodFlags =
      CERT_REV_M_TEST_USING_THIS_METHOD
      | ((mOCSPDownloadEnabled && !localOnly) ?
          CERT_REV_M_ALLOW_NETWORK_FETCHING : CERT_REV_M_FORBID_NETWORK_FETCHING)
      | CERT_REV_M_ALLOW_IMPLICIT_DEFAULT_SOURCE
      | CERT_REV_M_REQUIRE_INFO_ON_MISSING_SOURCE
      | CERT_REV_M_IGNORE_MISSING_FRESH_INFO
      | CERT_REV_M_STOP_TESTING_ON_FRESH_INFO
      | (mOCSPGETEnabled ? 0 : CERT_REV_M_FORCE_POST_METHOD_FOR_OCSP);

    rev.leafTests.cert_rev_flags_per_method[cert_revocation_method_crl] =
    rev.chainTests.cert_rev_flags_per_method[cert_revocation_method_crl]
      = CERT_REV_M_DO_NOT_TEST_USING_THIS_METHOD;

    rev.leafTests.cert_rev_flags_per_method[cert_revocation_method_ocsp] =
    rev.chainTests.cert_rev_flags_per_method[cert_revocation_method_ocsp]
      = ocspRevMethodFlags;

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
    PR_ASSERT(trustAnchors);
    cvin[i].type = cert_pi_trustAnchors;
    cvin[i].value.pointer.chain = trustAnchors.get();
    ++i;

    cvin[i].type = cert_pi_end;

    rv = CERT_PKIXVerifyCert(cert, usage, cvin, cvout, pinArg);
    if (rv == SECSuccess) {
      if (evOidPolicy) {
        *evOidPolicy = evPolicy;
      }
      PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
             ("VerifyCert: successful CERT_PKIXVerifyCert(ev) \n"));
      goto pkix_done;
    }
    PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
           ("VerifyCert: failed CERT_PKIXVerifyCert(ev)\n"));
    if (flags & FLAG_MUST_BE_EV) {
      return rv;
    }
    if (validationChain) {
      destroyCertListThatShouldNotExist(
        &cvout[validationChainLocation].value.pointer.chain);
    }

    if (verifyLog) {
      
      CERTVerifyLogNode* i_node;
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
#endif

  
  
  if (flags & FLAG_MUST_BE_EV) {
    PR_ASSERT(*evOidPolicy == SEC_OID_UNKNOWN);
#ifdef NSS_NO_LIBPKIX
    PR_SetError(SEC_ERROR_INVALID_ARGS, 0);
#else
    PR_SetError(PR_INVALID_STATE_ERROR, 0);
#endif
    return SECFailure;
  }

  if (mImplementation == insanity) {
    return InsanityVerifyCert(cert, usage, time, pinArg, flags,
                              stapledOCSPResponse, validationChain);
  }

  if (mImplementation == classic) {
    
    
    return ClassicVerifyCert(cert, usage, time, pinArg, validationChain,
                             verifyLog);
  }

#ifdef NSS_NO_LIBPKIX
  PR_NOT_REACHED("libpkix implementation chosen but not even compiled in");
  PR_SetError(PR_INVALID_STATE_ERROR, 0);
  return SECFailure;
#else
  PR_ASSERT(mImplementation == libpkix);

  
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

    | (mOCSPGETEnabled ? 0 : CERT_REV_M_FORCE_POST_METHOD_FOR_OCSP);
    ;

  rev.leafTests.preferred_methods[0] =
  rev.chainTests.preferred_methods[0] = cert_revocation_method_ocsp;

  rev.leafTests.cert_rev_method_independent_flags =
  rev.chainTests.cert_rev_method_independent_flags =
    
    CERT_REV_MI_TEST_ALL_LOCAL_INFORMATION_FIRST;

  
  cvin[evParamLocation].type = cert_pi_end;

  PR_LOG(gCertVerifierLog, PR_LOG_DEBUG, ("VerifyCert: calling CERT_PKIXVerifyCert(dv) \n"));
  rv = CERT_PKIXVerifyCert(cert, usage, cvin, cvout, pinArg);

pkix_done:
  if (validationChain) {
    PR_LOG(gCertVerifierLog, PR_LOG_DEBUG, ("VerifyCert: validation chain requested\n"));
    ScopedCERTCertificate trustAnchor(cvout[validationTrustAnchorLocation].value.pointer.cert);

    if (rv == SECSuccess) {
      if (! cvout[validationChainLocation].value.pointer.chain) {
        PR_SetError(PR_UNKNOWN_ERROR, 0);
        return SECFailure;
      }
      PR_LOG(gCertVerifierLog, PR_LOG_DEBUG, ("VerifyCert: I have a chain\n"));
      *validationChain = cvout[validationChainLocation].value.pointer.chain;
      if (trustAnchor) {
        
        
        
        if (!CERT_CompareCerts(trustAnchor.get(), cert)) {
          PR_LOG(gCertVerifierLog, PR_LOG_DEBUG, ("VerifyCert:  adding issuer to tail for display\n"));
          
          ScopedCERTCertificate tempCert(CERT_DupCertificate(trustAnchor.get()));
          rv = CERT_AddCertToListTail(validationChain->get(), tempCert.get());
          if (rv == SECSuccess) {
            tempCert.release(); 
          } else {
            *validationChain = nullptr;
          }
        }
      }
    } else {
      destroyCertListThatShouldNotExist(
        &cvout[validationChainLocation].value.pointer.chain);
    }
  }

  return rv;
#endif
}

SECStatus
CertVerifier::VerifySSLServerCert(CERTCertificate* peerCert,
                      const SECItem* stapledOCSPResponse,
                                  PRTime time,
                      void* pinarg,
                                  const char* hostname,
                                  bool saveIntermediatesInPermanentDatabase,
                  insanity::pkix::ScopedCERTCertList* certChainOut,
                  SECOidTag* evOidPolicy)
{
  PR_ASSERT(peerCert);
  
  PR_ASSERT(hostname);
  PR_ASSERT(hostname[0]);

  if (certChainOut) {
    *certChainOut = nullptr;
  }
  if (evOidPolicy) {
    *evOidPolicy = SEC_OID_UNKNOWN;
  }

  if (!hostname || !hostname[0]) {
    PR_SetError(SSL_ERROR_BAD_CERT_DOMAIN, 0);
    return SECFailure;
  }

  
  
  ScopedCERTCertList validationChain;
  SECStatus rv = VerifyCert(peerCert, stapledOCSPResponse,
                            certificateUsageSSLServer, time,
                            pinarg, 0, &validationChain, evOidPolicy);
  if (rv != SECSuccess) {
    return rv;
  }

  rv = CERT_VerifyCertName(peerCert, hostname);
  if (rv != SECSuccess) {
    return rv;
  }

  if (saveIntermediatesInPermanentDatabase) {
    SaveIntermediateCerts(validationChain);
  }

  if (certChainOut) {
    *certChainOut = validationChain.release();
  }

  return SECSuccess;
}

} } 
