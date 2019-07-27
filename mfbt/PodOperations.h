













#ifndef mozilla_PodOperations_h
#define mozilla_PodOperations_h

#include "mozilla/Array.h"
#include "mozilla/ArrayUtils.h"
#include "mozilla/Attributes.h"

#include <stdint.h>
#include <string.h>

namespace mozilla {


template<typename T>
static MOZ_ALWAYS_INLINE void
PodZero(T* aT)
{
  memset(aT, 0, sizeof(T));
}


template<typename T>
static MOZ_ALWAYS_INLINE void
PodZero(T* aT, size_t aNElem)
{
  




  for (T* end = aT + aNElem; aT < end; aT++) {
    memset(aT, 0, sizeof(T));
  }
}








template<typename T, size_t N>
static void PodZero(T (&aT)[N]) = delete;
template<typename T, size_t N>
static void PodZero(T (&aT)[N], size_t aNElem) = delete;


template <class T, size_t N>
static MOZ_ALWAYS_INLINE void
PodArrayZero(T (&aT)[N])
{
  memset(aT, 0, N * sizeof(T));
}

template <typename T, size_t N>
static MOZ_ALWAYS_INLINE void
PodArrayZero(Array<T, N>& aArr)
{
  memset(&aArr[0], 0, N * sizeof(T));
}





template<typename T>
static MOZ_ALWAYS_INLINE void
PodAssign(T* aDst, const T* aSrc)
{
  MOZ_ASSERT(aDst + 1 <= aSrc || aSrc + 1 <= aDst,
             "destination and source must not overlap");
  memcpy(reinterpret_cast<char*>(aDst), reinterpret_cast<const char*>(aSrc),
         sizeof(T));
}





template<typename T>
static MOZ_ALWAYS_INLINE void
PodCopy(T* aDst, const T* aSrc, size_t aNElem)
{
  MOZ_ASSERT(aDst + aNElem <= aSrc || aSrc + aNElem <= aDst,
             "destination and source must not overlap");
  if (aNElem < 128) {
    



    for (const T* srcend = aSrc + aNElem; aSrc < srcend; aSrc++, aDst++) {
      PodAssign(aDst, aSrc);
    }
  } else {
    memcpy(aDst, aSrc, aNElem * sizeof(T));
  }
}

template<typename T>
static MOZ_ALWAYS_INLINE void
PodCopy(volatile T* aDst, const volatile T* aSrc, size_t aNElem)
{
  MOZ_ASSERT(aDst + aNElem <= aSrc || aSrc + aNElem <= aDst,
             "destination and source must not overlap");

  





  for (const volatile T* srcend = aSrc + aNElem;
       aSrc < srcend;
       aSrc++, aDst++) {
    *aDst = *aSrc;
  }
}





template <class T, size_t N>
static MOZ_ALWAYS_INLINE void
PodArrayCopy(T (&aDst)[N], const T (&aSrc)[N])
{
  PodCopy(aDst, aSrc, N);
}







template<typename T>
static MOZ_ALWAYS_INLINE void
PodMove(T* aDst, const T* aSrc, size_t aNElem)
{
  MOZ_ASSERT(aNElem <= SIZE_MAX / sizeof(T),
             "trying to move an impossible number of elements");
  memmove(aDst, aSrc, aNElem * sizeof(T));
}





template<typename T>
static MOZ_ALWAYS_INLINE bool
PodEqual(const T* one, const T* two, size_t len)
{
  if (len < 128) {
    const T* p1end = one + len;
    const T* p1 = one;
    const T* p2 = two;
    for (; p1 < p1end; p1++, p2++) {
      if (*p1 != *p2) {
        return false;
      }
    }
    return true;
  }

  return !memcmp(one, two, len * sizeof(T));
}

} 

#endif 
