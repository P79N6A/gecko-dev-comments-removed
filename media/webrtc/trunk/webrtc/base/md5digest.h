









#ifndef WEBRTC_BASE_MD5DIGEST_H_
#define WEBRTC_BASE_MD5DIGEST_H_

#include "webrtc/base/md5.h"
#include "webrtc/base/messagedigest.h"

namespace rtc {


class Md5Digest : public MessageDigest {
 public:
  enum { kSize = 16 };
  Md5Digest() {
    MD5Init(&ctx_);
  }
  virtual size_t Size() const {
    return kSize;
  }
  virtual void Update(const void* buf, size_t len) {
    MD5Update(&ctx_, static_cast<const uint8*>(buf), len);
  }
  virtual size_t Finish(void* buf, size_t len) {
    if (len < kSize) {
      return 0;
    }
    MD5Final(&ctx_, static_cast<uint8*>(buf));
    MD5Init(&ctx_);  
    return kSize;
  }
 private:
  MD5_CTX ctx_;
};

}  

#endif
