










#ifndef mozilla_RangedPtr_h
#define mozilla_RangedPtr_h

#include "mozilla/ArrayUtils.h"
#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"

#include <stdint.h>

namespace mozilla {




















template<typename T>
class RangedPtr
{
  T* mPtr;

#ifdef DEBUG
  T* const mRangeStart;
  T* const mRangeEnd;
#endif

  void checkSanity()
  {
    MOZ_ASSERT(mRangeStart <= mPtr);
    MOZ_ASSERT(mPtr <= mRangeEnd);
  }

  
  RangedPtr<T> create(T* aPtr) const
  {
#ifdef DEBUG
    return RangedPtr<T>(aPtr, mRangeStart, mRangeEnd);
#else
    return RangedPtr<T>(aPtr, nullptr, size_t(0));
#endif
  }

  uintptr_t asUintptr() const { return reinterpret_cast<uintptr_t>(mPtr); }

public:
  RangedPtr(T* aPtr, T* aStart, T* aEnd)
    : mPtr(aPtr)
#ifdef DEBUG
    , mRangeStart(aStart), mRangeEnd(aEnd)
#endif
  {
    MOZ_ASSERT(mRangeStart <= mRangeEnd);
    checkSanity();
  }
  RangedPtr(T* aPtr, T* aStart, size_t aLength)
    : mPtr(aPtr)
#ifdef DEBUG
    , mRangeStart(aStart), mRangeEnd(aStart + aLength)
#endif
  {
    MOZ_ASSERT(aLength <= size_t(-1) / sizeof(T));
    MOZ_ASSERT(reinterpret_cast<uintptr_t>(mRangeStart) + aLength * sizeof(T) >=
               reinterpret_cast<uintptr_t>(mRangeStart));
    checkSanity();
  }

  
  RangedPtr(T* aPtr, size_t aLength)
    : mPtr(aPtr)
#ifdef DEBUG
    , mRangeStart(aPtr), mRangeEnd(aPtr + aLength)
#endif
  {
    MOZ_ASSERT(aLength <= size_t(-1) / sizeof(T));
    MOZ_ASSERT(reinterpret_cast<uintptr_t>(mRangeStart) + aLength * sizeof(T) >=
               reinterpret_cast<uintptr_t>(mRangeStart));
    checkSanity();
  }

  
  template<size_t N>
  RangedPtr(T (&aArr)[N])
    : mPtr(aArr)
#ifdef DEBUG
    , mRangeStart(aArr), mRangeEnd(aArr + N)
#endif
  {
    checkSanity();
  }

  T* get() const { return mPtr; }

  explicit operator bool() const { return mPtr != nullptr; }

  









  RangedPtr<T>& operator=(const RangedPtr<T>& aOther)
  {
    MOZ_ASSERT(mRangeStart == aOther.mRangeStart);
    MOZ_ASSERT(mRangeEnd == aOther.mRangeEnd);
    mPtr = aOther.mPtr;
    checkSanity();
    return *this;
  }

  RangedPtr<T> operator+(size_t aInc)
  {
    MOZ_ASSERT(aInc <= size_t(-1) / sizeof(T));
    MOZ_ASSERT(asUintptr() + aInc * sizeof(T) >= asUintptr());
    return create(mPtr + aInc);
  }

  RangedPtr<T> operator-(size_t aDec)
  {
    MOZ_ASSERT(aDec <= size_t(-1) / sizeof(T));
    MOZ_ASSERT(asUintptr() - aDec * sizeof(T) <= asUintptr());
    return create(mPtr - aDec);
  }

  



  template <typename U>
  RangedPtr<T>& operator=(U* aPtr)
  {
    *this = create(aPtr);
    return *this;
  }

  template <typename U>
  RangedPtr<T>& operator=(const RangedPtr<U>& aPtr)
  {
    MOZ_ASSERT(mRangeStart <= aPtr.mPtr);
    MOZ_ASSERT(aPtr.mPtr <= mRangeEnd);
    mPtr = aPtr.mPtr;
    checkSanity();
    return *this;
  }

  RangedPtr<T>& operator++()
  {
    return (*this += 1);
  }

  RangedPtr<T> operator++(int)
  {
    RangedPtr<T> rcp = *this;
    ++*this;
    return rcp;
  }

  RangedPtr<T>& operator--()
  {
    return (*this -= 1);
  }

  RangedPtr<T> operator--(int)
  {
    RangedPtr<T> rcp = *this;
    --*this;
    return rcp;
  }

  RangedPtr<T>& operator+=(size_t aInc)
  {
    *this = *this + aInc;
    return *this;
  }

  RangedPtr<T>& operator-=(size_t aDec)
  {
    *this = *this - aDec;
    return *this;
  }

  T& operator[](int aIndex) const
  {
    MOZ_ASSERT(size_t(aIndex > 0 ? aIndex : -aIndex) <= size_t(-1) / sizeof(T));
    return *create(mPtr + aIndex);
  }

  T& operator*() const
  {
    MOZ_ASSERT(mPtr >= mRangeStart);
    MOZ_ASSERT(mPtr < mRangeEnd);
    return *mPtr;
  }

  template <typename U>
  bool operator==(const RangedPtr<U>& aOther) const
  {
    return mPtr == aOther.mPtr;
  }
  template <typename U>
  bool operator!=(const RangedPtr<U>& aOther) const
  {
    return !(*this == aOther);
  }

  template<typename U>
  bool operator==(const U* u) const
  {
    return mPtr == u;
  }
  template<typename U>
  bool operator!=(const U* u) const
  {
    return !(*this == u);
  }

  template <typename U>
  bool operator<(const RangedPtr<U>& aOther) const
  {
    return mPtr < aOther.mPtr;
  }
  template <typename U>
  bool operator<=(const RangedPtr<U>& aOther) const
  {
    return mPtr <= aOther.mPtr;
  }

  template <typename U>
  bool operator>(const RangedPtr<U>& aOther) const
  {
    return mPtr > aOther.mPtr;
  }
  template <typename U>
  bool operator>=(const RangedPtr<U>& aOther) const
  {
    return mPtr >= aOther.mPtr;
  }

  size_t operator-(const RangedPtr<T>& aOther) const
  {
    MOZ_ASSERT(mPtr >= aOther.mPtr);
    return PointerRangeSize(aOther.mPtr, mPtr);
  }

private:
  RangedPtr() = delete;
  T* operator&() = delete;
};

} 

#endif 
