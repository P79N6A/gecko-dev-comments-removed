




































#ifndef PROCESSOR_SIMPLE_SERIALIZER_H__
#define PROCESSOR_SIMPLE_SERIALIZER_H__

#include <sys/types.h>

namespace google_breakpad {

typedef u_int64_t MemAddr;



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
