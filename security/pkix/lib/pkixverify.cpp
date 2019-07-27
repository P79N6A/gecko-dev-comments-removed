























#include "pkixutil.h"

namespace mozilla { namespace pkix {

Result
DigestSignedData(TrustDomain& trustDomain,
                 const der::SignedDataWithSignature& signedData,
                  uint8_t(&digestBuf)[MAX_DIGEST_SIZE_IN_BYTES],
                  der::PublicKeyAlgorithm& publicKeyAlg,
                  SignedDigest& signedDigest)
{
  Reader signatureAlg(signedData.algorithm);
  Result rv = der::SignatureAlgorithmIdentifierValue(
                signatureAlg, publicKeyAlg, signedDigest.digestAlgorithm);
  if (rv != Success) {
    return rv;
  }
  if (!signatureAlg.AtEnd()) {
    return Result::ERROR_BAD_DER;
  }

  size_t digestLen;
  switch (signedDigest.digestAlgorithm) {
    case DigestAlgorithm::sha512: digestLen = 512 / 8; break;
    case DigestAlgorithm::sha384: digestLen = 384 / 8; break;
    case DigestAlgorithm::sha256: digestLen = 256 / 8; break;
    case DigestAlgorithm::sha1: digestLen = 160 / 8; break;
    MOZILLA_PKIX_UNREACHABLE_DEFAULT_ENUM
  }
  assert(digestLen <= sizeof(digestBuf));

  rv = trustDomain.DigestBuf(signedData.data, signedDigest.digestAlgorithm,
                             digestBuf, digestLen);
  if (rv != Success) {
    return rv;
  }
  rv = signedDigest.digest.Init(digestBuf, digestLen);
  if (rv != Success) {
    return rv;
  }

  return signedDigest.signature.Init(signedData.signature);
}

Result
VerifySignedDigest(TrustDomain& trustDomain,
                   der::PublicKeyAlgorithm publicKeyAlg,
                   const SignedDigest& signedDigest,
                   Input signerSubjectPublicKeyInfo)
{
  switch (publicKeyAlg) {
    case der::PublicKeyAlgorithm::ECDSA:
      return trustDomain.VerifyECDSASignedDigest(signedDigest,
                                                 signerSubjectPublicKeyInfo);
    case der::PublicKeyAlgorithm::RSA_PKCS1:
      return trustDomain.VerifyRSAPKCS1SignedDigest(signedDigest,
                                                    signerSubjectPublicKeyInfo);
    MOZILLA_PKIX_UNREACHABLE_DEFAULT_ENUM
  }
}

Result
VerifySignedData(TrustDomain& trustDomain,
                 const der::SignedDataWithSignature& signedData,
                 Input signerSubjectPublicKeyInfo)
{
  uint8_t digestBuf[MAX_DIGEST_SIZE_IN_BYTES];
  der::PublicKeyAlgorithm publicKeyAlg;
  SignedDigest signedDigest;
  Result rv = DigestSignedData(trustDomain, signedData, digestBuf,
                               publicKeyAlg, signedDigest);
  if (rv != Success) {
    return rv;
  }
  return VerifySignedDigest(trustDomain, publicKeyAlg, signedDigest,
                            signerSubjectPublicKeyInfo);
}

} } 
