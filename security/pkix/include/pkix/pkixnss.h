























#ifndef mozilla_pkix_pkixnss_h
#define mozilla_pkix_pkixnss_h

#include "pkixtypes.h"
#include "prerror.h"
#include "seccomon.h"

namespace mozilla { namespace pkix {



Result VerifyRSAPKCS1SignedDigestNSS(const SignedDigest& sd,
                                     Input subjectPublicKeyInfo,
                                     void* pkcs11PinArg);



Result VerifyECDSASignedDigestNSS(const SignedDigest& sd,
                                  Input subjectPublicKeyInfo,
                                  void* pkcs11PinArg);












Result DigestBufNSS(Input item,
                    DigestAlgorithm digestAlg,
                     uint8_t* digestBuf,
                    size_t digestBufLen);

Result MapPRErrorCodeToResult(PRErrorCode errorCode);
PRErrorCode MapResultToPRErrorCode(Result result);








static const PRErrorCode ERROR_BASE = -0x4000;
static const PRErrorCode ERROR_LIMIT = ERROR_BASE + 1000;

enum ErrorCode
{
  MOZILLA_PKIX_ERROR_KEY_PINNING_FAILURE = ERROR_BASE + 0,
  MOZILLA_PKIX_ERROR_CA_CERT_USED_AS_END_ENTITY = ERROR_BASE + 1,
  MOZILLA_PKIX_ERROR_INADEQUATE_KEY_SIZE = ERROR_BASE + 2,
  MOZILLA_PKIX_ERROR_V1_CERT_USED_AS_CA = ERROR_BASE + 3,
  MOZILLA_PKIX_ERROR_NO_RFC822NAME_MATCH = ERROR_BASE + 4,
  MOZILLA_PKIX_ERROR_NOT_YET_VALID_CERTIFICATE = ERROR_BASE + 5,
  MOZILLA_PKIX_ERROR_NOT_YET_VALID_ISSUER_CERTIFICATE = ERROR_BASE + 6,
};

void RegisterErrorTable();

inline SECItem UnsafeMapInputToSECItem(Input input)
{
  SECItem result = {
    siBuffer,
    const_cast<uint8_t*>(input.UnsafeGetData()),
    input.GetLength()
  };
  static_assert(sizeof(decltype(input.GetLength())) <= sizeof(result.len),
                "input.GetLength() must fit in a SECItem");
  return result;
}

} } 

#endif 
