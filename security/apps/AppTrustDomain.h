





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
                              mozilla::pkix::Input candidateCertDER,
                               mozilla::pkix::TrustLevel& trustLevel)
                              MOZ_OVERRIDE;
  virtual Result FindIssuer(mozilla::pkix::Input encodedIssuerName,
                            IssuerChecker& checker,
                            mozilla::pkix::Time time) MOZ_OVERRIDE;
  virtual Result CheckRevocation(mozilla::pkix::EndEntityOrCA endEntityOrCA,
                                 const mozilla::pkix::CertID& certID,
                                 mozilla::pkix::Time time,
                     const mozilla::pkix::Input* stapledOCSPresponse,
                     const mozilla::pkix::Input* aiaExtension) MOZ_OVERRIDE;
  virtual Result IsChainValid(const mozilla::pkix::DERArray& certChain,
                              mozilla::pkix::Time time) MOZ_OVERRIDE;
  virtual Result CheckRSAPublicKeyModulusSizeInBits(
                   mozilla::pkix::EndEntityOrCA endEntityOrCA,
                   unsigned int modulusSizeInBits) MOZ_OVERRIDE;
  virtual Result CheckECDSACurveIsAcceptable(
                   mozilla::pkix::EndEntityOrCA endEntityOrCA,
                   mozilla::pkix::NamedCurve curve) MOZ_OVERRIDE;
  virtual Result VerifySignedData(
           const mozilla::pkix::SignedDataWithSignature& signedData,
           mozilla::pkix::Input subjectPublicKeyInfo) MOZ_OVERRIDE;
  virtual Result DigestBuf(mozilla::pkix::Input item,
                            uint8_t* digestBuf,
                           size_t digestBufLen) MOZ_OVERRIDE;

private:
   ScopedCERTCertList& mCertChain;
  void* mPinArg; 
  ScopedCERTCertificate mTrustedRoot;
  unsigned int mMinimumNonECCBits;
};

} } 

#endif 
