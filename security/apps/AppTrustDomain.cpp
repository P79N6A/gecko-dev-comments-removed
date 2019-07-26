





#ifdef MOZ_LOGGING
#define FORCE_PR_LOG 1
#endif

#include "AppTrustDomain.h"
#include "certdb.h"
#include "insanity/pkix.h"
#include "mozilla/ArrayUtils.h"
#include "nsIX509CertDB.h"
#include "prerror.h"
#include "secerr.h"


#include "marketplace-prod-public.inc"
#include "marketplace-prod-reviewers.inc"
#include "marketplace-dev-public.inc"
#include "marketplace-dev-reviewers.inc"
#include "xpcshell.inc"

using namespace insanity::pkix;

#ifdef PR_LOGGING
extern PRLogModuleInfo* gPIPNSSLog;
#endif

namespace mozilla { namespace psm {

AppTrustDomain::AppTrustDomain(void* pinArg)
  : mPinArg(pinArg)
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

    case nsIX509CertDB::AppXPCShellRoot:
      trustedDER.data = const_cast<uint8_t*>(xpcshellRoot);
      trustedDER.len = mozilla::ArrayLength(xpcshellRoot);
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

SECStatus
AppTrustDomain::FindPotentialIssuers(const SECItem* encodedIssuerName,
                                     PRTime time,
                              insanity::pkix::ScopedCERTCertList& results)
{
  MOZ_ASSERT(mTrustedRoot);
  if (!mTrustedRoot) {
    PR_SetError(PR_INVALID_STATE_ERROR, 0);
    return SECFailure;
  }

  results = CERT_CreateSubjectCertList(nullptr, CERT_GetDefaultCertDB(),
                                       encodedIssuerName, time, true);
  if (!results) {
    
    
    if (PR_GetError() == SEC_ERROR_BAD_DATABASE) {
      PR_SetError(SEC_ERROR_UNKNOWN_ISSUER, 0);
    }
    return SECFailure;
  }

  return SECSuccess;
}

SECStatus
AppTrustDomain::GetCertTrust(EndEntityOrCA endEntityOrCA,
                             SECOidTag policy,
                             const CERTCertificate* candidateCert,
                      TrustLevel* trustLevel)
{
  MOZ_ASSERT(policy == SEC_OID_X509_ANY_POLICY);
  MOZ_ASSERT(candidateCert);
  MOZ_ASSERT(trustLevel);
  MOZ_ASSERT(mTrustedRoot);
  if (!candidateCert || !trustLevel || policy != SEC_OID_X509_ANY_POLICY) {
    PR_SetError(SEC_ERROR_INVALID_ARGS, 0);
    return SECFailure;
  }
  if (!mTrustedRoot) {
    PR_SetError(PR_INVALID_STATE_ERROR, 0);
    return SECFailure;
  }

  
  CERTCertTrust trust;
  if (CERT_GetCertTrust(candidateCert, &trust) == SECSuccess) {
    PRUint32 flags = SEC_GET_TRUST_FLAGS(&trust, trustObjectSigning);

    
    
    
    
    
    PRUint32 relevantTrustBit = endEntityOrCA == MustBeCA
                              ? CERTDB_TRUSTED_CA
                              : CERTDB_TRUSTED;
    if (((flags & (relevantTrustBit | CERTDB_TERMINAL_RECORD)))
            == CERTDB_TERMINAL_RECORD) {
      *trustLevel = ActivelyDistrusted;
      return SECSuccess;
    }

#ifdef MOZ_B2G_CERTDATA
    
    
    

    
    
    
    if (flags & CERTDB_TRUSTED_CA) {
      *trustLevel = TrustAnchor;
      return SECSuccess;
    }
#endif
  }

  
  if (CERT_CompareCerts(mTrustedRoot.get(), candidateCert)) {
    *trustLevel = TrustAnchor;
    return SECSuccess;
  }

  *trustLevel = InheritsTrust;
  return SECSuccess;
}

SECStatus
AppTrustDomain::VerifySignedData(const CERTSignedData* signedData,
                                  const CERTCertificate* cert)
{
  return ::insanity::pkix::VerifySignedData(signedData, cert, mPinArg);
}

SECStatus
AppTrustDomain::CheckRevocation(EndEntityOrCA,
                                const CERTCertificate*,
                                 CERTCertificate*,
                                PRTime time,
                                 const SECItem*)
{
  
  
  return SECSuccess;
}

} }
