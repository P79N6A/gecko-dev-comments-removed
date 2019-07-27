





#ifndef mozilla_psm_AppsTrustDomain_h
#define mozilla_psm_AppsTrustDomain_h

#include "pkix/pkixtypes.h"
#include "nsDebug.h"
#include "nsIX509CertDB.h"

namespace mozilla { namespace psm {

class AppTrustDomain MOZ_FINAL : public mozilla::pkix::TrustDomain
{
public:
  AppTrustDomain(void* pinArg);

  SECStatus SetTrustedRoot(AppTrustedRoot trustedRoot);

  SECStatus GetCertTrust(mozilla::pkix::EndEntityOrCA endEntityOrCA,
                         const mozilla::pkix::CertPolicyId& policy,
                         const SECItem& candidateCertDER,
                  mozilla::pkix::TrustLevel* trustLevel) MOZ_OVERRIDE;
  SECStatus FindIssuer(const SECItem& encodedIssuerName,
                       IssuerChecker& checker, PRTime time) MOZ_OVERRIDE;
  SECStatus VerifySignedData(const CERTSignedData& signedData,
                             const SECItem& subjectPublicKeyInfo) MOZ_OVERRIDE;
  SECStatus CheckRevocation(mozilla::pkix::EndEntityOrCA endEntityOrCA,
                            const mozilla::pkix::CertID& certID, PRTime time,
                             const SECItem* stapledOCSPresponse,
                             const SECItem* aiaExtension);
  SECStatus IsChainValid(const CERTCertList* certChain) { return SECSuccess; }

private:
  void* mPinArg; 
  mozilla::pkix::ScopedCERTCertificate mTrustedRoot;
};

} } 

#endif 
