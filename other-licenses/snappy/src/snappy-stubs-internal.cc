



























#include <algorithm>
#include <string>

#include "snappy-stubs-internal.h"

namespace snappy {

void Varint::Append32(string* s, uint32 value) {
  char buf[Varint::kMax32];
  const char* p = Varint::Encode32(buf, value);
  s->append(buf, p - buf);
}

}  
