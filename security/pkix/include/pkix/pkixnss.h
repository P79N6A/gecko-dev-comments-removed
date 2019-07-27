























#ifndef mozilla_pkix__pkixnss_h
#define mozilla_pkix__pkixnss_h

#include "pkixtypes.h"
#include "prerror.h"
#include "seccomon.h"

namespace mozilla { namespace pkix {


Result VerifySignedData(const SignedDataWithSignature& sd,
                        Input subjectPublicKeyInfo,
                        void* pkcs11PinArg);












Result DigestBuf(Input item,  uint8_t* digestBuf,
                 size_t digestBufLen);


Result CheckPublicKey(Input subjectPublicKeyInfo);

Result MapPRErrorCodeToResult(PRErrorCode errorCode);
PRErrorCode MapResultToPRErrorCode(Result result);








static const PRErrorCode ERROR_BASE = -0x4000;
static const PRErrorCode ERROR_LIMIT = ERROR_BASE + 1000;

enum ErrorCode {
  MOZILLA_PKIX_ERROR_KEY_PINNING_FAILURE = ERROR_BASE + 0,
  MOZILLA_PKIX_ERROR_CA_CERT_USED_AS_END_ENTITY = ERROR_BASE + 1,
  MOZILLA_PKIX_ERROR_INADEQUATE_KEY_SIZE = ERROR_BASE + 2
};

void RegisterErrorTable();

inline SECItem UnsafeMapInputToSECItem(Input input)
{
  SECItem result = {
    siBuffer,
    const_cast<uint8_t*>(input.UnsafeGetData()),
    input.GetLength()
  };
  return result;
}

} } 

#endif 
