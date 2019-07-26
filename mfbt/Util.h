










#ifndef mozilla_Util_h
#define mozilla_Util_h

#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/Types.h"

#ifdef __cplusplus

#include "mozilla/Alignment.h"

namespace mozilla {







template<class T>
MOZ_ALWAYS_INLINE size_t
PointerRangeSize(T* begin, T* end)
{
  MOZ_ASSERT(end >= begin);
  return (size_t(end) - size_t(begin)) / sizeof(T);
}







template<typename T, size_t N>
MOZ_CONSTEXPR size_t
ArrayLength(T (&arr)[N])
{
  return N;
}






template<typename T, size_t N>
MOZ_CONSTEXPR T*
ArrayEnd(T (&arr)[N])
{
  return arr + ArrayLength(arr);
}

} 

#endif 






#ifdef MOZ_HAVE_CXX11_CONSTEXPR
#  define MOZ_ARRAY_LENGTH(array)   mozilla::ArrayLength(array)
#else
#  define MOZ_ARRAY_LENGTH(array)   (sizeof(array)/sizeof((array)[0]))
#endif

#endif 
