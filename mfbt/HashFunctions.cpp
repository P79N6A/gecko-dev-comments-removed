






#include "mozilla/HashFunctions.h"
#include "mozilla/Types.h"

#include <string.h>

namespace mozilla {

uint32_t
HashBytes(const void* bytes, size_t length)
{
  uint32_t hash = 0;
  const char* b = reinterpret_cast<const char*>(bytes);

  
  size_t i = 0;
  for (; i < length - (length % sizeof(size_t)); i += sizeof(size_t)) {
    
    size_t data;
    memcpy(&data, b + i, sizeof(size_t));

    hash = AddToHash(hash, data, sizeof(data));
  }

  
  for (; i < length; i++)
    hash = AddToHash(hash, b[i]);

  return hash;
}

} 
