







#ifndef mozilla_NumericLimits_h
#define mozilla_NumericLimits_h

#include "mozilla/Char16.h"

#include <limits>
#include <stdint.h>

namespace mozilla {







template<typename T>
class NumericLimits : public std::numeric_limits<T>
{
};

#ifdef MOZ_CHAR16_IS_NOT_WCHAR
template<>
class NumericLimits<char16_t> : public std::numeric_limits<uint16_t>
{
  
};
#endif

} 

#endif 
