







#include "mozilla/HashFunctions.h"
#include "mozilla/Types.h"

#include <string.h>

namespace mozilla {

uint32_t
HashBytes(const void* aBytes, size_t aLength)
{
  uint32_t hash = 0;
  const char* b = reinterpret_cast<const char*>(aBytes);

  
  size_t i = 0;
  for (; i < aLength - (aLength % sizeof(size_t)); i += sizeof(size_t)) {
    
    size_t data;
    memcpy(&data, b + i, sizeof(size_t));

    hash = AddToHash(hash, data, sizeof(data));
  }

  
  for (; i < aLength; i++) {
    hash = AddToHash(hash, b[i]);
  }
  return hash;
}

} 
