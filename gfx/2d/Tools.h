




#ifndef MOZILLA_GFX_TOOLS_H_
#define MOZILLA_GFX_TOOLS_H_

#include "mozilla/CheckedInt.h"
#include "mozilla/TypeTraits.h"
#include "Types.h"
#include "Point.h"
#include <math.h>
#if defined(_MSC_VER) && (_MSC_VER < 1600)
#define hypotf _hypotf
#endif

namespace mozilla {
namespace gfx {

static inline bool
IsOperatorBoundByMask(CompositionOp aOp) {
  switch (aOp) {
  case CompositionOp::OP_IN:
  case CompositionOp::OP_OUT:
  case CompositionOp::OP_DEST_IN:
  case CompositionOp::OP_DEST_ATOP:
  case CompositionOp::OP_SOURCE:
    return false;
  default:
    return true;
  }
}

template <class T>
struct ClassStorage
{
  char bytes[sizeof(T)];

  const T *addr() const { return (const T *)bytes; }
  T *addr() { return (T *)(void *)bytes; }
};

static inline bool
FuzzyEqual(Float aA, Float aB, Float aErr)
{
  if ((aA + aErr >= aB) && (aA - aErr <= aB)) {
    return true;
  }
  return false;
}

static inline void
NudgeToInteger(float *aVal)
{
  float r = floorf(*aVal + 0.5f);
  
  
  
  
  
  if (FuzzyEqual(r, *aVal, r == 0.0f ? 1e-6f : fabs(r*1e-6f))) {
    *aVal = r;
  }
}

static inline void
NudgeToInteger(float *aVal, float aErr)
{
  float r = floorf(*aVal + 0.5f);
  if (FuzzyEqual(r, *aVal, aErr)) {
    *aVal = r;
  }
}

static inline Float
Distance(Point aA, Point aB)
{
  return hypotf(aB.x - aA.x, aB.y - aA.y);
}

static inline int
BytesPerPixel(SurfaceFormat aFormat)
{
  switch (aFormat) {
  case SurfaceFormat::A8:
    return 1;
  case SurfaceFormat::R5G6B5:
    return 2;
  default:
    return 4;
  }
}

template<typename T, int alignment = 16>
struct AlignedArray
{
  typedef T value_type;

  AlignedArray()
    : mPtr(nullptr)
    , mStorage(nullptr)
  {
  }

  explicit MOZ_ALWAYS_INLINE AlignedArray(size_t aCount, bool aZero = false)
    : mStorage(nullptr)
    , mCount(0)
  {
    Realloc(aCount, aZero);
  }

  MOZ_ALWAYS_INLINE ~AlignedArray()
  {
    Dealloc();
  }

  void Dealloc()
  {
    
    
    
    
    static_assert(mozilla::IsPod<T>::value,
                  "Destructors must be invoked for this type");
#if 0
    for (size_t i = 0; i < mCount; ++i) {
      
      
      
      
      mPtr[i].~T();
    }
#endif

    delete [] mStorage;
    mStorage = nullptr;
    mPtr = nullptr;
  }

  MOZ_ALWAYS_INLINE void Realloc(size_t aCount, bool aZero = false)
  {
    delete [] mStorage;
    CheckedInt32 storageByteCount =
      CheckedInt32(sizeof(T)) * aCount + (alignment - 1);
    if (!storageByteCount.isValid()) {
      mStorage = nullptr;
      mPtr = nullptr;
      mCount = 0;
      return;
    }
    
    
    if (aZero) {
      mStorage = static_cast<uint8_t *>(calloc(1, storageByteCount.value()));
    } else {
      mStorage = new (std::nothrow) uint8_t[storageByteCount.value()];
    }
    if (!mStorage) {
      mStorage = nullptr;
      mPtr = nullptr;
      mCount = 0;
      return;
    }
    if (uintptr_t(mStorage) % alignment) {
      
      mPtr = (T*)(uintptr_t(mStorage) + alignment - (uintptr_t(mStorage) % alignment));
    } else {
      mPtr = (T*)(mStorage);
    }
    
    
    
    
    mPtr = new (mPtr) T[aCount];
    mCount = aCount;
  }

  MOZ_ALWAYS_INLINE operator T*()
  {
    return mPtr;
  }

  T *mPtr;

private:
  uint8_t *mStorage;
  size_t mCount;
};









template<int alignment>
int32_t GetAlignedStride(int32_t aStride)
{
  static_assert(alignment > 0 && (alignment & (alignment-1)) == 0,
                "This implementation currently require power-of-two alignment");
  const int32_t mask = alignment - 1;
  return (aStride + mask) & ~mask;
}

}
}

#endif 
