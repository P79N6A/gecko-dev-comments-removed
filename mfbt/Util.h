










#ifndef mozilla_Util_h
#define mozilla_Util_h

#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/Types.h"

#ifdef __cplusplus

#include "mozilla/Alignment.h"
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

} 

#endif 






#ifdef MOZ_HAVE_CXX11_CONSTEXPR
#  define MOZ_ARRAY_LENGTH(array)   mozilla::ArrayLength(array)
#else
#  define MOZ_ARRAY_LENGTH(array)   (sizeof(array)/sizeof((array)[0]))
#endif

#endif 
