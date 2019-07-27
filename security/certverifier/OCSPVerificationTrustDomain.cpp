





#include "OCSPVerificationTrustDomain.h"

using namespace mozilla;
using namespace mozilla::pkix;

OCSPVerificationTrustDomain::OCSPVerificationTrustDomain(
  NSSCertDBTrustDomain& certDBTrustDomain)
  : mCertDBTrustDomain(certDBTrustDomain)
{
}

Result
OCSPVerificationTrustDomain::GetCertTrust(EndEntityOrCA endEntityOrCA,
                                          const CertPolicyId& policy,
                                          Input candidateCertDER,
                                   TrustLevel& trustLevel)
{
  return mCertDBTrustDomain.GetCertTrust(endEntityOrCA, policy,
                                         candidateCertDER, trustLevel);
}


Result
OCSPVerificationTrustDomain::FindIssuer(Input, IssuerChecker&, Time)
{
  
  return Result::FATAL_ERROR_LIBRARY_FAILURE;
}

Result
OCSPVerificationTrustDomain::IsChainValid(const DERArray&, Time)
{
  
  return Result::FATAL_ERROR_LIBRARY_FAILURE;
}

Result
OCSPVerificationTrustDomain::CheckRevocation(EndEntityOrCA, const CertID&,
                                             Time, Duration, const Input*,
                                             const Input*)
{
  
  return Result::FATAL_ERROR_LIBRARY_FAILURE;
}

Result
OCSPVerificationTrustDomain::CheckSignatureDigestAlgorithm(
  DigestAlgorithm aAlg, EndEntityOrCA aEEOrCA)
{
  
  
  
  
  return Success;
}

Result
OCSPVerificationTrustDomain::CheckRSAPublicKeyModulusSizeInBits(
  EndEntityOrCA aEEOrCA, unsigned int aModulusSizeInBits)
{
  return mCertDBTrustDomain.
      CheckRSAPublicKeyModulusSizeInBits(aEEOrCA, aModulusSizeInBits);
}

Result
OCSPVerificationTrustDomain::VerifyRSAPKCS1SignedDigest(
  const SignedDigest& aSignedDigest, Input aSubjectPublicKeyInfo)
{
  return mCertDBTrustDomain.VerifyRSAPKCS1SignedDigest(aSignedDigest,
                                                       aSubjectPublicKeyInfo);
}

Result
OCSPVerificationTrustDomain::CheckECDSACurveIsAcceptable(
  EndEntityOrCA aEEOrCA, NamedCurve aCurve)
{
  return mCertDBTrustDomain.CheckECDSACurveIsAcceptable(aEEOrCA, aCurve);
}

Result
OCSPVerificationTrustDomain::VerifyECDSASignedDigest(
  const SignedDigest& aSignedDigest, Input aSubjectPublicKeyInfo)
{
  return mCertDBTrustDomain.VerifyECDSASignedDigest(aSignedDigest,
                                                    aSubjectPublicKeyInfo);
}

Result
OCSPVerificationTrustDomain::CheckValidityIsAcceptable(
  Time notBefore, Time notAfter, EndEntityOrCA endEntityOrCA,
  KeyPurposeId keyPurpose)
{
  return mCertDBTrustDomain.CheckValidityIsAcceptable(notBefore, notAfter,
                                                      endEntityOrCA,
                                                      keyPurpose);
}

Result
OCSPVerificationTrustDomain::DigestBuf(
  Input item, DigestAlgorithm digestAlg,
   uint8_t* digestBuf, size_t digestBufLen)
{
  return mCertDBTrustDomain.DigestBuf(item, digestAlg, digestBuf, digestBufLen);
}
