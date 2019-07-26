






#ifndef mozilla_Compression_h_
#define mozilla_Compression_h_

#include "mozilla/Types.h"
#include "mozilla/Assertions.h"

namespace mozilla {
namespace Compression {












class LZ4
{

public:

  








  static MFBT_API size_t compress(const char* source, size_t inputSize, char* dest);

  













  static MFBT_API size_t compressLimitedOutput(const char* source, size_t inputSize, char* dest,
                                               size_t maxOutputSize);

  












  static MFBT_API bool decompress(const char* source, char* dest, size_t outputSize);

  















  static MFBT_API bool decompress(const char* source, size_t inputSize, char* dest,
                                  size_t maxOutputSize, size_t *outputSize);

  








  static MFBT_API size_t maxCompressedSize(size_t inputSize)
  {
      size_t max = ((inputSize) + ((inputSize)/255) + 16);
      MOZ_ASSERT(max > inputSize);
      return max;
  }

};

} 
} 

#endif 
