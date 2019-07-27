









#ifndef mozilla_ArrayUtils_h
#define mozilla_ArrayUtils_h

#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"

#include <stddef.h>

#ifdef __cplusplus

#include "mozilla/Alignment.h"
#include "mozilla/Array.h"
#include "mozilla/TypeTraits.h"

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

template<typename AlignType, typename Pointee,
         typename = EnableIf<!IsVoid<AlignType>::value>>
struct AlignedChecker
{
  static void
  test(const Pointee* aPtr)
  {
    MOZ_ASSERT((uintptr_t(aPtr) % MOZ_ALIGNOF(AlignType)) == 0,
               "performing a range-check with a misaligned pointer");
  }
};

template<typename AlignType, typename Pointee>
struct AlignedChecker<AlignType, Pointee>
{
  static void
  test(const Pointee* aPtr)
  {
  }
};

} 














template<typename T, typename U>
inline typename EnableIf<IsSame<T, U>::value ||
                         IsBaseOf<T, U>::value ||
                         IsVoid<T>::value,
                         bool>::Type
IsInRange(const T* aPtr, const U* aBegin, const U* aEnd)
{
  MOZ_ASSERT(aBegin <= aEnd);
  detail::AlignedChecker<U, T>::test(aPtr);
  detail::AlignedChecker<U, U>::test(aBegin);
  detail::AlignedChecker<U, U>::test(aEnd);
  return aBegin <= reinterpret_cast<const U*>(aPtr) &&
         reinterpret_cast<const U*>(aPtr) < aEnd;
}






template<typename T>
inline bool
IsInRange(const T* aPtr, uintptr_t aBegin, uintptr_t aEnd)
{
  return IsInRange(aPtr,
                   reinterpret_cast<const T*>(aBegin),
                   reinterpret_cast<const T*>(aEnd));
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
