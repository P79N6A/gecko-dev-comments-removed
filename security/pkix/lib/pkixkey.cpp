























#include <limits>
#include <stdint.h>

#include "cert.h"
#include "cryptohi.h"
#include "keyhi.h"
#include "pkix/pkix.h"
#include "prerror.h"
#include "secerr.h"

namespace mozilla { namespace pkix {

SECStatus
VerifySignedData(const CERTSignedData& sd, const SECItem& subjectPublicKeyInfo,
                 void* pkcs11PinArg)
{
  if (!sd.data.data || !sd.signatureAlgorithm.algorithm.data ||
      !sd.signature.data) {
    PR_NOT_REACHED("invalid args to VerifySignedData");
    PR_SetError(SEC_ERROR_INVALID_ARGS, 0);
    return SECFailure;
  }

  
  if (sd.data.len > static_cast<unsigned int>(std::numeric_limits<int>::max())) {
    PR_SetError(SEC_ERROR_INVALID_ARGS, 0);
    return SECFailure;
  }

  
  SECItem sig = sd.signature;
  DER_ConvertBitString(&sig);

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

  SECOidTag hashAlg;
  if (VFY_VerifyDataWithAlgorithmID(sd.data.data, static_cast<int>(sd.data.len),
                                    pubKey.get(), &sig, &sd.signatureAlgorithm,
                                    &hashAlg, pkcs11PinArg) != SECSuccess) {
    return SECFailure;
  }

  
  
  
  uint32_t policy;
  if (NSS_GetAlgorithmPolicy(hashAlg, &policy) != SECSuccess) {
    return SECFailure;
  }

  
  
  
  static const uint32_t requiredPolicy = NSS_USE_ALG_IN_CERT_SIGNATURE |
                                         NSS_USE_ALG_IN_CMS_SIGNATURE;
  if ((policy & requiredPolicy) != requiredPolicy) {
    PR_SetError(SEC_ERROR_CERT_SIGNATURE_ALGORITHM_DISABLED, 0);
    return SECFailure;
  }

  return SECSuccess;
}

} } 
