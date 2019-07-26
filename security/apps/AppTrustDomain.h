





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
  SECStatus FindPotentialIssuers(const SECItem* encodedIssuerName,
                                 PRTime time,
                          mozilla::pkix::ScopedCERTCertList& results)
                                 MOZ_OVERRIDE;
  SECStatus VerifySignedData(const CERTSignedData* signedData,
                             const CERTCertificate* cert) MOZ_OVERRIDE;
  SECStatus CheckRevocation(mozilla::pkix::EndEntityOrCA endEntityOrCA,
                            const CERTCertificate* cert,
                             CERTCertificate* issuerCertToDup,
                            PRTime time,
                             const SECItem* stapledOCSPresponse);
  SECStatus IsChainValid(const CERTCertList* certChain) { return SECSuccess; }

private:
  void* mPinArg; 
  mozilla::pkix::ScopedCERTCertificate mTrustedRoot;
};

} } 

#endif 
