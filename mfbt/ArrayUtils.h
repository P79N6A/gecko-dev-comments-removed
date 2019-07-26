









#ifndef mozilla_ArrayUtils_h
#define mozilla_ArrayUtils_h

#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"

#include <stddef.h>

#ifdef __cplusplus

#include "mozilla/Array.h"

namespace mozilla {







template<class T>
MOZ_ALWAYS_INLINE size_t
PointerRangeSize(T* aBegin, T* aEnd)
{
  MOZ_ASSERT(aEnd >= aBegin);
  return (size_t(aEnd) - size_t(aBegin)) / sizeof(T);
}







template<typename T, size_t N>
MOZ_CONSTEXPR size_t
ArrayLength(T (&aArr)[N])
{
  return N;
}

template<typename T, size_t N>
MOZ_CONSTEXPR size_t
ArrayLength(const Array<T, N>& aArr)
{
  return N;
}






template<typename T, size_t N>
MOZ_CONSTEXPR T*
ArrayEnd(T (&aArr)[N])
{
  return aArr + ArrayLength(aArr);
}

template<typename T, size_t N>
MOZ_CONSTEXPR T*
ArrayEnd(Array<T, N>& aArr)
{
  return &aArr[0] + ArrayLength(aArr);
}

template<typename T, size_t N>
MOZ_CONSTEXPR const T*
ArrayEnd(const Array<T, N>& aArr)
{
  return &aArr[0] + ArrayLength(aArr);
}

namespace detail {





template <typename T, size_t N>
char (&ArrayLengthHelper(T (&array)[N]))[N];

} 

} 

#endif 






#ifdef __cplusplus
#  define MOZ_ARRAY_LENGTH(array)   sizeof(mozilla::detail::ArrayLengthHelper(array))
#else
#  define MOZ_ARRAY_LENGTH(array)   (sizeof(array)/sizeof((array)[0]))
#endif

#endif 
