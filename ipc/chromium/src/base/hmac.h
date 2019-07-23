






#ifndef BASE_HMAC_H_
#define BASE_HMAC_H_

#include <string>

#include "base/basictypes.h"
#include "base/scoped_ptr.h"

namespace base {


struct HMACPlatformData;

class HMAC {
 public:
  
  enum HashAlgorithm {
    SHA1
  };

  explicit HMAC(HashAlgorithm hash_alg);
  ~HMAC();

  
  
  bool Init(const unsigned char* key, int key_length);

  
  
  bool Init(const std::string& key) {
    return Init(reinterpret_cast<const unsigned char*>(key.data()),
                static_cast<int>(key.size()));
  }

  
  
  
  bool Sign(const std::string& data, unsigned char* digest, int digest_length);

 private:
  HashAlgorithm hash_alg_;
  scoped_ptr<HMACPlatformData> plat_;

  DISALLOW_COPY_AND_ASSIGN(HMAC);
};

}  

#endif
