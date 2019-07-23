



#ifndef BASE_CRYPTO_SIGNATURE_VERIFIER_H_
#define BASE_CRYPTO_SIGNATURE_VERIFIER_H_

#include "build/build_config.h"

#if defined(OS_LINUX)
#include <cryptoht.h>
#elif defined(OS_MACOSX)
#include <Security/cssm.h>
#elif defined(OS_WIN)
#include <windows.h>
#include <wincrypt.h>
#endif

#include <vector>

#include "base/basictypes.h"

namespace base {



class SignatureVerifier {
 public:
  SignatureVerifier();
  ~SignatureVerifier();

  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  bool VerifyInit(const uint8* signature_algorithm,
                  int signature_algorithm_len,
                  const uint8* signature,
                  int signature_len,
                  const uint8* public_key_info,
                  int public_key_info_len);

  
  void VerifyUpdate(const uint8* data_part, int data_part_len);

  
  
  
  bool VerifyFinal();

  
  
  
  
  
  
  
  
  

 private:
  void Reset();

  std::vector<uint8> signature_;

#if defined(OS_LINUX)
  VFYContext* vfy_context_;
#elif defined(OS_MACOSX)
  std::vector<uint8> public_key_info_;

  CSSM_CSP_HANDLE csp_handle_;

  CSSM_CC_HANDLE sig_handle_;

  CSSM_KEY public_key_;
#elif defined(OS_WIN)
  HCRYPTPROV provider_;

  HCRYPTHASH hash_object_;

  HCRYPTKEY public_key_;
#endif
};

}  

#endif  
