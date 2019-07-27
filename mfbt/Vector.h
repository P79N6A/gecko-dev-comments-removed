







#ifndef mozilla_Vector_h
#define mozilla_Vector_h

#include "mozilla/Alignment.h"
#include "mozilla/AllocPolicy.h"
#include "mozilla/ArrayUtils.h" 
#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/MathAlgorithms.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/Move.h"
#include "mozilla/NullPtr.h"
#include "mozilla/ReentrancyGuard.h"
#include "mozilla/TemplateLib.h"
#include "mozilla/TypeTraits.h"

#include <new> 


#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4345)
#endif

namespace mozilla {

template<typename T, size_t N, class AllocPolicy, class ThisVector>
class VectorBase;

namespace detail {






template<typename T>
static bool CapacityHasExcessSpace(size_t aCapacity)
{
  size_t size = aCapacity * sizeof(T);
  return RoundUpPow2(size) - size >= sizeof(T);
}





template<typename T, size_t N, class AP, class ThisVector, bool IsPod>
struct VectorImpl
{
  
  static inline void destroy(T* aBegin, T* aEnd)
  {
    MOZ_ASSERT(aBegin <= aEnd);
    for (T* p = aBegin; p < aEnd; ++p) {
      p->~T();
    }
  }

  
  static inline void initialize(T* aBegin, T* aEnd)
  {
    MOZ_ASSERT(aBegin <= aEnd);
    for (T* p = aBegin; p < aEnd; ++p) {
      new(p) T();
    }
  }

  



  template<typename U>
  static inline void copyConstruct(T* aDst,
                                   const U* aSrcStart, const U* aSrcEnd)
  {
    MOZ_ASSERT(aSrcStart <= aSrcEnd);
    for (const U* p = aSrcStart; p < aSrcEnd; ++p, ++aDst) {
      new(aDst) T(*p);
    }
  }

  



  template<typename U>
  static inline void moveConstruct(T* aDst, U* aSrcStart, U* aSrcEnd)
  {
    MOZ_ASSERT(aSrcStart <= aSrcEnd);
    for (U* p = aSrcStart; p < aSrcEnd; ++p, ++aDst) {
      new(aDst) T(Move(*p));
    }
  }

  



  template<typename U>
  static inline void copyConstructN(T* aDst, size_t aN, const U& aU)
  {
    for (T* end = aDst + aN; aDst < end; ++aDst) {
      new(aDst) T(aU);
    }
  }

  





  static inline bool
  growTo(VectorBase<T, N, AP, ThisVector>& aV, size_t aNewCap)
  {
    MOZ_ASSERT(!aV.usingInlineStorage());
    MOZ_ASSERT(!CapacityHasExcessSpace<T>(aNewCap));
    T* newbuf = reinterpret_cast<T*>(aV.malloc_(aNewCap * sizeof(T)));
    if (!newbuf) {
      return false;
    }
    T* dst = newbuf;
    T* src = aV.beginNoCheck();
    for (; src < aV.endNoCheck(); ++dst, ++src) {
      new(dst) T(Move(*src));
    }
    VectorImpl::destroy(aV.beginNoCheck(), aV.endNoCheck());
    aV.free_(aV.mBegin);
    aV.mBegin = newbuf;
    
    aV.mCapacity = aNewCap;
    return true;
  }
};






template<typename T, size_t N, class AP, class ThisVector>
struct VectorImpl<T, N, AP, ThisVector, true>
{
  static inline void destroy(T*, T*) {}

  static inline void initialize(T* aBegin, T* aEnd)
  {
    







    MOZ_ASSERT(aBegin <= aEnd);
    for (T* p = aBegin; p < aEnd; ++p) {
      new(p) T();
    }
  }

  template<typename U>
  static inline void copyConstruct(T* aDst,
                                   const U* aSrcStart, const U* aSrcEnd)
  {
    






    MOZ_ASSERT(aSrcStart <= aSrcEnd);
    for (const U* p = aSrcStart; p < aSrcEnd; ++p, ++aDst) {
      *aDst = *p;
    }
  }

  template<typename U>
  static inline void moveConstruct(T* aDst,
                                   const U* aSrcStart, const U* aSrcEnd)
  {
    copyConstruct(aDst, aSrcStart, aSrcEnd);
  }

  static inline void copyConstructN(T* aDst, size_t aN, const T& aT)
  {
    for (T* end = aDst + aN; aDst < end; ++aDst) {
      *aDst = aT;
    }
  }

  static inline bool
  growTo(VectorBase<T, N, AP, ThisVector>& aV, size_t aNewCap)
  {
    MOZ_ASSERT(!aV.usingInlineStorage());
    MOZ_ASSERT(!CapacityHasExcessSpace<T>(aNewCap));
    T* newbuf = aV.template pod_realloc<T>(aV.mBegin, aV.mCapacity, aNewCap);
    if (!newbuf) {
      return false;
    }
    aV.mBegin = newbuf;
    
    aV.mCapacity = aNewCap;
    return true;
  }
};

} 








template<typename T, size_t N, class AllocPolicy, class ThisVector>
class VectorBase : private AllocPolicy
{
  

  static const bool kElemIsPod = IsPod<T>::value;
  typedef detail::VectorImpl<T, N, AllocPolicy, ThisVector, kElemIsPod> Impl;
  friend struct detail::VectorImpl<T, N, AllocPolicy, ThisVector, kElemIsPod>;

  bool growStorageBy(size_t aIncr);
  bool convertToHeapStorage(size_t aNewCap);

  

  static const int kMaxInlineBytes = 1024;

  

  








  template<int M, int Dummy>
  struct ElemSize
  {
    static const size_t value = sizeof(T);
  };
  template<int Dummy>
  struct ElemSize<0, Dummy>
  {
    static const size_t value = 1;
  };

  static const size_t kInlineCapacity =
    tl::Min<N, kMaxInlineBytes / ElemSize<N, 0>::value>::value;

  
  static const size_t kInlineBytes =
    tl::Max<1, kInlineCapacity * ElemSize<N, 0>::value>::value;

  

  






  T* mBegin;

  
  size_t mLength;

  
  size_t mCapacity;

#ifdef DEBUG
  
  size_t mReserved;
#endif

  
  AlignedStorage<kInlineBytes> mStorage;

#ifdef DEBUG
  friend class ReentrancyGuard;
  bool mEntered;
#endif

  

  bool usingInlineStorage() const
  {
    return mBegin == const_cast<VectorBase*>(this)->inlineStorage();
  }

  T* inlineStorage()
  {
    return static_cast<T*>(mStorage.addr());
  }

  T* beginNoCheck() const
  {
    return mBegin;
  }

  T* endNoCheck()
  {
    return mBegin + mLength;
  }

  const T* endNoCheck() const
  {
    return mBegin + mLength;
  }

#ifdef DEBUG
  size_t reserved() const
  {
    MOZ_ASSERT(mReserved <= mCapacity);
    MOZ_ASSERT(mLength <= mReserved);
    return mReserved;
  }
#endif

  
  template<typename U> void internalAppend(U&& aU);
  template<typename U, size_t O, class BP, class UV>
  void internalAppendAll(const VectorBase<U, O, BP, UV>& aU);
  void internalAppendN(const T& aT, size_t aN);
  template<typename U> void internalAppend(const U* aBegin, size_t aLength);

public:
  static const size_t sMaxInlineStorage = N;

  typedef T ElementType;

  explicit VectorBase(AllocPolicy = AllocPolicy());
  explicit VectorBase(ThisVector&&); 
  ThisVector& operator=(ThisVector&&); 
  ~VectorBase();

  

  const AllocPolicy& allocPolicy() const { return *this; }

  AllocPolicy& allocPolicy() { return *this; }

  enum { InlineLength = N };

  size_t length() const { return mLength; }

  bool empty() const { return mLength == 0; }

  size_t capacity() const { return mCapacity; }

  T* begin()
  {
    MOZ_ASSERT(!mEntered);
    return mBegin;
  }

  const T* begin() const
  {
    MOZ_ASSERT(!mEntered);
    return mBegin;
  }

  T* end()
  {
    MOZ_ASSERT(!mEntered);
    return mBegin + mLength;
  }

  const T* end() const
  {
    MOZ_ASSERT(!mEntered);
    return mBegin + mLength;
  }

  T& operator[](size_t aIndex)
  {
    MOZ_ASSERT(!mEntered);
    MOZ_ASSERT(aIndex < mLength);
    return begin()[aIndex];
  }

  const T& operator[](size_t aIndex) const
  {
    MOZ_ASSERT(!mEntered);
    MOZ_ASSERT(aIndex < mLength);
    return begin()[aIndex];
  }

  T& back()
  {
    MOZ_ASSERT(!mEntered);
    MOZ_ASSERT(!empty());
    return *(end() - 1);
  }

  const T& back() const
  {
    MOZ_ASSERT(!mEntered);
    MOZ_ASSERT(!empty());
    return *(end() - 1);
  }

  class Range
  {
    friend class VectorBase;
    T* mCur;
    T* mEnd;
    Range(T* aCur, T* aEnd)
      : mCur(aCur)
      , mEnd(aEnd)
    {
      MOZ_ASSERT(aCur <= aEnd);
    }

  public:
    Range() {}
    bool empty() const { return mCur == mEnd; }
    size_t remain() const { return PointerRangeSize(mCur, mEnd); }
    T& front() const { MOZ_ASSERT(!empty()); return *mCur; }
    void popFront() { MOZ_ASSERT(!empty()); ++mCur; }
    T popCopyFront() { MOZ_ASSERT(!empty()); return *mCur++; }
  };

  Range all() { return Range(begin(), end()); }

  

  



  bool initCapacity(size_t aRequest);

  



  bool reserve(size_t aRequest);

  



  void shrinkBy(size_t aIncr);

  
  bool growBy(size_t aIncr);

  
  bool resize(size_t aNewLength);

  



  bool growByUninitialized(size_t aIncr);
  bool resizeUninitialized(size_t aNewLength);

  
  void clear();

  
  void clearAndFree();

  





  bool canAppendWithoutRealloc(size_t aNeeded) const;

  

  




  template<typename U> bool append(U&& aU);

  template<typename U, size_t O, class BP, class UV>
  bool appendAll(const VectorBase<U, O, BP, UV>& aU);
  bool appendN(const T& aT, size_t aN);
  template<typename U> bool append(const U* aBegin, const U* aEnd);
  template<typename U> bool append(const U* aBegin, size_t aLength);

  




  template<typename U> void infallibleAppend(U&& aU)
  {
    internalAppend(Forward<U>(aU));
  }
  void infallibleAppendN(const T& aT, size_t aN)
  {
    internalAppendN(aT, aN);
  }
  template<typename U> void infallibleAppend(const U* aBegin, const U* aEnd)
  {
    internalAppend(aBegin, PointerRangeSize(aBegin, aEnd));
  }
  template<typename U> void infallibleAppend(const U* aBegin, size_t aLength)
  {
    internalAppend(aBegin, aLength);
  }

  void popBack();

  T popCopy();

  









  T* extractRawBuffer();

  







  void replaceRawBuffer(T* aP, size_t aLength);

  














  template<typename U>
  T* insert(T* aP, U&& aVal);

  



  void erase(T* aT);

  




  void erase(T* aBegin, T* aEnd);

  


  size_t sizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const;

  



  size_t sizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const;

  void swap(ThisVector& aOther);

private:
  VectorBase(const VectorBase&) MOZ_DELETE;
  void operator=(const VectorBase&) MOZ_DELETE;

  
  VectorBase(VectorBase&&) MOZ_DELETE;
  void operator=(VectorBase&&) MOZ_DELETE;
};


#define MOZ_REENTRANCY_GUARD_ET_AL \
  ReentrancyGuard g(*this); \
  MOZ_ASSERT_IF(usingInlineStorage(), mCapacity == kInlineCapacity); \
  MOZ_ASSERT(reserved() <= mCapacity); \
  MOZ_ASSERT(mLength <= reserved()); \
  MOZ_ASSERT(mLength <= mCapacity)



template<typename T, size_t N, class AP, class TV>
MOZ_ALWAYS_INLINE
VectorBase<T, N, AP, TV>::VectorBase(AP aAP)
  : AP(aAP)
  , mLength(0)
  , mCapacity(kInlineCapacity)
#ifdef DEBUG
  , mReserved(kInlineCapacity)
  , mEntered(false)
#endif
{
  mBegin = static_cast<T*>(mStorage.addr());
}


template<typename T, size_t N, class AllocPolicy, class TV>
MOZ_ALWAYS_INLINE
VectorBase<T, N, AllocPolicy, TV>::VectorBase(TV&& aRhs)
  : AllocPolicy(Move(aRhs))
#ifdef DEBUG
  , mEntered(false)
#endif
{
  mLength = aRhs.mLength;
  mCapacity = aRhs.mCapacity;
#ifdef DEBUG
  mReserved = aRhs.mReserved;
#endif

  if (aRhs.usingInlineStorage()) {
    
    mBegin = static_cast<T*>(mStorage.addr());
    Impl::moveConstruct(mBegin, aRhs.beginNoCheck(), aRhs.endNoCheck());
    



  } else {
    



    mBegin = aRhs.mBegin;
    aRhs.mBegin = static_cast<T*>(aRhs.mStorage.addr());
    aRhs.mCapacity = kInlineCapacity;
    aRhs.mLength = 0;
#ifdef DEBUG
    aRhs.mReserved = kInlineCapacity;
#endif
  }
}


template<typename T, size_t N, class AP, class TV>
MOZ_ALWAYS_INLINE TV&
VectorBase<T, N, AP, TV>::operator=(TV&& aRhs)
{
  MOZ_ASSERT(this != &aRhs, "self-move assignment is prohibited");
  TV* tv = static_cast<TV*>(this);
  tv->~TV();
  new(tv) TV(Move(aRhs));
  return *tv;
}

template<typename T, size_t N, class AP, class TV>
MOZ_ALWAYS_INLINE
VectorBase<T, N, AP, TV>::~VectorBase()
{
  MOZ_REENTRANCY_GUARD_ET_AL;
  Impl::destroy(beginNoCheck(), endNoCheck());
  if (!usingInlineStorage()) {
    this->free_(beginNoCheck());
  }
}






template<typename T, size_t N, class AP, class TV>
inline bool
VectorBase<T, N, AP, TV>::convertToHeapStorage(size_t aNewCap)
{
  MOZ_ASSERT(usingInlineStorage());

  
  MOZ_ASSERT(!detail::CapacityHasExcessSpace<T>(aNewCap));
  T* newBuf = reinterpret_cast<T*>(this->malloc_(aNewCap * sizeof(T)));
  if (!newBuf) {
    return false;
  }

  
  Impl::moveConstruct(newBuf, beginNoCheck(), endNoCheck());
  Impl::destroy(beginNoCheck(), endNoCheck());

  
  mBegin = newBuf;
  
  mCapacity = aNewCap;
  return true;
}

template<typename T, size_t N, class AP, class TV>
MOZ_NEVER_INLINE bool
VectorBase<T, N, AP, TV>::growStorageBy(size_t aIncr)
{
  MOZ_ASSERT(mLength + aIncr > mCapacity);
  MOZ_ASSERT_IF(!usingInlineStorage(),
                !detail::CapacityHasExcessSpace<T>(mCapacity));

  







  size_t newCap;

  if (aIncr == 1) {
    if (usingInlineStorage()) {
      
      size_t newSize =
        tl::RoundUpPow2<(kInlineCapacity + 1) * sizeof(T)>::value;
      newCap = newSize / sizeof(T);
      goto convert;
    }

    if (mLength == 0) {
      
      newCap = 1;
      goto grow;
    }

    

    








    if (mLength & tl::MulOverflowMask<4 * sizeof(T)>::value) {
      this->reportAllocOverflow();
      return false;
    }

    




    newCap = mLength * 2;
    if (detail::CapacityHasExcessSpace<T>(newCap)) {
      newCap += 1;
    }
  } else {
    
    size_t newMinCap = mLength + aIncr;

    
    if (newMinCap < mLength ||
        newMinCap & tl::MulOverflowMask<2 * sizeof(T)>::value)
    {
      this->reportAllocOverflow();
      return false;
    }

    size_t newMinSize = newMinCap * sizeof(T);
    size_t newSize = RoundUpPow2(newMinSize);
    newCap = newSize / sizeof(T);
  }

  if (usingInlineStorage()) {
convert:
    return convertToHeapStorage(newCap);
  }

grow:
  return Impl::growTo(*this, newCap);
}

template<typename T, size_t N, class AP, class TV>
inline bool
VectorBase<T, N, AP, TV>::initCapacity(size_t aRequest)
{
  MOZ_ASSERT(empty());
  MOZ_ASSERT(usingInlineStorage());
  if (aRequest == 0) {
    return true;
  }
  T* newbuf = reinterpret_cast<T*>(this->malloc_(aRequest * sizeof(T)));
  if (!newbuf) {
    return false;
  }
  mBegin = newbuf;
  mCapacity = aRequest;
#ifdef DEBUG
  mReserved = aRequest;
#endif
  return true;
}

template<typename T, size_t N, class AP, class TV>
inline bool
VectorBase<T, N, AP, TV>::reserve(size_t aRequest)
{
  MOZ_REENTRANCY_GUARD_ET_AL;
  if (aRequest > mCapacity && !growStorageBy(aRequest - mLength)) {
    return false;
  }
#ifdef DEBUG
  if (aRequest > mReserved) {
    mReserved = aRequest;
  }
  MOZ_ASSERT(mLength <= mReserved);
  MOZ_ASSERT(mReserved <= mCapacity);
#endif
  return true;
}

template<typename T, size_t N, class AP, class TV>
inline void
VectorBase<T, N, AP, TV>::shrinkBy(size_t aIncr)
{
  MOZ_REENTRANCY_GUARD_ET_AL;
  MOZ_ASSERT(aIncr <= mLength);
  Impl::destroy(endNoCheck() - aIncr, endNoCheck());
  mLength -= aIncr;
}

template<typename T, size_t N, class AP, class TV>
MOZ_ALWAYS_INLINE bool
VectorBase<T, N, AP, TV>::growBy(size_t aIncr)
{
  MOZ_REENTRANCY_GUARD_ET_AL;
  if (aIncr > mCapacity - mLength && !growStorageBy(aIncr)) {
    return false;
  }
  MOZ_ASSERT(mLength + aIncr <= mCapacity);
  T* newend = endNoCheck() + aIncr;
  Impl::initialize(endNoCheck(), newend);
  mLength += aIncr;
#ifdef DEBUG
  if (mLength > mReserved) {
    mReserved = mLength;
  }
#endif
  return true;
}

template<typename T, size_t N, class AP, class TV>
MOZ_ALWAYS_INLINE bool
VectorBase<T, N, AP, TV>::growByUninitialized(size_t aIncr)
{
  MOZ_REENTRANCY_GUARD_ET_AL;
  if (aIncr > mCapacity - mLength && !growStorageBy(aIncr)) {
    return false;
  }
  MOZ_ASSERT(mLength + aIncr <= mCapacity);
  mLength += aIncr;
#ifdef DEBUG
  if (mLength > mReserved) {
    mReserved = mLength;
  }
#endif
  return true;
}

template<typename T, size_t N, class AP, class TV>
inline bool
VectorBase<T, N, AP, TV>::resize(size_t aNewLength)
{
  size_t curLength = mLength;
  if (aNewLength > curLength) {
    return growBy(aNewLength - curLength);
  }
  shrinkBy(curLength - aNewLength);
  return true;
}

template<typename T, size_t N, class AP, class TV>
MOZ_ALWAYS_INLINE bool
VectorBase<T, N, AP, TV>::resizeUninitialized(size_t aNewLength)
{
  size_t curLength = mLength;
  if (aNewLength > curLength) {
    return growByUninitialized(aNewLength - curLength);
  }
  shrinkBy(curLength - aNewLength);
  return true;
}

template<typename T, size_t N, class AP, class TV>
inline void
VectorBase<T, N, AP, TV>::clear()
{
  MOZ_REENTRANCY_GUARD_ET_AL;
  Impl::destroy(beginNoCheck(), endNoCheck());
  mLength = 0;
}

template<typename T, size_t N, class AP, class TV>
inline void
VectorBase<T, N, AP, TV>::clearAndFree()
{
  clear();

  if (usingInlineStorage()) {
    return;
  }
  this->free_(beginNoCheck());
  mBegin = static_cast<T*>(mStorage.addr());
  mCapacity = kInlineCapacity;
#ifdef DEBUG
  mReserved = kInlineCapacity;
#endif
}

template<typename T, size_t N, class AP, class TV>
inline bool
VectorBase<T, N, AP, TV>::canAppendWithoutRealloc(size_t aNeeded) const
{
  return mLength + aNeeded <= mCapacity;
}

template<typename T, size_t N, class AP, class TV>
template<typename U, size_t O, class BP, class UV>
MOZ_ALWAYS_INLINE void
VectorBase<T, N, AP, TV>::internalAppendAll(
  const VectorBase<U, O, BP, UV>& aOther)
{
  internalAppend(aOther.begin(), aOther.length());
}

template<typename T, size_t N, class AP, class TV>
template<typename U>
MOZ_ALWAYS_INLINE void
VectorBase<T, N, AP, TV>::internalAppend(U&& aU)
{
  MOZ_ASSERT(mLength + 1 <= mReserved);
  MOZ_ASSERT(mReserved <= mCapacity);
  new(endNoCheck()) T(Forward<U>(aU));
  ++mLength;
}

template<typename T, size_t N, class AP, class TV>
MOZ_ALWAYS_INLINE bool
VectorBase<T, N, AP, TV>::appendN(const T& aT, size_t aNeeded)
{
  MOZ_REENTRANCY_GUARD_ET_AL;
  if (mLength + aNeeded > mCapacity && !growStorageBy(aNeeded)) {
    return false;
  }
#ifdef DEBUG
  if (mLength + aNeeded > mReserved) {
    mReserved = mLength + aNeeded;
  }
#endif
  internalAppendN(aT, aNeeded);
  return true;
}

template<typename T, size_t N, class AP, class TV>
MOZ_ALWAYS_INLINE void
VectorBase<T, N, AP, TV>::internalAppendN(const T& aT, size_t aNeeded)
{
  MOZ_ASSERT(mLength + aNeeded <= mReserved);
  MOZ_ASSERT(mReserved <= mCapacity);
  Impl::copyConstructN(endNoCheck(), aNeeded, aT);
  mLength += aNeeded;
}

template<typename T, size_t N, class AP, class TV>
template<typename U>
inline T*
VectorBase<T, N, AP, TV>::insert(T* aP, U&& aVal)
{
  MOZ_ASSERT(begin() <= aP);
  MOZ_ASSERT(aP <= end());
  size_t pos = aP - begin();
  MOZ_ASSERT(pos <= mLength);
  size_t oldLength = mLength;
  if (pos == oldLength) {
    if (!append(Forward<U>(aVal))) {
      return nullptr;
    }
  } else {
    T oldBack = Move(back());
    if (!append(Move(oldBack))) { 
      return nullptr;
    }
    for (size_t i = oldLength; i > pos; --i) {
      (*this)[i] = Move((*this)[i - 1]);
    }
    (*this)[pos] = Forward<U>(aVal);
  }
  return begin() + pos;
}

template<typename T, size_t N, class AP, class TV>
inline void
VectorBase<T, N, AP, TV>::erase(T* aIt)
{
  MOZ_ASSERT(begin() <= aIt);
  MOZ_ASSERT(aIt < end());
  while (aIt + 1 < end()) {
    *aIt = Move(*(aIt + 1));
    ++aIt;
  }
  popBack();
}

template<typename T, size_t N, class AP, class TV>
inline void
VectorBase<T, N, AP, TV>::erase(T* aBegin, T* aEnd)
{
  MOZ_ASSERT(begin() <= aBegin);
  MOZ_ASSERT(aBegin <= aEnd);
  MOZ_ASSERT(aEnd <= end());
  while (aEnd < end()) {
    *aBegin++ = Move(*aEnd++);
  }
  shrinkBy(aEnd - aBegin);
}

template<typename T, size_t N, class AP, class TV>
template<typename U>
MOZ_ALWAYS_INLINE bool
VectorBase<T, N, AP, TV>::append(const U* aInsBegin, const U* aInsEnd)
{
  MOZ_REENTRANCY_GUARD_ET_AL;
  size_t aNeeded = PointerRangeSize(aInsBegin, aInsEnd);
  if (mLength + aNeeded > mCapacity && !growStorageBy(aNeeded)) {
    return false;
  }
#ifdef DEBUG
  if (mLength + aNeeded > mReserved) {
    mReserved = mLength + aNeeded;
  }
#endif
  internalAppend(aInsBegin, aNeeded);
  return true;
}

template<typename T, size_t N, class AP, class TV>
template<typename U>
MOZ_ALWAYS_INLINE void
VectorBase<T, N, AP, TV>::internalAppend(const U* aInsBegin, size_t aInsLength)
{
  MOZ_ASSERT(mLength + aInsLength <= mReserved);
  MOZ_ASSERT(mReserved <= mCapacity);
  Impl::copyConstruct(endNoCheck(), aInsBegin, aInsBegin + aInsLength);
  mLength += aInsLength;
}

template<typename T, size_t N, class AP, class TV>
template<typename U>
MOZ_ALWAYS_INLINE bool
VectorBase<T, N, AP, TV>::append(U&& aU)
{
  MOZ_REENTRANCY_GUARD_ET_AL;
  if (mLength == mCapacity && !growStorageBy(1)) {
    return false;
  }
#ifdef DEBUG
  if (mLength + 1 > mReserved) {
    mReserved = mLength + 1;
  }
#endif
  internalAppend(Forward<U>(aU));
  return true;
}

template<typename T, size_t N, class AP, class TV>
template<typename U, size_t O, class BP, class UV>
MOZ_ALWAYS_INLINE bool
VectorBase<T, N, AP, TV>::appendAll(const VectorBase<U, O, BP, UV>& aOther)
{
  return append(aOther.begin(), aOther.length());
}

template<typename T, size_t N, class AP, class TV>
template<class U>
MOZ_ALWAYS_INLINE bool
VectorBase<T, N, AP, TV>::append(const U* aInsBegin, size_t aInsLength)
{
  return append(aInsBegin, aInsBegin + aInsLength);
}

template<typename T, size_t N, class AP, class TV>
MOZ_ALWAYS_INLINE void
VectorBase<T, N, AP, TV>::popBack()
{
  MOZ_REENTRANCY_GUARD_ET_AL;
  MOZ_ASSERT(!empty());
  --mLength;
  endNoCheck()->~T();
}

template<typename T, size_t N, class AP, class TV>
MOZ_ALWAYS_INLINE T
VectorBase<T, N, AP, TV>::popCopy()
{
  T ret = back();
  popBack();
  return ret;
}

template<typename T, size_t N, class AP, class TV>
inline T*
VectorBase<T, N, AP, TV>::extractRawBuffer()
{
  T* ret;
  if (usingInlineStorage()) {
    ret = reinterpret_cast<T*>(this->malloc_(mLength * sizeof(T)));
    if (!ret) {
      return nullptr;
    }
    Impl::copyConstruct(ret, beginNoCheck(), endNoCheck());
    Impl::destroy(beginNoCheck(), endNoCheck());
    
    mLength = 0;
  } else {
    ret = mBegin;
    mBegin = static_cast<T*>(mStorage.addr());
    mLength = 0;
    mCapacity = kInlineCapacity;
#ifdef DEBUG
    mReserved = kInlineCapacity;
#endif
  }
  return ret;
}

template<typename T, size_t N, class AP, class TV>
inline void
VectorBase<T, N, AP, TV>::replaceRawBuffer(T* aP, size_t aLength)
{
  MOZ_REENTRANCY_GUARD_ET_AL;

  
  Impl::destroy(beginNoCheck(), endNoCheck());
  if (!usingInlineStorage()) {
    this->free_(beginNoCheck());
  }

  
  if (aLength <= kInlineCapacity) {
    




    mBegin = static_cast<T*>(mStorage.addr());
    mLength = aLength;
    mCapacity = kInlineCapacity;
    Impl::moveConstruct(mBegin, aP, aP + aLength);
    Impl::destroy(aP, aP + aLength);
    this->free_(aP);
  } else {
    mBegin = aP;
    mLength = aLength;
    mCapacity = aLength;
  }
#ifdef DEBUG
  mReserved = aLength;
#endif
}

template<typename T, size_t N, class AP, class TV>
inline size_t
VectorBase<T, N, AP, TV>::sizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
{
  return usingInlineStorage() ? 0 : aMallocSizeOf(beginNoCheck());
}

template<typename T, size_t N, class AP, class TV>
inline size_t
VectorBase<T, N, AP, TV>::sizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
{
  return aMallocSizeOf(this) + sizeOfExcludingThis(aMallocSizeOf);
}

template<typename T, size_t N, class AP, class TV>
inline void
VectorBase<T, N, AP, TV>::swap(TV& aOther)
{
  static_assert(N == 0,
                "still need to implement this for N != 0");

  
  if (!usingInlineStorage() && aOther.usingInlineStorage()) {
    aOther.mBegin = mBegin;
    mBegin = inlineStorage();
  } else if (usingInlineStorage() && !aOther.usingInlineStorage()) {
    mBegin = aOther.mBegin;
    aOther.mBegin = aOther.inlineStorage();
  } else if (!usingInlineStorage() && !aOther.usingInlineStorage()) {
    Swap(mBegin, aOther.mBegin);
  } else {
    
  }

  Swap(mLength, aOther.mLength);
  Swap(mCapacity, aOther.mCapacity);
#ifdef DEBUG
  Swap(mReserved, aOther.mReserved);
#endif
}



















template<typename T,
         size_t MinInlineCapacity = 0,
         class AllocPolicy = MallocAllocPolicy>
class Vector
  : public VectorBase<T,
                      MinInlineCapacity,
                      AllocPolicy,
                      Vector<T, MinInlineCapacity, AllocPolicy> >
{
  typedef VectorBase<T, MinInlineCapacity, AllocPolicy, Vector> Base;

public:
  explicit Vector(AllocPolicy alloc = AllocPolicy()) : Base(alloc) {}
  Vector(Vector&& vec) : Base(Move(vec)) {}
  Vector& operator=(Vector&& aOther)
  {
    return Base::operator=(Move(aOther));
  }
};

} 

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif 
