





#ifndef mozilla_psm__OCSPVerificationTrustDomain_h
#define mozilla_psm__OCSPVerificationTrustDomain_h

#include "pkix/pkixtypes.h"
#include "NSSCertDBTrustDomain.h"

namespace mozilla { namespace psm {

class OCSPVerificationTrustDomain : public mozilla::pkix::TrustDomain
{
public:
  OCSPVerificationTrustDomain(NSSCertDBTrustDomain& certDBTrustDomain);

  virtual Result FindIssuer(mozilla::pkix::Input encodedIssuerName,
                            IssuerChecker& checker,
                            mozilla::pkix::Time time) override;

  virtual Result GetCertTrust(mozilla::pkix::EndEntityOrCA endEntityOrCA,
                              const mozilla::pkix::CertPolicyId& policy,
                              mozilla::pkix::Input candidateCertDER,
                       mozilla::pkix::TrustLevel& trustLevel)
                              override;

  virtual Result CheckSignatureDigestAlgorithm(
                   mozilla::pkix::DigestAlgorithm digestAlg,
                   mozilla::pkix::EndEntityOrCA endEntityOrCA) override;

  virtual Result CheckRSAPublicKeyModulusSizeInBits(
                   mozilla::pkix::EndEntityOrCA endEntityOrCA,
                   unsigned int modulusSizeInBits) override;

  virtual Result VerifyRSAPKCS1SignedDigest(
                   const mozilla::pkix::SignedDigest& signedDigest,
                   mozilla::pkix::Input subjectPublicKeyInfo) override;

  virtual Result CheckECDSACurveIsAcceptable(
                   mozilla::pkix::EndEntityOrCA endEntityOrCA,
                   mozilla::pkix::NamedCurve curve) override;

  virtual Result VerifyECDSASignedDigest(
                   const mozilla::pkix::SignedDigest& signedDigest,
                   mozilla::pkix::Input subjectPublicKeyInfo) override;

  virtual Result DigestBuf(mozilla::pkix::Input item,
                           mozilla::pkix::DigestAlgorithm digestAlg,
                    uint8_t* digestBuf,
                           size_t digestBufLen) override;

  virtual Result CheckValidityIsAcceptable(
                   mozilla::pkix::Time notBefore, mozilla::pkix::Time notAfter,
                   mozilla::pkix::EndEntityOrCA endEntityOrCA,
                   mozilla::pkix::KeyPurposeId keyPurpose) override;

  virtual Result CheckRevocation(
                   mozilla::pkix::EndEntityOrCA endEntityOrCA,
                   const mozilla::pkix::CertID& certID,
                   mozilla::pkix::Time time,
                   mozilla::pkix::Duration validityDuration,
       const mozilla::pkix::Input* stapledOCSPResponse,
       const mozilla::pkix::Input* aiaExtension)
                   override;

  virtual Result IsChainValid(const mozilla::pkix::DERArray& certChain,
                              mozilla::pkix::Time time) override;

private:
  NSSCertDBTrustDomain& mCertDBTrustDomain;
};


} } 

#endif 
