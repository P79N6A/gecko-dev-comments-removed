





#include "CertVerifier.h"

#include <stdint.h>

#include "ExtendedValidation.h"
#include "NSSCertDBTrustDomain.h"
#include "NSSErrorsService.h"
#include "PublicKeyPinningService.h"
#include "cert.h"
#include "pk11pub.h"
#include "pkix/pkix.h"
#include "pkix/pkixnss.h"
#include "prerror.h"
#include "secerr.h"
#include "sslerr.h"

using namespace mozilla::pkix;
using namespace mozilla::psm;

#ifdef PR_LOGGING
PRLogModuleInfo* gCertVerifierLog = nullptr;
#endif

namespace mozilla { namespace psm {

const CertVerifier::Flags CertVerifier::FLAG_LOCAL_ONLY = 1;
const CertVerifier::Flags CertVerifier::FLAG_MUST_BE_EV = 2;

CertVerifier::CertVerifier(ocsp_download_config odc,
                           ocsp_strict_config osc,
                           ocsp_get_config ogc,
                           pinning_enforcement_config pel)
  : mOCSPDownloadEnabled(odc == ocsp_on)
  , mOCSPStrict(osc == ocsp_strict)
  , mOCSPGETEnabled(ogc == ocsp_get_enabled)
  , mPinningEnforcementLevel(pel)
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

SECStatus
IsCertBuiltInRoot(CERTCertificate* cert, bool& result) {
  result = false;
  ScopedPK11SlotList slots;
  slots = PK11_GetAllSlotsForCert(cert, nullptr);
  if (!slots) {
    if (PORT_GetError() == SEC_ERROR_NO_TOKEN) {
      
      return SECSuccess;
    }
    return SECFailure;
  }
  for (PK11SlotListElement* le = slots->head; le; le = le->next) {
    char* token = PK11_GetTokenName(le->slot);
    PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
           ("BuiltInRoot? subject=%s token=%s",cert->subjectName, token));
    if (strcmp("Builtin Object Token", token) == 0) {
      result = true;
      return SECSuccess;
    }
  }
  return SECSuccess;
}

struct ChainValidationCallbackState
{
  const char* hostname;
  const CertVerifier::pinning_enforcement_config pinningEnforcementLevel;
  const SECCertificateUsage usage;
  const Time time;
};

SECStatus chainValidationCallback(void* state, const CERTCertList* certList,
                                  PRBool* chainOK)
{
  ChainValidationCallbackState* callbackState =
    reinterpret_cast<ChainValidationCallbackState*>(state);

  *chainOK = PR_FALSE;

  PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
         ("verifycert: Inside the Callback \n"));

  
  if (!certList) {
    PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
           ("verifycert: Short circuit, callback, sanity check failed \n"));
    PR_SetError(PR_INVALID_STATE_ERROR, 0);
    return SECFailure;
  }
  if (!callbackState) {
    PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
           ("verifycert: Short circuit, callback, no state! \n"));
    PR_SetError(PR_INVALID_STATE_ERROR, 0);
    return SECFailure;
  }

  if (callbackState->usage != certificateUsageSSLServer ||
      callbackState->pinningEnforcementLevel == CertVerifier::pinningDisabled) {
    PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
           ("verifycert: Callback shortcut pel=%d \n",
            callbackState->pinningEnforcementLevel));
    *chainOK = PR_TRUE;
    return SECSuccess;
  }

  for (CERTCertListNode* node = CERT_LIST_HEAD(certList);
       !CERT_LIST_END(node, certList);
       node = CERT_LIST_NEXT(node)) {
    CERTCertificate* currentCert = node->cert;
    if (CERT_LIST_END(CERT_LIST_NEXT(node), certList)) {
      bool isBuiltInRoot = false;
      SECStatus srv = IsCertBuiltInRoot(currentCert, isBuiltInRoot);
      if (srv != SECSuccess) {
        PR_LOG(gCertVerifierLog, PR_LOG_DEBUG, ("Is BuiltInRoot failure"));
        return srv;
      }
      
      
      
      if (!isBuiltInRoot &&
          (callbackState->pinningEnforcementLevel ==
             CertVerifier::pinningAllowUserCAMITM)) {
        *chainOK = PR_TRUE;
        return SECSuccess;
      }
    }
  }

  bool enforceTestMode = (callbackState->pinningEnforcementLevel ==
                          CertVerifier::pinningEnforceTestMode);
  *chainOK = PublicKeyPinningService::
    ChainHasValidPins(certList, callbackState->hostname, callbackState->time,
                      enforceTestMode);

  return SECSuccess;
}

static Result
BuildCertChainForOneKeyUsage(TrustDomain& trustDomain, Input certDER,
                             Time time, KeyUsage ku1, KeyUsage ku2,
                             KeyUsage ku3, KeyPurposeId eku,
                             const CertPolicyId& requiredPolicy,
                             const Input* stapledOCSPResponse)
{
  Result rv = BuildCertChain(trustDomain, certDER, time,
                             EndEntityOrCA::MustBeEndEntity, ku1,
                             eku, requiredPolicy, stapledOCSPResponse);
  if (rv == Result::ERROR_INADEQUATE_KEY_USAGE) {
    rv = BuildCertChain(trustDomain, certDER, time,
                        EndEntityOrCA::MustBeEndEntity, ku2,
                        eku, requiredPolicy, stapledOCSPResponse);
    if (rv == Result::ERROR_INADEQUATE_KEY_USAGE) {
      rv = BuildCertChain(trustDomain, certDER, time,
                          EndEntityOrCA::MustBeEndEntity, ku3,
                          eku, requiredPolicy, stapledOCSPResponse);
      if (rv != Success) {
        rv = Result::ERROR_INADEQUATE_KEY_USAGE;
      }
    }
  }
  return rv;
}

SECStatus
CertVerifier::VerifyCert(CERTCertificate* cert, SECCertificateUsage usage,
                         Time time, void* pinArg, const char* hostname,
                         const Flags flags,
             const SECItem* stapledOCSPResponseSECItem,
         ScopedCERTCertList* builtChain,
         SECOidTag* evOidPolicy)
{
  PR_LOG(gCertVerifierLog, PR_LOG_DEBUG, ("Top of VerifyCert\n"));

  PR_ASSERT(cert);
  PR_ASSERT(usage == certificateUsageSSLServer || !(flags & FLAG_MUST_BE_EV));

  if (builtChain) {
    *builtChain = nullptr;
  }
  if (evOidPolicy) {
    *evOidPolicy = SEC_OID_UNKNOWN;
  }

  if (!cert ||
      (usage != certificateUsageSSLServer && (flags & FLAG_MUST_BE_EV))) {
    PR_SetError(SEC_ERROR_INVALID_ARGS, 0);
    return SECFailure;
  }

  Result rv;

  Input certDER;
  rv = certDER.Init(cert->derCert.data, cert->derCert.len);
  if (rv != Success) {
    PR_SetError(MapResultToPRErrorCode(rv), 0);
    return SECFailure;
  }

  ChainValidationCallbackState callbackState = {
    hostname, mPinningEnforcementLevel, usage, time
  };
  CERTChainVerifyCallback callbackContainer;
  callbackContainer.isChainValid = chainValidationCallback;
  callbackContainer.isChainValidArg = &callbackState;

  NSSCertDBTrustDomain::OCSPFetching ocspFetching
    = !mOCSPDownloadEnabled ||
      (flags & FLAG_LOCAL_ONLY) ? NSSCertDBTrustDomain::NeverFetchOCSP
    : !mOCSPStrict              ? NSSCertDBTrustDomain::FetchOCSPForDVSoftFail
                                : NSSCertDBTrustDomain::FetchOCSPForDVHardFail;

  ocsp_get_config ocspGETConfig = mOCSPGETEnabled ? ocsp_get_enabled
                                                  : ocsp_get_disabled;

  Input stapledOCSPResponseInput;
  const Input* stapledOCSPResponse = nullptr;
  if (stapledOCSPResponseSECItem) {
    rv = stapledOCSPResponseInput.Init(stapledOCSPResponseSECItem->data,
                                       stapledOCSPResponseSECItem->len);
    if (rv != Success) {
      
      PR_SetError(SEC_ERROR_OCSP_MALFORMED_RESPONSE, 0);
      return SECFailure;
    }
    stapledOCSPResponse = &stapledOCSPResponseInput;
  }

  switch (usage) {
    case certificateUsageSSLClient: {
      
      
      NSSCertDBTrustDomain trustDomain(trustEmail, ocspFetching, mOCSPCache,
                                       pinArg, ocspGETConfig, nullptr,
                                       builtChain);
      rv = BuildCertChain(trustDomain, certDER, time,
                          EndEntityOrCA::MustBeEndEntity,
                          KeyUsage::digitalSignature,
                          KeyPurposeId::id_kp_clientAuth,
                          CertPolicyId::anyPolicy, stapledOCSPResponse);
      break;
    }

    case certificateUsageSSLServer: {
      
      
      

#ifndef MOZ_NO_EV_CERTS
      
      CertPolicyId evPolicy;
      SECOidTag evPolicyOidTag;
      SECStatus srv = GetFirstEVPolicy(cert, evPolicy, evPolicyOidTag);
      if (srv == SECSuccess) {
        NSSCertDBTrustDomain
          trustDomain(trustSSL,
                      ocspFetching == NSSCertDBTrustDomain::NeverFetchOCSP
                        ? NSSCertDBTrustDomain::LocalOnlyOCSPForEV
                        : NSSCertDBTrustDomain::FetchOCSPForEV,
                      mOCSPCache, pinArg, ocspGETConfig,
                      &callbackContainer, builtChain);
        rv = BuildCertChainForOneKeyUsage(trustDomain, certDER, time,
                                          KeyUsage::digitalSignature,
                                          KeyUsage::keyEncipherment, 
                                          KeyUsage::keyAgreement,    
                                          KeyPurposeId::id_kp_serverAuth,
                                          evPolicy, stapledOCSPResponse);
        if (rv == Success) {
          if (evOidPolicy) {
            *evOidPolicy = evPolicyOidTag;
          }
          break;
        }
      }
#endif

      if (flags & FLAG_MUST_BE_EV) {
        rv = Result::ERROR_POLICY_VALIDATION_FAILED;
        break;
      }

      
      NSSCertDBTrustDomain trustDomain(trustSSL, ocspFetching, mOCSPCache,
                                       pinArg, ocspGETConfig, &callbackContainer,
                                       builtChain);
      rv = BuildCertChainForOneKeyUsage(trustDomain, certDER, time,
                                        KeyUsage::digitalSignature, 
                                        KeyUsage::keyEncipherment, 
                                        KeyUsage::keyAgreement, 
                                        KeyPurposeId::id_kp_serverAuth,
                                        CertPolicyId::anyPolicy,
                                        stapledOCSPResponse);
      break;
    }

    case certificateUsageSSLCA: {
      NSSCertDBTrustDomain trustDomain(trustSSL, ocspFetching, mOCSPCache,
                                       pinArg, ocspGETConfig, nullptr,
                                       builtChain);
      rv = BuildCertChain(trustDomain, certDER, time,
                          EndEntityOrCA::MustBeCA, KeyUsage::keyCertSign,
                          KeyPurposeId::id_kp_serverAuth,
                          CertPolicyId::anyPolicy, stapledOCSPResponse);
      break;
    }

    case certificateUsageEmailSigner: {
      NSSCertDBTrustDomain trustDomain(trustEmail, ocspFetching, mOCSPCache,
                                       pinArg, ocspGETConfig, nullptr,
                                       builtChain);
      rv = BuildCertChain(trustDomain, certDER, time,
                          EndEntityOrCA::MustBeEndEntity,
                          KeyUsage::digitalSignature,
                          KeyPurposeId::id_kp_emailProtection,
                          CertPolicyId::anyPolicy, stapledOCSPResponse);
      break;
    }

    case certificateUsageEmailRecipient: {
      
      
      
      NSSCertDBTrustDomain trustDomain(trustEmail, ocspFetching, mOCSPCache,
                                       pinArg, ocspGETConfig, nullptr,
                                       builtChain);
      rv = BuildCertChain(trustDomain, certDER, time,
                          EndEntityOrCA::MustBeEndEntity,
                          KeyUsage::keyEncipherment, 
                          KeyPurposeId::id_kp_emailProtection,
                          CertPolicyId::anyPolicy, stapledOCSPResponse);
      if (rv == Result::ERROR_INADEQUATE_KEY_USAGE) {
        rv = BuildCertChain(trustDomain, certDER, time,
                            EndEntityOrCA::MustBeEndEntity,
                            KeyUsage::keyAgreement, 
                            KeyPurposeId::id_kp_emailProtection,
                            CertPolicyId::anyPolicy, stapledOCSPResponse);
      }
      break;
    }

    case certificateUsageObjectSigner: {
      NSSCertDBTrustDomain trustDomain(trustObjectSigning, ocspFetching,
                                       mOCSPCache, pinArg, ocspGETConfig,
                                       nullptr, builtChain);
      rv = BuildCertChain(trustDomain, certDER, time,
                          EndEntityOrCA::MustBeEndEntity,
                          KeyUsage::digitalSignature,
                          KeyPurposeId::id_kp_codeSigning,
                          CertPolicyId::anyPolicy, stapledOCSPResponse);
      break;
    }

    case certificateUsageVerifyCA:
    case certificateUsageStatusResponder: {
      
      
      
      mozilla::pkix::EndEntityOrCA endEntityOrCA;
      mozilla::pkix::KeyUsage keyUsage;
      KeyPurposeId eku;
      if (usage == certificateUsageVerifyCA) {
        endEntityOrCA = EndEntityOrCA::MustBeCA;
        keyUsage = KeyUsage::keyCertSign;
        eku = KeyPurposeId::anyExtendedKeyUsage;
      } else {
        endEntityOrCA = EndEntityOrCA::MustBeEndEntity;
        keyUsage = KeyUsage::digitalSignature;
        eku = KeyPurposeId::id_kp_OCSPSigning;
      }

      NSSCertDBTrustDomain sslTrust(trustSSL, ocspFetching, mOCSPCache, pinArg,
                                    ocspGETConfig, nullptr, builtChain);
      rv = BuildCertChain(sslTrust, certDER, time, endEntityOrCA,
                          keyUsage, eku, CertPolicyId::anyPolicy,
                          stapledOCSPResponse);
      if (rv == Result::ERROR_UNKNOWN_ISSUER) {
        NSSCertDBTrustDomain emailTrust(trustEmail, ocspFetching, mOCSPCache,
                                        pinArg, ocspGETConfig, nullptr,
                                        builtChain);
        rv = BuildCertChain(emailTrust, certDER, time, endEntityOrCA,
                            keyUsage, eku, CertPolicyId::anyPolicy,
                            stapledOCSPResponse);
        if (rv == Result::ERROR_UNKNOWN_ISSUER) {
          NSSCertDBTrustDomain objectSigningTrust(trustObjectSigning,
                                                  ocspFetching, mOCSPCache,
                                                  pinArg, ocspGETConfig,
                                                  nullptr, builtChain);
          rv = BuildCertChain(objectSigningTrust, certDER, time,
                              endEntityOrCA, keyUsage, eku,
                              CertPolicyId::anyPolicy, stapledOCSPResponse);
        }
      }

      break;
    }

    default:
      rv = Result::FATAL_ERROR_INVALID_ARGS;
  }

  if (rv != Success) {
    if (rv != Result::ERROR_KEY_PINNING_FAILURE) {
      ScopedCERTCertificate certCopy(CERT_DupCertificate(cert));
      if (!certCopy) {
        return SECFailure;
      }
      ScopedCERTCertList certList(CERT_NewCertList());
      if (!certList) {
        return SECFailure;
      }
      SECStatus srv = CERT_AddCertToListTail(certList.get(), certCopy.get());
      if (srv != SECSuccess) {
        return SECFailure;
      }
      certCopy.forget(); 
      PRBool chainOK = false;
      srv = chainValidationCallback(&callbackState, certList, &chainOK);
      if (srv != SECSuccess) {
        return SECFailure;
      }
      if (!chainOK) {
        rv = Result::ERROR_KEY_PINNING_FAILURE;
      }
    }
    PR_SetError(MapResultToPRErrorCode(rv), 0);
    return SECFailure;
  }

  return SECSuccess;
}

SECStatus
CertVerifier::VerifySSLServerCert(CERTCertificate* peerCert,
                      const SECItem* stapledOCSPResponse,
                                  Time time,
                      void* pinarg,
                                  const char* hostname,
                                  bool saveIntermediatesInPermanentDatabase,
                                  Flags flags,
                  ScopedCERTCertList* builtChain,
                  SECOidTag* evOidPolicy)
{
  PR_ASSERT(peerCert);
  
  PR_ASSERT(hostname);
  PR_ASSERT(hostname[0]);

  if (builtChain) {
    *builtChain = nullptr;
  }
  if (evOidPolicy) {
    *evOidPolicy = SEC_OID_UNKNOWN;
  }

  if (!hostname || !hostname[0]) {
    PR_SetError(SSL_ERROR_BAD_CERT_DOMAIN, 0);
    return SECFailure;
  }

  ScopedCERTCertList builtChainTemp;
  
  
  SECStatus rv = VerifyCert(peerCert, certificateUsageSSLServer, time, pinarg,
                            hostname, flags, stapledOCSPResponse,
                            &builtChainTemp, evOidPolicy);
  if (rv != SECSuccess) {
    return rv;
  }

  rv = CERT_VerifyCertName(peerCert, hostname);
  if (rv != SECSuccess) {
    return rv;
  }

  if (saveIntermediatesInPermanentDatabase) {
    SaveIntermediateCerts(builtChainTemp);
  }

  if (builtChain) {
    *builtChain = builtChainTemp.forget();
  }

  return SECSuccess;
}

} } 
