



#ifndef BASE_HASH_H_
#define BASE_HASH_H_

#include <limits>
#include <string>

#include "base/base_export.h"
#include "base/basictypes.h"
#include "base/logging.h"

namespace base {


BASE_EXPORT uint32 SuperFastHash(const char* data, int len);



inline uint32 Hash(const char* data, size_t length) {
  if (length > static_cast<size_t>(std::numeric_limits<int>::max())) {
    NOTREACHED();
    return 0;
  }
  return SuperFastHash(data, static_cast<int>(length));
}



inline uint32 Hash(const std::string& str) {
  return Hash(str.data(), str.size());
}

}  

#endif  
