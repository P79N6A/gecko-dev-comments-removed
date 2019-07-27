









#ifndef WEBRTC_BASE_OPENSSLDIGEST_H_
#define WEBRTC_BASE_OPENSSLDIGEST_H_

#include <openssl/evp.h>

#include "webrtc/base/messagedigest.h"

namespace rtc {


class OpenSSLDigest : public MessageDigest {
 public:
  
  explicit OpenSSLDigest(const std::string& algorithm);
  ~OpenSSLDigest();
  
  virtual size_t Size() const;
  
  virtual void Update(const void* buf, size_t len);
  
  virtual size_t Finish(void* buf, size_t len);

  
  static bool GetDigestEVP(const std::string &algorithm,
                           const EVP_MD** md);
  
  static bool GetDigestName(const EVP_MD* md,
                            std::string* algorithm);
  
  static bool GetDigestSize(const std::string &algorithm,
                            size_t* len);

 private:
  EVP_MD_CTX ctx_;
  const EVP_MD* md_;
};

}  

#endif  
