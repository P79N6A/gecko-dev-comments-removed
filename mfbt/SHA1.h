







#ifndef mozilla_SHA1_h
#define mozilla_SHA1_h

#include "mozilla/Types.h"

#include <stddef.h>
#include <stdint.h>

namespace mozilla {



















class SHA1Sum
{
  union
  {
    uint32_t mW[16]; 
    uint8_t mB[64];
  } mU;
  uint64_t mSize; 
  unsigned mH[22]; 
  bool mDone;

public:
  MFBT_API SHA1Sum();

  static const size_t kHashSize = 20;
  typedef uint8_t Hash[kHashSize];

  
  MFBT_API void update(const void* aData, uint32_t aLength);

  
  MFBT_API void finish(SHA1Sum::Hash& aHashOut);
};

} 

#endif 
