









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
MOZ_CONSTEXPR size_t
ArrayLength(const Array<T, N>& arr)
{
  return N;
}






template<typename T, size_t N>
MOZ_CONSTEXPR T*
ArrayEnd(T (&arr)[N])
{
  return arr + ArrayLength(arr);
}

template<typename T, size_t N>
MOZ_CONSTEXPR T*
ArrayEnd(Array<T, N>& arr)
{
  return &arr[0] + ArrayLength(arr);
}

template<typename T, size_t N>
MOZ_CONSTEXPR const T*
ArrayEnd(const Array<T, N>& arr)
{
  return &arr[0] + ArrayLength(arr);
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
