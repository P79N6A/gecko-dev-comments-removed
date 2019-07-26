







#ifndef mozilla_Compression_h_
#define mozilla_Compression_h_

#include "mozilla/Types.h"
#include "mozilla/Assertions.h"

namespace mozilla {
namespace Compression {











class LZ4
{
public:
  








  static MFBT_API size_t
  compress(const char* aSource, size_t aInputSize, char* aDest);

  














  static MFBT_API size_t
  compressLimitedOutput(const char* aSource, size_t aInputSize, char* aDest,
                        size_t aMaxOutputSize);

  













  static MFBT_API bool
  decompress(const char* aSource, char* aDest, size_t aOutputSize);

  















  static MFBT_API bool
  decompress(const char* aSource, size_t aInputSize, char* aDest,
             size_t aMaxOutputSize, size_t *aOutputSize);

  








  static inline size_t maxCompressedSize(size_t aInputSize)
  {
    size_t max = (aInputSize + (aInputSize / 255) + 16);
    MOZ_ASSERT(max > aInputSize);
    return max;
  }
};

} 
} 

#endif 
