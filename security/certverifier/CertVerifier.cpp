





#include "CertVerifier.h"

#include <stdint.h>

#include "ExtendedValidation.h"
#include "NSSCertDBTrustDomain.h"
#include "NSSErrorsService.h"
#include "cert.h"
#include "pk11pub.h"
#include "pkix/pkix.h"
#include "pkix/pkixnss.h"
#include "prerror.h"
#include "secerr.h"
#include "sslerr.h"

using namespace mozilla::pkix;
using namespace mozilla::psm;

PRLogModuleInfo* gCertVerifierLog = nullptr;

namespace mozilla { namespace psm {

const CertVerifier::Flags CertVerifier::FLAG_LOCAL_ONLY = 1;
const CertVerifier::Flags CertVerifier::FLAG_MUST_BE_EV = 2;

CertVerifier::CertVerifier(OcspDownloadConfig odc,
                           OcspStrictConfig osc,
                           OcspGetConfig ogc,
                           uint32_t certShortLifetimeInDays,
                           PinningMode pinningMode)
  : mOCSPDownloadConfig(odc)
  , mOCSPStrict(osc == ocspStrict)
  , mOCSPGETEnabled(ogc == ocspGetEnabled)
  , mCertShortLifetimeInDays(certShortLifetimeInDays)
  , mPinningMode(pinningMode)
{
}

CertVerifier::~CertVerifier()
{
}

void
InitCertVerifierLog()
{
  if (!gCertVerifierLog) {
    gCertVerifierLog = PR_NewLogModule("certverifier");
  }
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
    MOZ_LOG(gCertVerifierLog, LogLevel::Debug,
           ("BuiltInRoot? subject=%s token=%s",cert->subjectName, token));
    if (strcmp("Builtin Object Token", token) == 0) {
      result = true;
      return SECSuccess;
    }
  }
  return SECSuccess;
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
         KeySizeStatus* keySizeStatus,
         SignatureDigestStatus* sigDigestStatus)
{
  MOZ_LOG(gCertVerifierLog, LogLevel::Debug, ("Top of VerifyCert\n"));

  PR_ASSERT(cert);
  PR_ASSERT(usage == certificateUsageSSLServer || !(flags & FLAG_MUST_BE_EV));
  PR_ASSERT(usage == certificateUsageSSLServer || !keySizeStatus);
  PR_ASSERT(usage == certificateUsageSSLServer || !sigDigestStatus);

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

  if (sigDigestStatus) {
    if (usage != certificateUsageSSLServer) {
      PR_SetError(SEC_ERROR_INVALID_ARGS, 0);
      return SECFailure;
    }
    *sigDigestStatus = SignatureDigestStatus::NeverChecked;
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

  
  
  NSSCertDBTrustDomain::OCSPFetching defaultOCSPFetching
    = (mOCSPDownloadConfig == ocspOff) ||
      (mOCSPDownloadConfig == ocspEVOnly) ||
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
      
      
      NSSCertDBTrustDomain trustDomain(trustEmail, defaultOCSPFetching,
                                       mOCSPCache, pinArg, ocspGETConfig,
                                       mCertShortLifetimeInDays,
                                       pinningDisabled, MIN_RSA_BITS_WEAK,
                                       ValidityCheckingMode::CheckingOff,
                                       AcceptAllAlgorithms, nullptr,
                                       builtChain);
      rv = BuildCertChain(trustDomain, certDER, time,
                          EndEntityOrCA::MustBeEndEntity,
                          KeyUsage::digitalSignature,
                          KeyPurposeId::id_kp_clientAuth,
                          CertPolicyId::anyPolicy, stapledOCSPResponse);
      break;
    }

    case certificateUsageSSLServer: {
      
      
      

      SignatureDigestOption digestAlgorithmOptions[] = {
        DisableSHA1Everywhere,
        DisableSHA1ForCA,
        DisableSHA1ForEE,
        AcceptAllAlgorithms
      };

      SignatureDigestStatus digestAlgorithmStatuses[] = {
        SignatureDigestStatus::GoodAlgorithmsOnly,
        SignatureDigestStatus::WeakEECert,
        SignatureDigestStatus::WeakCACert,
        SignatureDigestStatus::WeakCAAndEE
      };

      size_t digestAlgorithmOptionsCount = MOZ_ARRAY_LENGTH(digestAlgorithmStatuses);

      static_assert(MOZ_ARRAY_LENGTH(digestAlgorithmOptions) ==
                    MOZ_ARRAY_LENGTH(digestAlgorithmStatuses),
                    "digestAlgorithm array lengths differ");

      rv = Result::ERROR_UNKNOWN_ERROR;

#ifndef MOZ_NO_EV_CERTS
      
      NSSCertDBTrustDomain::OCSPFetching evOCSPFetching
        = (mOCSPDownloadConfig == ocspOff) ||
          (flags & FLAG_LOCAL_ONLY) ? NSSCertDBTrustDomain::LocalOnlyOCSPForEV
                                    : NSSCertDBTrustDomain::FetchOCSPForEV;

      CertPolicyId evPolicy;
      SECOidTag evPolicyOidTag;
      SECStatus srv = GetFirstEVPolicy(cert, evPolicy, evPolicyOidTag);
      for (size_t i=0;
           i < digestAlgorithmOptionsCount && rv != Success && srv == SECSuccess;
           i++) {
        NSSCertDBTrustDomain
          trustDomain(trustSSL, evOCSPFetching,
                      mOCSPCache, pinArg, ocspGETConfig,
                      mCertShortLifetimeInDays, mPinningMode, MIN_RSA_BITS,
                      ValidityCheckingMode::CheckForEV,
                      digestAlgorithmOptions[i], hostname, builtChain);
        rv = BuildCertChainForOneKeyUsage(trustDomain, certDER, time,
                                          KeyUsage::digitalSignature,
                                          KeyUsage::keyEncipherment, 
                                          KeyUsage::keyAgreement,    
                                          KeyPurposeId::id_kp_serverAuth,
                                          evPolicy, stapledOCSPResponse,
                                          ocspStaplingStatus);
        if (rv == Success) {
          MOZ_LOG(gCertVerifierLog, LogLevel::Debug,
                  ("cert is EV with status %i\n", digestAlgorithmStatuses[i]));
          if (evOidPolicy) {
            *evOidPolicy = evPolicyOidTag;
          }
          if (sigDigestStatus) {
            *sigDigestStatus = digestAlgorithmStatuses[i];
          }
        }
      }
      if (rv == Success) {
        break;
      }
#endif

      if (flags & FLAG_MUST_BE_EV) {
        rv = Result::ERROR_POLICY_VALIDATION_FAILED;
        break;
      }

      
      unsigned int keySizeOptions[] = {
        MIN_RSA_BITS,
        MIN_RSA_BITS_WEAK
      };

      KeySizeStatus keySizeStatuses[] = {
        KeySizeStatus::LargeMinimumSucceeded,
        KeySizeStatus::CompatibilityRisk
      };

      static_assert(MOZ_ARRAY_LENGTH(keySizeOptions) ==
                    MOZ_ARRAY_LENGTH(keySizeStatuses),
                    "keySize array lengths differ");

      size_t keySizeOptionsCount = MOZ_ARRAY_LENGTH(keySizeStatuses);

      for (size_t i=0; i<keySizeOptionsCount && rv != Success; i++) {
        for (size_t j=0; j<digestAlgorithmOptionsCount && rv != Success; j++) {
          NSSCertDBTrustDomain trustDomain(trustSSL, defaultOCSPFetching,
                                           mOCSPCache, pinArg, ocspGETConfig,
                                           mCertShortLifetimeInDays,
                                           mPinningMode, keySizeOptions[i],
                                           ValidityCheckingMode::CheckingOff,
                                           digestAlgorithmOptions[j],
                                           hostname, builtChain);
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
              *keySizeStatus = keySizeStatuses[i];
            }
            if (sigDigestStatus) {
              *sigDigestStatus = digestAlgorithmStatuses[j];
            }
          }
        }
      }

      if (rv == Success) {
        break;
      }

      if (keySizeStatus) {
        *keySizeStatus = KeySizeStatus::AlreadyBad;
      }
      if (sigDigestStatus) {
        *sigDigestStatus = SignatureDigestStatus::AlreadyBad;
      }

      break;
    }

    case certificateUsageSSLCA: {
      NSSCertDBTrustDomain trustDomain(trustSSL, defaultOCSPFetching,
                                       mOCSPCache, pinArg, ocspGETConfig,
                                       mCertShortLifetimeInDays,
                                       pinningDisabled, MIN_RSA_BITS_WEAK,
                                       ValidityCheckingMode::CheckingOff,
                                       AcceptAllAlgorithms, nullptr,
                                       builtChain);
      rv = BuildCertChain(trustDomain, certDER, time,
                          EndEntityOrCA::MustBeCA, KeyUsage::keyCertSign,
                          KeyPurposeId::id_kp_serverAuth,
                          CertPolicyId::anyPolicy, stapledOCSPResponse);
      break;
    }

    case certificateUsageEmailSigner: {
      NSSCertDBTrustDomain trustDomain(trustEmail, defaultOCSPFetching,
                                       mOCSPCache, pinArg, ocspGETConfig,
                                       mCertShortLifetimeInDays,
                                       pinningDisabled, MIN_RSA_BITS_WEAK,
                                       ValidityCheckingMode::CheckingOff,
                                       AcceptAllAlgorithms, nullptr,
                                       builtChain);
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
      
      
      
      NSSCertDBTrustDomain trustDomain(trustEmail, defaultOCSPFetching,
                                       mOCSPCache, pinArg, ocspGETConfig,
                                       mCertShortLifetimeInDays,
                                       pinningDisabled, MIN_RSA_BITS_WEAK,
                                       ValidityCheckingMode::CheckingOff,
                                       AcceptAllAlgorithms, nullptr,
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
      NSSCertDBTrustDomain trustDomain(trustObjectSigning, defaultOCSPFetching,
                                       mOCSPCache, pinArg, ocspGETConfig,
                                       mCertShortLifetimeInDays,
                                       pinningDisabled, MIN_RSA_BITS_WEAK,
                                       ValidityCheckingMode::CheckingOff,
                                       AcceptAllAlgorithms, nullptr,
                                       builtChain);
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

      NSSCertDBTrustDomain sslTrust(trustSSL, defaultOCSPFetching, mOCSPCache,
                                    pinArg, ocspGETConfig, mCertShortLifetimeInDays,
                                    pinningDisabled, MIN_RSA_BITS_WEAK,
                                    ValidityCheckingMode::CheckingOff,
                                    AcceptAllAlgorithms, nullptr, builtChain);
      rv = BuildCertChain(sslTrust, certDER, time, endEntityOrCA,
                          keyUsage, eku, CertPolicyId::anyPolicy,
                          stapledOCSPResponse);
      if (rv == Result::ERROR_UNKNOWN_ISSUER) {
        NSSCertDBTrustDomain emailTrust(trustEmail, defaultOCSPFetching,
                                        mOCSPCache, pinArg, ocspGETConfig,
                                        mCertShortLifetimeInDays,
                                        pinningDisabled, MIN_RSA_BITS_WEAK,
                                        ValidityCheckingMode::CheckingOff,
                                        AcceptAllAlgorithms, nullptr,
                                        builtChain);
        rv = BuildCertChain(emailTrust, certDER, time, endEntityOrCA,
                            keyUsage, eku, CertPolicyId::anyPolicy,
                            stapledOCSPResponse);
        if (rv == Result::ERROR_UNKNOWN_ISSUER) {
          NSSCertDBTrustDomain objectSigningTrust(trustObjectSigning,
                                                  defaultOCSPFetching, mOCSPCache,
                                                  pinArg, ocspGETConfig,
                                                  mCertShortLifetimeInDays,
                                                  pinningDisabled,
                                                  MIN_RSA_BITS_WEAK,
                                                  ValidityCheckingMode::CheckingOff,
                                                  AcceptAllAlgorithms,
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
                  KeySizeStatus* keySizeStatus,
                  SignatureDigestStatus* sigDigestStatus)
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
                            keySizeStatus, sigDigestStatus);
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
    
    if (result == Result::ERROR_BAD_DER) {
      PR_SetError(SSL_ERROR_BAD_CERT_DOMAIN, 0);
    } else {
      PR_SetError(MapResultToPRErrorCode(result), 0);
    }
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
