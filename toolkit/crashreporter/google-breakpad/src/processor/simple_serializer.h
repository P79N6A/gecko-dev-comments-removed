




































#ifndef PROCESSOR_SIMPLE_SERIALIZER_H__
#define PROCESSOR_SIMPLE_SERIALIZER_H__

#include "google_breakpad/common/breakpad_types.h"

namespace google_breakpad {

typedef uint64_t MemAddr;



template<class Type> class SimpleSerializer {
 public:
  
  static size_t SizeOf(const Type &item) { return sizeof(item); }
  
  
  static char *Write(const Type &item, char *dest) {
    new (dest) Type(item);
    return dest + SizeOf(item);
  }
};

}  

#endif  
