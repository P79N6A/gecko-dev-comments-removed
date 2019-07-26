





#ifndef mozilla_psm_AppsTrustDomain_h
#define mozilla_psm_AppsTrustDomain_h

#include "insanity/pkixtypes.h"
#include "nsDebug.h"
#include "nsIX509CertDB.h"

namespace mozilla { namespace psm {

class AppTrustDomain MOZ_FINAL : public insanity::pkix::TrustDomain
{
public:
  AppTrustDomain(void* pinArg);

  SECStatus SetTrustedRoot(AppTrustedRoot trustedRoot);

  SECStatus GetCertTrust(insanity::pkix::EndEntityOrCA endEntityOrCA,
                         const CERTCertificate* candidateCert,
                  TrustLevel* trustLevel) MOZ_OVERRIDE;
  SECStatus FindPotentialIssuers(const SECItem* encodedIssuerName,
                                 PRTime time,
                          insanity::pkix::ScopedCERTCertList& results)
                                 MOZ_OVERRIDE;
  SECStatus VerifySignedData(const CERTSignedData* signedData,
                             const CERTCertificate* cert) MOZ_OVERRIDE;
  SECStatus CheckRevocation(insanity::pkix::EndEntityOrCA endEntityOrCA,
                            const CERTCertificate* cert,
                             CERTCertificate* issuerCertToDup,
                            PRTime time,
                             const SECItem* stapledOCSPresponse);
private:
  void* mPinArg; 
  insanity::pkix::ScopedCERTCertificate mTrustedRoot;
};

} } 

#endif 
