












#ifndef mozilla_PodOperations_h
#define mozilla_PodOperations_h

#include "mozilla/Attributes.h"
#include "mozilla/Util.h"

#include <string.h>

namespace mozilla {


template<typename T>
static void
PodZero(T* t)
{
  memset(t, 0, sizeof(T));
}


template<typename T>
static void
PodZero(T* t, size_t nelem)
{
  




  for (T* end = t + nelem; t < end; t++)
    memset(t, 0, sizeof(T));
}








template<typename T, size_t N>
static void PodZero(T (&t)[N]) MOZ_DELETE;
template<typename T, size_t N>
static void PodZero(T (&t)[N], size_t nelem) MOZ_DELETE;


template <class T, size_t N>
static void
PodArrayZero(T (&t)[N])
{
  memset(t, 0, N * sizeof(T));
}





template<typename T>
static void
PodAssign(T* dst, const T* src)
{
  MOZ_ASSERT(dst != src);
  MOZ_ASSERT_IF(src < dst, PointerRangeSize(src, static_cast<const T*>(dst)) >= 1);
  MOZ_ASSERT_IF(dst < src, PointerRangeSize(static_cast<const T*>(dst), src) >= 1);
  memcpy(reinterpret_cast<char*>(dst), reinterpret_cast<const char*>(src), sizeof(T));
}





template<typename T>
MOZ_ALWAYS_INLINE static void
PodCopy(T* dst, const T* src, size_t nelem)
{
  MOZ_ASSERT(dst != src);
  MOZ_ASSERT_IF(src < dst, PointerRangeSize(src, static_cast<const T*>(dst)) >= nelem);
  MOZ_ASSERT_IF(dst < src, PointerRangeSize(static_cast<const T*>(dst), src) >= nelem);

  if (nelem < 128) {
    



    for (const T* srcend = src + nelem; src < srcend; src++, dst++)
      PodAssign(dst, src);
  } else {
    memcpy(dst, src, nelem * sizeof(T));
  }
}





template<typename T>
MOZ_ALWAYS_INLINE static bool
PodEqual(const T* one, const T* two, size_t len)
{
  if (len < 128) {
    const T* p1end = one + len;
    const T* p1 = one;
    const T* p2 = two;
    for (; p1 < p1end; p1++, p2++) {
      if (*p1 != *p2)
        return false;
    }
    return true;
  }

  return !memcmp(one, two, len * sizeof(T));
}

} 

#endif 
