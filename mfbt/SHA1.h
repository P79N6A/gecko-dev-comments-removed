






#ifndef mozilla_SHA1_h_
#define mozilla_SHA1_h_

#include "mozilla/StandardInteger.h"
#include "mozilla/Types.h"

#include <stddef.h>

namespace mozilla {



















class SHA1Sum
{
    union {
        uint32_t w[16]; 
        uint8_t b[64];
    } u;
    uint64_t size; 
    unsigned H[22]; 
    bool mDone;

  public:
    MFBT_API() SHA1Sum();

    static const size_t HashSize = 20;
    typedef uint8_t Hash[HashSize];

    
    MFBT_API(void) update(const void* dataIn, uint32_t len);

    
    MFBT_API(void) finish(SHA1Sum::Hash& hashOut);
};

} 

#endif 
