






#ifndef mozilla_MathAlgorithms_h_
#define mozilla_MathAlgorithms_h_

#include "mozilla/Assertions.h"

namespace mozilla {


template<typename IntegerType>
MOZ_ALWAYS_INLINE IntegerType
EuclidGCD(IntegerType a, IntegerType b)
{
  
  
  MOZ_ASSERT(a > 0);
  MOZ_ASSERT(b > 0);

  while (a != b) {
    if (a > b) {
      a = a - b;
    } else {
      b = b - a;
    }
  }

  return a;
}


template<typename IntegerType>
MOZ_ALWAYS_INLINE IntegerType
EuclidLCM(IntegerType a, IntegerType b)
{
  
  return (a / EuclidGCD(a, b)) * b;
}

} 

#endif  
