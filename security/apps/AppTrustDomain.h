





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
  typedef mozilla::pkix::Result Result;

  AppTrustDomain(ScopedCERTCertList&, void* pinArg);

  SECStatus SetTrustedRoot(AppTrustedRoot trustedRoot);

  virtual Result GetCertTrust(mozilla::pkix::EndEntityOrCA endEntityOrCA,
                              const mozilla::pkix::CertPolicyId& policy,
                              const SECItem& candidateCertDER,
                               mozilla::pkix::TrustLevel* trustLevel)
                              MOZ_OVERRIDE;
  virtual Result FindIssuer(const SECItem& encodedIssuerName,
                            IssuerChecker& checker,
                            PRTime time) MOZ_OVERRIDE;
  virtual Result CheckRevocation(mozilla::pkix::EndEntityOrCA endEntityOrCA,
                                 const mozilla::pkix::CertID& certID, PRTime time,
                                  const SECItem* stapledOCSPresponse,
                                  const SECItem* aiaExtension);
  virtual Result IsChainValid(const mozilla::pkix::DERArray& certChain)
                              MOZ_OVERRIDE;
  virtual Result CheckPublicKey(const SECItem& subjectPublicKeyInfo)
                                MOZ_OVERRIDE;
  virtual Result VerifySignedData(
           const mozilla::pkix::SignedDataWithSignature& signedData,
           const SECItem& subjectPublicKeyInfo) MOZ_OVERRIDE;
  virtual Result DigestBuf(const SECItem& item,  uint8_t* digestBuf,
                           size_t digestBufLen) MOZ_OVERRIDE;

private:
   ScopedCERTCertList& mCertChain;
  void* mPinArg; 
  ScopedCERTCertificate mTrustedRoot;
};

} } 

#endif 
