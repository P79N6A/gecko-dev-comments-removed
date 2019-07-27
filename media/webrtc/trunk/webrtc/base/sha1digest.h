









#ifndef WEBRTC_BASE_SHA1DIGEST_H_
#define WEBRTC_BASE_SHA1DIGEST_H_

#include "webrtc/base/messagedigest.h"
#include "webrtc/base/sha1.h"

namespace rtc {


class Sha1Digest : public MessageDigest {
 public:
  enum { kSize = SHA1_DIGEST_SIZE };
  Sha1Digest() {
    SHA1Init(&ctx_);
  }
  virtual size_t Size() const {
    return kSize;
  }
  virtual void Update(const void* buf, size_t len) {
    SHA1Update(&ctx_, static_cast<const uint8*>(buf), len);
  }
  virtual size_t Finish(void* buf, size_t len) {
    if (len < kSize) {
      return 0;
    }
    SHA1Final(&ctx_, static_cast<uint8*>(buf));
    SHA1Init(&ctx_);  
    return kSize;
  }

 private:
  SHA1_CTX ctx_;
};

}  

#endif
