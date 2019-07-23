



#include "base/sha2.h"

#include "base/third_party/nss/blapi.h"
#include "base/third_party/nss/sha256.h"

namespace base {

void SHA256HashString(const std::string& str, void* output, size_t len) {
  SHA256Context ctx;

  SHA256_Begin(&ctx);
  SHA256_Update(&ctx, reinterpret_cast<const unsigned char*>(str.data()),
                static_cast<unsigned int>(str.length()));
  SHA256_End(&ctx, static_cast<unsigned char*>(output), NULL,
             static_cast<unsigned int>(len));
}

}  
