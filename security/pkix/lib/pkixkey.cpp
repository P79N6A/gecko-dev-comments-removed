























#include <limits>
#include <stdint.h>

#include "cert.h"
#include "cryptohi.h"
#include "keyhi.h"
#include "pkix/pkix.h"
#include "pkix/ScopedPtr.h"
#include "prerror.h"
#include "secerr.h"

namespace mozilla { namespace pkix {

SECStatus
VerifySignedData(const SignedDataWithSignature& sd,
                 const SECItem& subjectPublicKeyInfo, void* pkcs11PinArg)
{
  if (!sd.data.data || !sd.signature.data) {
    PR_NOT_REACHED("invalid args to VerifySignedData");
    PR_SetError(SEC_ERROR_INVALID_ARGS, 0);
    return SECFailure;
  }

  
  if (sd.data.len > static_cast<unsigned int>(std::numeric_limits<int>::max())) {
    PR_SetError(SEC_ERROR_INVALID_ARGS, 0);
    return SECFailure;
  }

  SECOidTag pubKeyAlg;
  SECOidTag digestAlg;
  switch (sd.algorithm) {
    case SignatureAlgorithm::ecdsa_with_sha512:
      pubKeyAlg = SEC_OID_ANSIX962_EC_PUBLIC_KEY;
      digestAlg = SEC_OID_SHA512;
      break;
    case SignatureAlgorithm::ecdsa_with_sha384:
      pubKeyAlg = SEC_OID_ANSIX962_EC_PUBLIC_KEY;
      digestAlg = SEC_OID_SHA384;
      break;
    case SignatureAlgorithm::ecdsa_with_sha256:
      pubKeyAlg = SEC_OID_ANSIX962_EC_PUBLIC_KEY;
      digestAlg = SEC_OID_SHA256;
      break;
    case SignatureAlgorithm::ecdsa_with_sha1:
      pubKeyAlg = SEC_OID_ANSIX962_EC_PUBLIC_KEY;
      digestAlg = SEC_OID_SHA1;
      break;
    case SignatureAlgorithm::rsa_pkcs1_with_sha512:
      pubKeyAlg = SEC_OID_PKCS1_RSA_ENCRYPTION;
      digestAlg = SEC_OID_SHA512;
      break;
    case SignatureAlgorithm::rsa_pkcs1_with_sha384:
      pubKeyAlg = SEC_OID_PKCS1_RSA_ENCRYPTION;
      digestAlg = SEC_OID_SHA384;
      break;
    case SignatureAlgorithm::rsa_pkcs1_with_sha256:
      pubKeyAlg = SEC_OID_PKCS1_RSA_ENCRYPTION;
      digestAlg = SEC_OID_SHA256;
      break;
    case SignatureAlgorithm::rsa_pkcs1_with_sha1:
      pubKeyAlg = SEC_OID_PKCS1_RSA_ENCRYPTION;
      digestAlg = SEC_OID_SHA1;
      break;
    case SignatureAlgorithm::dsa_with_sha256:
      pubKeyAlg = SEC_OID_ANSIX9_DSA_SIGNATURE;
      digestAlg = SEC_OID_SHA256;
      break;
    case SignatureAlgorithm::dsa_with_sha1:
      pubKeyAlg = SEC_OID_ANSIX9_DSA_SIGNATURE;
      digestAlg = SEC_OID_SHA1;
      break;
    default:
      PR_NOT_REACHED("unknown signature algorithm");
      PR_SetError(SEC_ERROR_CERT_SIGNATURE_ALGORITHM_DISABLED, 0);
      return SECFailure;
  }

  ScopedPtr<CERTSubjectPublicKeyInfo, SECKEY_DestroySubjectPublicKeyInfo>
    spki(SECKEY_DecodeDERSubjectPublicKeyInfo(&subjectPublicKeyInfo));
  if (!spki) {
    return SECFailure;
  }
  ScopedPtr<SECKEYPublicKey, SECKEY_DestroyPublicKey>
    pubKey(SECKEY_ExtractPublicKey(spki.get()));
  if (!pubKey) {
    return SECFailure;
  }

  
  
  return VFY_VerifyDataDirect(sd.data.data, static_cast<int>(sd.data.len),
                              pubKey.get(), &sd.signature, pubKeyAlg,
                              digestAlg, nullptr, pkcs11PinArg);
}

} } 
