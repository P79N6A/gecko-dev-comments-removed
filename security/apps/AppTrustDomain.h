





#ifndef mozilla_psm_AppsTrustDomain_h
#define mozilla_psm_AppsTrustDomain_h

#include "pkix/pkixtypes.h"
#include "nsDebug.h"
#include "nsIX509CertDB.h"
#include "ScopedNSSTypes.h"

namespace mozilla { namespace psm {

class AppTrustDomain MOZ_FINAL : public mozilla::pkix::TrustDomain
{
public:
  AppTrustDomain(ScopedCERTCertList&, void* pinArg);

  SECStatus SetTrustedRoot(AppTrustedRoot trustedRoot);

  SECStatus GetCertTrust(mozilla::pkix::EndEntityOrCA endEntityOrCA,
                         const mozilla::pkix::CertPolicyId& policy,
                         const SECItem& candidateCertDER,
                  mozilla::pkix::TrustLevel* trustLevel) MOZ_OVERRIDE;
  SECStatus FindIssuer(const SECItem& encodedIssuerName,
                       IssuerChecker& checker, PRTime time) MOZ_OVERRIDE;
  SECStatus CheckRevocation(mozilla::pkix::EndEntityOrCA endEntityOrCA,
                            const mozilla::pkix::CertID& certID, PRTime time,
                             const SECItem* stapledOCSPresponse,
                             const SECItem* aiaExtension);
  SECStatus IsChainValid(const mozilla::pkix::DERArray& certChain);

  SECStatus VerifySignedData(
              const mozilla::pkix::SignedDataWithSignature& signedData,
              const SECItem& subjectPublicKeyInfo) MOZ_OVERRIDE;
  SECStatus DigestBuf(const SECItem& item,  uint8_t* digestBuf,
                      size_t digestBufLen) MOZ_OVERRIDE;

private:
   ScopedCERTCertList& mCertChain;
  void* mPinArg; 
  ScopedCERTCertificate mTrustedRoot;
};

} } 

#endif 
