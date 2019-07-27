



#include "base/hash.h"




extern "C" uint32_t SuperFastHash(const char* data, int len);

namespace base {

uint32 SuperFastHash(const char* data, int len) {
  return ::SuperFastHash(data, len);
}

}  
