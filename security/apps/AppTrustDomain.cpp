





#include "AppTrustDomain.h"
#include "certdb.h"
#include "pkix/pkixnss.h"
#include "mozilla/ArrayUtils.h"
#include "nsIX509CertDB.h"
#include "nsNSSCertificate.h"
#include "prerror.h"
#include "secerr.h"


#include "marketplace-prod-public.inc"
#include "marketplace-prod-reviewers.inc"
#include "marketplace-dev-public.inc"
#include "marketplace-dev-reviewers.inc"
#include "marketplace-stage.inc"
#include "xpcshell.inc"

#include "manifest-signing-root.inc"
#include "manifest-signing-test-root.inc"

using namespace mozilla::pkix;

#ifdef PR_LOGGING
extern PRLogModuleInfo* gPIPNSSLog;
#endif

static const unsigned int DEFAULT_MIN_RSA_BITS = 2048;

namespace mozilla { namespace psm {

AppTrustDomain::AppTrustDomain(ScopedCERTCertList& certChain, void* pinArg)
  : mCertChain(certChain)
  , mPinArg(pinArg)
  , mMinRSABits(DEFAULT_MIN_RSA_BITS)
{
}

SECStatus
AppTrustDomain::SetTrustedRoot(AppTrustedRoot trustedRoot)
{
  SECItem trustedDER;

  
  

  switch (trustedRoot)
  {
    case nsIX509CertDB::AppMarketplaceProdPublicRoot:
      trustedDER.data = const_cast<uint8_t*>(marketplaceProdPublicRoot);
      trustedDER.len = mozilla::ArrayLength(marketplaceProdPublicRoot);
      break;

    case nsIX509CertDB::AppMarketplaceProdReviewersRoot:
      trustedDER.data = const_cast<uint8_t*>(marketplaceProdReviewersRoot);
      trustedDER.len = mozilla::ArrayLength(marketplaceProdReviewersRoot);
      break;

    case nsIX509CertDB::AppMarketplaceDevPublicRoot:
      trustedDER.data = const_cast<uint8_t*>(marketplaceDevPublicRoot);
      trustedDER.len = mozilla::ArrayLength(marketplaceDevPublicRoot);
      break;

    case nsIX509CertDB::AppMarketplaceDevReviewersRoot:
      trustedDER.data = const_cast<uint8_t*>(marketplaceDevReviewersRoot);
      trustedDER.len = mozilla::ArrayLength(marketplaceDevReviewersRoot);
      break;

    case nsIX509CertDB::AppMarketplaceStageRoot:
      trustedDER.data = const_cast<uint8_t*>(marketplaceStageRoot);
      trustedDER.len = mozilla::ArrayLength(marketplaceStageRoot);
      
      mMinRSABits = 1024u;
      break;

    case nsIX509CertDB::AppXPCShellRoot:
      trustedDER.data = const_cast<uint8_t*>(xpcshellRoot);
      trustedDER.len = mozilla::ArrayLength(xpcshellRoot);
      break;

    case nsIX509CertDB::TrustedHostedAppPublicRoot:
      trustedDER.data = const_cast<uint8_t*>(trustedAppPublicRoot);
      trustedDER.len = mozilla::ArrayLength(trustedAppPublicRoot);
      break;

    case nsIX509CertDB::TrustedHostedAppTestRoot:
      trustedDER.data = const_cast<uint8_t*>(trustedAppTestRoot);
      trustedDER.len = mozilla::ArrayLength(trustedAppTestRoot);
      break;

    default:
      PR_SetError(SEC_ERROR_INVALID_ARGS, 0);
      return SECFailure;
  }

  mTrustedRoot = CERT_NewTempCertificate(CERT_GetDefaultCertDB(),
                                         &trustedDER, nullptr, false, true);
  if (!mTrustedRoot) {
    return SECFailure;
  }

  return SECSuccess;
}

Result
AppTrustDomain::FindIssuer(Input encodedIssuerName, IssuerChecker& checker,
                           Time)

{
  MOZ_ASSERT(mTrustedRoot);
  if (!mTrustedRoot) {
    return Result::FATAL_ERROR_INVALID_STATE;
  }

  
  
  
  
  
  
  
  
  SECItem encodedIssuerNameSECItem =
    UnsafeMapInputToSECItem(encodedIssuerName);
  ScopedCERTCertList
    candidates(CERT_CreateSubjectCertList(nullptr, CERT_GetDefaultCertDB(),
                                          &encodedIssuerNameSECItem, 0,
                                          false));
  if (candidates) {
    for (CERTCertListNode* n = CERT_LIST_HEAD(candidates);
         !CERT_LIST_END(n, candidates); n = CERT_LIST_NEXT(n)) {
      Input certDER;
      Result rv = certDER.Init(n->cert->derCert.data, n->cert->derCert.len);
      if (rv != Success) {
        continue; 
      }

      bool keepGoing;
      rv = checker.Check(certDER, nullptr,
                         keepGoing);
      if (rv != Success) {
        return rv;
      }
      if (!keepGoing) {
        break;
      }
    }
  }

  return Success;
}

Result
AppTrustDomain::GetCertTrust(EndEntityOrCA endEntityOrCA,
                             const CertPolicyId& policy,
                             Input candidateCertDER,
                      TrustLevel& trustLevel)
{
  MOZ_ASSERT(policy.IsAnyPolicy());
  MOZ_ASSERT(mTrustedRoot);
  if (!policy.IsAnyPolicy()) {
    return Result::FATAL_ERROR_INVALID_ARGS;
  }
  if (!mTrustedRoot) {
    return Result::FATAL_ERROR_INVALID_STATE;
  }

  

  
  
  
  SECItem candidateCertDERSECItem =
    UnsafeMapInputToSECItem(candidateCertDER);
  ScopedCERTCertificate candidateCert(
    CERT_NewTempCertificate(CERT_GetDefaultCertDB(), &candidateCertDERSECItem,
                            nullptr, false, true));
  if (!candidateCert) {
    return MapPRErrorCodeToResult(PR_GetError());
  }

  CERTCertTrust trust;
  if (CERT_GetCertTrust(candidateCert.get(), &trust) == SECSuccess) {
    uint32_t flags = SEC_GET_TRUST_FLAGS(&trust, trustObjectSigning);

    
    
    
    
    
    uint32_t relevantTrustBit = endEntityOrCA == EndEntityOrCA::MustBeCA
                              ? CERTDB_TRUSTED_CA
                              : CERTDB_TRUSTED;
    if (((flags & (relevantTrustBit | CERTDB_TERMINAL_RECORD)))
            == CERTDB_TERMINAL_RECORD) {
      trustLevel = TrustLevel::ActivelyDistrusted;
      return Success;
    }
  }

  
  if (CERT_CompareCerts(mTrustedRoot.get(), candidateCert.get())) {
    trustLevel = TrustLevel::TrustAnchor;
    return Success;
  }

  trustLevel = TrustLevel::InheritsTrust;
  return Success;
}

Result
AppTrustDomain::DigestBuf(Input item,
                          DigestAlgorithm digestAlg,
                           uint8_t* digestBuf,
                          size_t digestBufLen)
{
  return DigestBufNSS(item, digestAlg, digestBuf, digestBufLen);
}

Result
AppTrustDomain::CheckRevocation(EndEntityOrCA, const CertID&, Time,
                                 const Input*,
                                 const Input*)
{
  
  
  return Success;
}

Result
AppTrustDomain::IsChainValid(const DERArray& certChain, Time time)
{
  SECStatus srv = ConstructCERTCertListFromReversedDERArray(certChain,
                                                            mCertChain);
  if (srv != SECSuccess) {
    return MapPRErrorCodeToResult(PR_GetError());
  }
  return Success;
}

Result
AppTrustDomain::CheckSignatureDigestAlgorithm(DigestAlgorithm)
{
  
  return Success;
}

Result
AppTrustDomain::CheckRSAPublicKeyModulusSizeInBits(
  EndEntityOrCA , unsigned int modulusSizeInBits)
{
  if (modulusSizeInBits < mMinRSABits) {
    return Result::ERROR_INADEQUATE_KEY_SIZE;
  }
  return Success;
}

Result
AppTrustDomain::VerifyRSAPKCS1SignedDigest(const SignedDigest& signedDigest,
                                           Input subjectPublicKeyInfo)
{
  
  return VerifyRSAPKCS1SignedDigestNSS(signedDigest, subjectPublicKeyInfo,
                                       mPinArg);
}

Result
AppTrustDomain::CheckECDSACurveIsAcceptable(EndEntityOrCA ,
                                            NamedCurve curve)
{
  switch (curve) {
    case NamedCurve::secp256r1: 
    case NamedCurve::secp384r1: 
    case NamedCurve::secp521r1:
      return Success;
  }

  return Result::ERROR_UNSUPPORTED_ELLIPTIC_CURVE;
}

Result
AppTrustDomain::VerifyECDSASignedDigest(const SignedDigest& signedDigest,
                                        Input subjectPublicKeyInfo)
{
  return VerifyECDSASignedDigestNSS(signedDigest, subjectPublicKeyInfo,
                                    mPinArg);
}

} } 
