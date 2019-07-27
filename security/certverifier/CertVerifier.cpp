





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

CertVerifier::CertVerifier(OcspDownloadConfig odc,
                           OcspStrictConfig osc,
                           OcspGetConfig ogc,
                           PinningMode pinningMode)
  : mOCSPDownloadEnabled(odc == ocspOn)
  , mOCSPStrict(osc == ocspStrict)
  , mOCSPGETEnabled(ogc == ocspGetEnabled)
  , mPinningMode(pinningMode)
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

Result
CertListContainsExpectedKeys(const CERTCertList* certList,
                             const char* hostname, Time time,
                             CertVerifier::PinningMode pinningMode)
{
  if (pinningMode == CertVerifier::pinningDisabled) {
    PR_LOG(gCertVerifierLog, PR_LOG_DEBUG,
           ("Pinning is disabled; not checking keys."));
    return Success;
  }

  if (!certList) {
    return Result::FATAL_ERROR_INVALID_ARGS;
  }

  CERTCertListNode* rootNode = CERT_LIST_TAIL(certList);
  if (CERT_LIST_END(rootNode, certList)) {
    return Result::FATAL_ERROR_INVALID_ARGS;
  }

  bool isBuiltInRoot = false;
  SECStatus srv = IsCertBuiltInRoot(rootNode->cert, isBuiltInRoot);
  if (srv != SECSuccess) {
    return MapPRErrorCodeToResult(PR_GetError());
  }
  
  
  
  if (!isBuiltInRoot && pinningMode == CertVerifier::pinningAllowUserCAMITM) {
    return Success;
  }

  bool enforceTestMode = (pinningMode == CertVerifier::pinningEnforceTestMode);
  bool chainHasValidPins;
  nsresult rv = PublicKeyPinningService::ChainHasValidPins(certList,
                                                           hostname,
                                                           time,
                                                           enforceTestMode,
                                                           chainHasValidPins);
  if (NS_FAILED(rv)) {
    return Result::FATAL_ERROR_LIBRARY_FAILURE;
  }
  if (!chainHasValidPins) {
    return Result::ERROR_KEY_PINNING_FAILURE;
  }

  return Success;
}

static Result
BuildCertChainForOneKeyUsage(NSSCertDBTrustDomain& trustDomain, Input certDER,
                             Time time, KeyUsage ku1, KeyUsage ku2,
                             KeyUsage ku3, KeyPurposeId eku,
                             const CertPolicyId& requiredPolicy,
                             const Input* stapledOCSPResponse,
                              CertVerifier::OCSPStaplingStatus*
                                                ocspStaplingStatus)
{
  trustDomain.ResetOCSPStaplingStatus();
  Result rv = BuildCertChain(trustDomain, certDER, time,
                             EndEntityOrCA::MustBeEndEntity, ku1,
                             eku, requiredPolicy, stapledOCSPResponse);
  if (rv == Result::ERROR_INADEQUATE_KEY_USAGE) {
    trustDomain.ResetOCSPStaplingStatus();
    rv = BuildCertChain(trustDomain, certDER, time,
                        EndEntityOrCA::MustBeEndEntity, ku2,
                        eku, requiredPolicy, stapledOCSPResponse);
    if (rv == Result::ERROR_INADEQUATE_KEY_USAGE) {
      trustDomain.ResetOCSPStaplingStatus();
      rv = BuildCertChain(trustDomain, certDER, time,
                          EndEntityOrCA::MustBeEndEntity, ku3,
                          eku, requiredPolicy, stapledOCSPResponse);
      if (rv != Success) {
        rv = Result::ERROR_INADEQUATE_KEY_USAGE;
      }
    }
  }
  if (ocspStaplingStatus) {
    *ocspStaplingStatus = trustDomain.GetOCSPStaplingStatus();
  }
  return rv;
}

static const unsigned int MIN_RSA_BITS = 2048;
static const unsigned int MIN_RSA_BITS_WEAK = 1024;

SECStatus
CertVerifier::VerifyCert(CERTCertificate* cert, SECCertificateUsage usage,
                         Time time, void* pinArg, const char* hostname,
                         const Flags flags,
             const SECItem* stapledOCSPResponseSECItem,
         ScopedCERTCertList* builtChain,
         SECOidTag* evOidPolicy,
         OCSPStaplingStatus* ocspStaplingStatus,
         KeySizeStatus* keySizeStatus)
{
  PR_LOG(gCertVerifierLog, PR_LOG_DEBUG, ("Top of VerifyCert\n"));

  PR_ASSERT(cert);
  PR_ASSERT(usage == certificateUsageSSLServer || !(flags & FLAG_MUST_BE_EV));
  PR_ASSERT(usage == certificateUsageSSLServer || !keySizeStatus);

  if (builtChain) {
    *builtChain = nullptr;
  }
  if (evOidPolicy) {
    *evOidPolicy = SEC_OID_UNKNOWN;
  }
  if (ocspStaplingStatus) {
    if (usage != certificateUsageSSLServer) {
      PR_SetError(SEC_ERROR_INVALID_ARGS, 0);
      return SECFailure;
    }
    *ocspStaplingStatus = OCSP_STAPLING_NEVER_CHECKED;
  }

  if (keySizeStatus) {
    if (usage != certificateUsageSSLServer) {
      PR_SetError(SEC_ERROR_INVALID_ARGS, 0);
      return SECFailure;
    }
    *keySizeStatus = KeySizeStatus::NeverChecked;
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

  NSSCertDBTrustDomain::OCSPFetching ocspFetching
    = !mOCSPDownloadEnabled ||
      (flags & FLAG_LOCAL_ONLY) ? NSSCertDBTrustDomain::NeverFetchOCSP
    : !mOCSPStrict              ? NSSCertDBTrustDomain::FetchOCSPForDVSoftFail
                                : NSSCertDBTrustDomain::FetchOCSPForDVHardFail;

  OcspGetConfig ocspGETConfig = mOCSPGETEnabled ? ocspGetEnabled
                                                : ocspGetDisabled;

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
                                       pinArg, ocspGETConfig, pinningDisabled,
                                       MIN_RSA_BITS_WEAK, nullptr, builtChain);
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
                      mOCSPCache, pinArg, ocspGETConfig, mPinningMode,
                      MIN_RSA_BITS, hostname, builtChain);
        rv = BuildCertChainForOneKeyUsage(trustDomain, certDER, time,
                                          KeyUsage::digitalSignature,
                                          KeyUsage::keyEncipherment, 
                                          KeyUsage::keyAgreement,    
                                          KeyPurposeId::id_kp_serverAuth,
                                          evPolicy, stapledOCSPResponse,
                                          ocspStaplingStatus);
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
                                       pinArg, ocspGETConfig, mPinningMode,
                                       MIN_RSA_BITS, hostname, builtChain);
      rv = BuildCertChainForOneKeyUsage(trustDomain, certDER, time,
                                        KeyUsage::digitalSignature, 
                                        KeyUsage::keyEncipherment, 
                                        KeyUsage::keyAgreement, 
                                        KeyPurposeId::id_kp_serverAuth,
                                        CertPolicyId::anyPolicy,
                                        stapledOCSPResponse,
                                        ocspStaplingStatus);
      if (rv == Success) {
        if (keySizeStatus) {
          *keySizeStatus = KeySizeStatus::LargeMinimumSucceeded;
        }
        break;
      }

      
      NSSCertDBTrustDomain trustDomainWeak(trustSSL, ocspFetching, mOCSPCache,
                                           pinArg, ocspGETConfig, mPinningMode,
                                           MIN_RSA_BITS_WEAK, hostname,
                                           builtChain);
      rv = BuildCertChainForOneKeyUsage(trustDomainWeak, certDER, time,
                                        KeyUsage::digitalSignature, 
                                        KeyUsage::keyEncipherment, 
                                        KeyUsage::keyAgreement, 
                                        KeyPurposeId::id_kp_serverAuth,
                                        CertPolicyId::anyPolicy,
                                        stapledOCSPResponse,
                                        ocspStaplingStatus);
      if (keySizeStatus) {
        if (rv == Success) {
          *keySizeStatus = KeySizeStatus::CompatibilityRisk;
        } else {
          *keySizeStatus = KeySizeStatus::AlreadyBad;
        }
      }

      break;
    }

    case certificateUsageSSLCA: {
      NSSCertDBTrustDomain trustDomain(trustSSL, ocspFetching, mOCSPCache,
                                       pinArg, ocspGETConfig, pinningDisabled,
                                       MIN_RSA_BITS_WEAK, nullptr, builtChain);
      rv = BuildCertChain(trustDomain, certDER, time,
                          EndEntityOrCA::MustBeCA, KeyUsage::keyCertSign,
                          KeyPurposeId::id_kp_serverAuth,
                          CertPolicyId::anyPolicy, stapledOCSPResponse);
      break;
    }

    case certificateUsageEmailSigner: {
      NSSCertDBTrustDomain trustDomain(trustEmail, ocspFetching, mOCSPCache,
                                       pinArg, ocspGETConfig, pinningDisabled,
                                       MIN_RSA_BITS_WEAK, nullptr, builtChain);
      rv = BuildCertChain(trustDomain, certDER, time,
                          EndEntityOrCA::MustBeEndEntity,
                          KeyUsage::digitalSignature,
                          KeyPurposeId::id_kp_emailProtection,
                          CertPolicyId::anyPolicy, stapledOCSPResponse);
      if (rv == Result::ERROR_INADEQUATE_KEY_USAGE) {
        rv = BuildCertChain(trustDomain, certDER, time,
                            EndEntityOrCA::MustBeEndEntity,
                            KeyUsage::nonRepudiation,
                            KeyPurposeId::id_kp_emailProtection,
                            CertPolicyId::anyPolicy, stapledOCSPResponse);
      }
      break;
    }

    case certificateUsageEmailRecipient: {
      
      
      
      NSSCertDBTrustDomain trustDomain(trustEmail, ocspFetching, mOCSPCache,
                                       pinArg, ocspGETConfig, pinningDisabled,
                                       MIN_RSA_BITS_WEAK, nullptr, builtChain);
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
                                       pinningDisabled,
                                       MIN_RSA_BITS_WEAK, nullptr, builtChain);
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
                                    ocspGETConfig, pinningDisabled,
                                    MIN_RSA_BITS_WEAK, nullptr, builtChain);
      rv = BuildCertChain(sslTrust, certDER, time, endEntityOrCA,
                          keyUsage, eku, CertPolicyId::anyPolicy,
                          stapledOCSPResponse);
      if (rv == Result::ERROR_UNKNOWN_ISSUER) {
        NSSCertDBTrustDomain emailTrust(trustEmail, ocspFetching, mOCSPCache,
                                        pinArg, ocspGETConfig, pinningDisabled,
                                        MIN_RSA_BITS_WEAK, nullptr, builtChain);
        rv = BuildCertChain(emailTrust, certDER, time, endEntityOrCA,
                            keyUsage, eku, CertPolicyId::anyPolicy,
                            stapledOCSPResponse);
        if (rv == Result::ERROR_UNKNOWN_ISSUER) {
          NSSCertDBTrustDomain objectSigningTrust(trustObjectSigning,
                                                  ocspFetching, mOCSPCache,
                                                  pinArg, ocspGETConfig,
                                                  pinningDisabled,
                                                  MIN_RSA_BITS_WEAK,
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
                  SECOidTag* evOidPolicy,
                  OCSPStaplingStatus* ocspStaplingStatus,
                  KeySizeStatus* keySizeStatus)
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
                            &builtChainTemp, evOidPolicy, ocspStaplingStatus,
                            keySizeStatus);
  if (rv != SECSuccess) {
    return rv;
  }

  Input peerCertInput;
  Result result = peerCertInput.Init(peerCert->derCert.data,
                                     peerCert->derCert.len);
  if (result != Success) {
    PR_SetError(MapResultToPRErrorCode(result), 0);
    return SECFailure;
  }
  Input hostnameInput;
  result = hostnameInput.Init(uint8_t_ptr_cast(hostname), strlen(hostname));
  if (result != Success) {
    PR_SetError(SEC_ERROR_INVALID_ARGS, 0);
    return SECFailure;
  }
  result = CheckCertHostname(peerCertInput, hostnameInput);
  if (result != Success) {
    PR_SetError(MapResultToPRErrorCode(result), 0);
    return SECFailure;
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
