







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
static bool CapacityHasExcessSpace(size_t cap)
{
  size_t size = cap * sizeof(T);
  return RoundUpPow2(size) - size >= sizeof(T);
}





template<typename T, size_t N, class AP, class ThisVector, bool IsPod>
struct VectorImpl
{
    
    static inline void destroy(T* begin, T* end) {
      MOZ_ASSERT(begin <= end);
      for (T* p = begin; p < end; ++p)
        p->~T();
    }

    
    static inline void initialize(T* begin, T* end) {
      MOZ_ASSERT(begin <= end);
      for (T* p = begin; p < end; ++p)
        new(p) T();
    }

    



    template<typename U>
    static inline void copyConstruct(T* dst, const U* srcbeg, const U* srcend) {
      MOZ_ASSERT(srcbeg <= srcend);
      for (const U* p = srcbeg; p < srcend; ++p, ++dst)
        new(dst) T(*p);
    }

    



    template<typename U>
    static inline void moveConstruct(T* dst, U* srcbeg, U* srcend) {
      MOZ_ASSERT(srcbeg <= srcend);
      for (U* p = srcbeg; p < srcend; ++p, ++dst)
        new(dst) T(Move(*p));
    }

    



    template<typename U>
    static inline void copyConstructN(T* dst, size_t n, const U& u) {
      for (T* end = dst + n; dst < end; ++dst)
        new(dst) T(u);
    }

    





    static inline bool
    growTo(VectorBase<T, N, AP, ThisVector>& v, size_t newCap) {
      MOZ_ASSERT(!v.usingInlineStorage());
      MOZ_ASSERT(!CapacityHasExcessSpace<T>(newCap));
      T* newbuf = reinterpret_cast<T*>(v.malloc_(newCap * sizeof(T)));
      if (!newbuf)
        return false;
      T* dst = newbuf;
      T* src = v.beginNoCheck();
      for (; src < v.endNoCheck(); ++dst, ++src)
        new(dst) T(Move(*src));
      VectorImpl::destroy(v.beginNoCheck(), v.endNoCheck());
      v.free_(v.mBegin);
      v.mBegin = newbuf;
      
      v.mCapacity = newCap;
      return true;
    }
};






template<typename T, size_t N, class AP, class ThisVector>
struct VectorImpl<T, N, AP, ThisVector, true>
{
    static inline void destroy(T*, T*) {}

    static inline void initialize(T* begin, T* end) {
      







      MOZ_ASSERT(begin <= end);
      for (T* p = begin; p < end; ++p)
        new(p) T();
    }

    template<typename U>
    static inline void copyConstruct(T* dst, const U* srcbeg, const U* srcend) {
      






      MOZ_ASSERT(srcbeg <= srcend);
      for (const U* p = srcbeg; p < srcend; ++p, ++dst)
        *dst = *p;
    }

    template<typename U>
    static inline void moveConstruct(T* dst, const U* srcbeg, const U* srcend) {
      copyConstruct(dst, srcbeg, srcend);
    }

    static inline void copyConstructN(T* dst, size_t n, const T& t) {
      for (T* end = dst + n; dst < end; ++dst)
        *dst = t;
    }

    static inline bool
    growTo(VectorBase<T, N, AP, ThisVector>& v, size_t newCap) {
      MOZ_ASSERT(!v.usingInlineStorage());
      MOZ_ASSERT(!CapacityHasExcessSpace<T>(newCap));
      size_t oldSize = sizeof(T) * v.mCapacity;
      size_t newSize = sizeof(T) * newCap;
      T* newbuf = reinterpret_cast<T*>(v.realloc_(v.mBegin, oldSize, newSize));
      if (!newbuf)
        return false;
      v.mBegin = newbuf;
      
      v.mCapacity = newCap;
      return true;
    }
};

} 








template<typename T, size_t N, class AllocPolicy, class ThisVector>
class VectorBase : private AllocPolicy
{
    

    static const bool sElemIsPod = IsPod<T>::value;
    typedef detail::VectorImpl<T, N, AllocPolicy, ThisVector, sElemIsPod> Impl;
    friend struct detail::VectorImpl<T, N, AllocPolicy, ThisVector, sElemIsPod>;

    bool growStorageBy(size_t incr);
    bool convertToHeapStorage(size_t newCap);

    

    static const int sMaxInlineBytes = 1024;

    

    








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

    static const size_t sInlineCapacity =
      tl::Min<N, sMaxInlineBytes / ElemSize<N, 0>::value>::value;

    
    static const size_t sInlineBytes =
      tl::Max<1, sInlineCapacity * ElemSize<N, 0>::value>::value;

    

    






    T* mBegin;

    
    size_t mLength;

    
    size_t mCapacity;

#ifdef DEBUG
    
    size_t mReserved;
#endif

    
    AlignedStorage<sInlineBytes> storage;

#ifdef DEBUG
    friend class ReentrancyGuard;
    bool entered;
#endif

    

    bool usingInlineStorage() const {
      return mBegin == const_cast<VectorBase*>(this)->inlineStorage();
    }

    T* inlineStorage() {
      return static_cast<T*>(storage.addr());
    }

    T* beginNoCheck() const {
      return mBegin;
    }

    T* endNoCheck() {
      return mBegin + mLength;
    }

    const T* endNoCheck() const {
      return mBegin + mLength;
    }

#ifdef DEBUG
    size_t reserved() const {
      MOZ_ASSERT(mReserved <= mCapacity);
      MOZ_ASSERT(mLength <= mReserved);
      return mReserved;
    }
#endif

    
    template<typename U> void internalAppend(U&& u);
    template<typename U, size_t O, class BP, class UV>
    void internalAppendAll(const VectorBase<U, O, BP, UV>& u);
    void internalAppendN(const T& t, size_t n);
    template<typename U> void internalAppend(const U* begin, size_t length);

  public:
    static const size_t sMaxInlineStorage = N;

    typedef T ElementType;

    VectorBase(AllocPolicy = AllocPolicy());
    VectorBase(ThisVector&&); 
    ThisVector& operator=(ThisVector&&); 
    ~VectorBase();

    

    const AllocPolicy& allocPolicy() const {
      return *this;
    }

    AllocPolicy& allocPolicy() {
      return *this;
    }

    enum { InlineLength = N };

    size_t length() const {
      return mLength;
    }

    bool empty() const {
      return mLength == 0;
    }

    size_t capacity() const {
      return mCapacity;
    }

    T* begin() {
      MOZ_ASSERT(!entered);
      return mBegin;
    }

    const T* begin() const {
      MOZ_ASSERT(!entered);
      return mBegin;
    }

    T* end() {
      MOZ_ASSERT(!entered);
      return mBegin + mLength;
    }

    const T* end() const {
      MOZ_ASSERT(!entered);
      return mBegin + mLength;
    }

    T& operator[](size_t i) {
      MOZ_ASSERT(!entered);
      MOZ_ASSERT(i < mLength);
      return begin()[i];
    }

    const T& operator[](size_t i) const {
      MOZ_ASSERT(!entered);
      MOZ_ASSERT(i < mLength);
      return begin()[i];
    }

    T& back() {
      MOZ_ASSERT(!entered);
      MOZ_ASSERT(!empty());
      return *(end() - 1);
    }

    const T& back() const {
      MOZ_ASSERT(!entered);
      MOZ_ASSERT(!empty());
      return *(end() - 1);
    }

    class Range
    {
        friend class VectorBase;
        T* cur_;
        T* end_;
        Range(T* cur, T* end) : cur_(cur), end_(end) {
          MOZ_ASSERT(cur <= end);
        }

      public:
        Range() {}
        bool empty() const { return cur_ == end_; }
        size_t remain() const { return PointerRangeSize(cur_, end_); }
        T& front() const { MOZ_ASSERT(!empty()); return *cur_; }
        void popFront() { MOZ_ASSERT(!empty()); ++cur_; }
        T popCopyFront() { MOZ_ASSERT(!empty()); return *cur_++; }
    };

    Range all() {
      return Range(begin(), end());
    }

    

    



    bool initCapacity(size_t request);

    



    bool reserve(size_t request);

    



    void shrinkBy(size_t incr);

    
    bool growBy(size_t incr);

    
    bool resize(size_t newLength);

    



    bool growByUninitialized(size_t incr);
    bool resizeUninitialized(size_t newLength);

    
    void clear();

    
    void clearAndFree();

    





    bool canAppendWithoutRealloc(size_t needed) const;

    

    




    template<typename U> bool append(U&& u);

    template<typename U, size_t O, class BP, class UV>
    bool appendAll(const VectorBase<U, O, BP, UV>& u);
    bool appendN(const T& t, size_t n);
    template<typename U> bool append(const U* begin, const U* end);
    template<typename U> bool append(const U* begin, size_t length);

    




    template<typename U> void infallibleAppend(U&& u) {
      internalAppend(Forward<U>(u));
    }
    void infallibleAppendN(const T& t, size_t n) {
      internalAppendN(t, n);
    }
    template<typename U> void infallibleAppend(const U* aBegin, const U* aEnd) {
      internalAppend(aBegin, PointerRangeSize(aBegin, aEnd));
    }
    template<typename U> void infallibleAppend(const U* aBegin, size_t aLength) {
      internalAppend(aBegin, aLength);
    }

    void popBack();

    T popCopy();

    









    T* extractRawBuffer();

    







    void replaceRawBuffer(T* p, size_t length);

    













    template<typename U>
    T* insert(T* p, U&& val);

    



    void erase(T* t);

    



    void erase(T* b, T *e);

    


    size_t sizeOfExcludingThis(MallocSizeOf mallocSizeOf) const;

    



    size_t sizeOfIncludingThis(MallocSizeOf mallocSizeOf) const;

    void swap(ThisVector& other);

  private:
    VectorBase(const VectorBase&) MOZ_DELETE;
    void operator=(const VectorBase&) MOZ_DELETE;

    
    VectorBase(VectorBase&&) MOZ_DELETE;
    void operator=(VectorBase&&) MOZ_DELETE;
};


#define MOZ_REENTRANCY_GUARD_ET_AL \
  ReentrancyGuard g(*this); \
  MOZ_ASSERT_IF(usingInlineStorage(), mCapacity == sInlineCapacity); \
  MOZ_ASSERT(reserved() <= mCapacity); \
  MOZ_ASSERT(mLength <= reserved()); \
  MOZ_ASSERT(mLength <= mCapacity)



template<typename T, size_t N, class AP, class TV>
MOZ_ALWAYS_INLINE
VectorBase<T, N, AP, TV>::VectorBase(AP ap)
  : AP(ap),
    mLength(0),
    mCapacity(sInlineCapacity)
#ifdef DEBUG
  , mReserved(sInlineCapacity),
    entered(false)
#endif
{
  mBegin = static_cast<T*>(storage.addr());
}


template<typename T, size_t N, class AllocPolicy, class TV>
MOZ_ALWAYS_INLINE
VectorBase<T, N, AllocPolicy, TV>::VectorBase(TV&& rhs)
  : AllocPolicy(Move(rhs))
#ifdef DEBUG
    , entered(false)
#endif
{
  mLength = rhs.mLength;
  mCapacity = rhs.mCapacity;
#ifdef DEBUG
  mReserved = rhs.mReserved;
#endif

  if (rhs.usingInlineStorage()) {
    
    mBegin = static_cast<T*>(storage.addr());
    Impl::moveConstruct(mBegin, rhs.beginNoCheck(), rhs.endNoCheck());
    



  } else {
    



    mBegin = rhs.mBegin;
    rhs.mBegin = static_cast<T*>(rhs.storage.addr());
    rhs.mCapacity = sInlineCapacity;
    rhs.mLength = 0;
#ifdef DEBUG
    rhs.mReserved = sInlineCapacity;
#endif
  }
}


template<typename T, size_t N, class AP, class TV>
MOZ_ALWAYS_INLINE
TV&
VectorBase<T, N, AP, TV>::operator=(TV&& rhs)
{
  MOZ_ASSERT(this != &rhs, "self-move assignment is prohibited");
  TV* tv = static_cast<TV*>(this);
  tv->~TV();
  new(tv) TV(Move(rhs));
  return *tv;
}

template<typename T, size_t N, class AP, class TV>
MOZ_ALWAYS_INLINE
VectorBase<T, N, AP, TV>::~VectorBase()
{
  MOZ_REENTRANCY_GUARD_ET_AL;
  Impl::destroy(beginNoCheck(), endNoCheck());
  if (!usingInlineStorage())
    this->free_(beginNoCheck());
}






template<typename T, size_t N, class AP, class TV>
inline bool
VectorBase<T, N, AP, TV>::convertToHeapStorage(size_t newCap)
{
  MOZ_ASSERT(usingInlineStorage());

  
  MOZ_ASSERT(!detail::CapacityHasExcessSpace<T>(newCap));
  T* newBuf = reinterpret_cast<T*>(this->malloc_(newCap * sizeof(T)));
  if (!newBuf)
    return false;

  
  Impl::moveConstruct(newBuf, beginNoCheck(), endNoCheck());
  Impl::destroy(beginNoCheck(), endNoCheck());

  
  mBegin = newBuf;
  
  mCapacity = newCap;
  return true;
}

template<typename T, size_t N, class AP, class TV>
MOZ_NEVER_INLINE bool
VectorBase<T, N, AP, TV>::growStorageBy(size_t incr)
{
  MOZ_ASSERT(mLength + incr > mCapacity);
  MOZ_ASSERT_IF(!usingInlineStorage(),
                !detail::CapacityHasExcessSpace<T>(mCapacity));

  







  size_t newCap;

  if (incr == 1) {
    if (usingInlineStorage()) {
      
      size_t newSize =
        tl::RoundUpPow2<(sInlineCapacity + 1) * sizeof(T)>::value;
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
    if (detail::CapacityHasExcessSpace<T>(newCap))
      newCap += 1;
  } else {
    
    size_t newMinCap = mLength + incr;

    
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
VectorBase<T, N, AP, TV>::initCapacity(size_t request)
{
  MOZ_ASSERT(empty());
  MOZ_ASSERT(usingInlineStorage());
  if (request == 0)
    return true;
  T* newbuf = reinterpret_cast<T*>(this->malloc_(request * sizeof(T)));
  if (!newbuf)
    return false;
  mBegin = newbuf;
  mCapacity = request;
#ifdef DEBUG
  mReserved = request;
#endif
  return true;
}

template<typename T, size_t N, class AP, class TV>
inline bool
VectorBase<T, N, AP, TV>::reserve(size_t request)
{
  MOZ_REENTRANCY_GUARD_ET_AL;
  if (request > mCapacity && !growStorageBy(request - mLength))
    return false;

#ifdef DEBUG
  if (request > mReserved)
    mReserved = request;
  MOZ_ASSERT(mLength <= mReserved);
  MOZ_ASSERT(mReserved <= mCapacity);
#endif
  return true;
}

template<typename T, size_t N, class AP, class TV>
inline void
VectorBase<T, N, AP, TV>::shrinkBy(size_t incr)
{
  MOZ_REENTRANCY_GUARD_ET_AL;
  MOZ_ASSERT(incr <= mLength);
  Impl::destroy(endNoCheck() - incr, endNoCheck());
  mLength -= incr;
}

template<typename T, size_t N, class AP, class TV>
MOZ_ALWAYS_INLINE bool
VectorBase<T, N, AP, TV>::growBy(size_t incr)
{
  MOZ_REENTRANCY_GUARD_ET_AL;
  if (incr > mCapacity - mLength && !growStorageBy(incr))
    return false;

  MOZ_ASSERT(mLength + incr <= mCapacity);
  T* newend = endNoCheck() + incr;
  Impl::initialize(endNoCheck(), newend);
  mLength += incr;
#ifdef DEBUG
  if (mLength > mReserved)
    mReserved = mLength;
#endif
  return true;
}

template<typename T, size_t N, class AP, class TV>
MOZ_ALWAYS_INLINE bool
VectorBase<T, N, AP, TV>::growByUninitialized(size_t incr)
{
  MOZ_REENTRANCY_GUARD_ET_AL;
  if (incr > mCapacity - mLength && !growStorageBy(incr))
    return false;

  MOZ_ASSERT(mLength + incr <= mCapacity);
  mLength += incr;
#ifdef DEBUG
  if (mLength > mReserved)
    mReserved = mLength;
#endif
  return true;
}

template<typename T, size_t N, class AP, class TV>
inline bool
VectorBase<T, N, AP, TV>::resize(size_t newLength)
{
  size_t curLength = mLength;
  if (newLength > curLength)
    return growBy(newLength - curLength);
  shrinkBy(curLength - newLength);
  return true;
}

template<typename T, size_t N, class AP, class TV>
MOZ_ALWAYS_INLINE bool
VectorBase<T, N, AP, TV>::resizeUninitialized(size_t newLength)
{
  size_t curLength = mLength;
  if (newLength > curLength)
    return growByUninitialized(newLength - curLength);
  shrinkBy(curLength - newLength);
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

  if (usingInlineStorage())
    return;

  this->free_(beginNoCheck());
  mBegin = static_cast<T*>(storage.addr());
  mCapacity = sInlineCapacity;
#ifdef DEBUG
  mReserved = sInlineCapacity;
#endif
}

template<typename T, size_t N, class AP, class TV>
inline bool
VectorBase<T, N, AP, TV>::canAppendWithoutRealloc(size_t needed) const
{
  return mLength + needed <= mCapacity;
}

template<typename T, size_t N, class AP, class TV>
template<typename U, size_t O, class BP, class UV>
MOZ_ALWAYS_INLINE void
VectorBase<T, N, AP, TV>::internalAppendAll(const VectorBase<U, O, BP, UV>& other)
{
  internalAppend(other.begin(), other.length());
}

template<typename T, size_t N, class AP, class TV>
template<typename U>
MOZ_ALWAYS_INLINE void
VectorBase<T, N, AP, TV>::internalAppend(U&& u)
{
  MOZ_ASSERT(mLength + 1 <= mReserved);
  MOZ_ASSERT(mReserved <= mCapacity);
  new(endNoCheck()) T(Forward<U>(u));
  ++mLength;
}

template<typename T, size_t N, class AP, class TV>
MOZ_ALWAYS_INLINE bool
VectorBase<T, N, AP, TV>::appendN(const T& t, size_t needed)
{
  MOZ_REENTRANCY_GUARD_ET_AL;
  if (mLength + needed > mCapacity && !growStorageBy(needed))
    return false;

#ifdef DEBUG
  if (mLength + needed > mReserved)
    mReserved = mLength + needed;
#endif
  internalAppendN(t, needed);
  return true;
}

template<typename T, size_t N, class AP, class TV>
MOZ_ALWAYS_INLINE void
VectorBase<T, N, AP, TV>::internalAppendN(const T& t, size_t needed)
{
  MOZ_ASSERT(mLength + needed <= mReserved);
  MOZ_ASSERT(mReserved <= mCapacity);
  Impl::copyConstructN(endNoCheck(), needed, t);
  mLength += needed;
}

template<typename T, size_t N, class AP, class TV>
template<typename U>
inline T*
VectorBase<T, N, AP, TV>::insert(T* p, U&& val)
{
  MOZ_ASSERT(begin() <= p);
  MOZ_ASSERT(p <= end());
  size_t pos = p - begin();
  MOZ_ASSERT(pos <= mLength);
  size_t oldLength = mLength;
  if (pos == oldLength) {
    if (!append(Forward<U>(val)))
      return nullptr;
  } else {
    T oldBack = Move(back());
    if (!append(Move(oldBack))) 
      return nullptr;
    for (size_t i = oldLength; i > pos; --i)
      (*this)[i] = Move((*this)[i - 1]);
    (*this)[pos] = Forward<U>(val);
  }
  return begin() + pos;
}

template<typename T, size_t N, class AP, class TV>
inline void
VectorBase<T, N, AP, TV>::erase(T* it)
{
  MOZ_ASSERT(begin() <= it);
  MOZ_ASSERT(it < end());
  while (it + 1 < end()) {
    *it = Move(*(it + 1));
    ++it;
  }
  popBack();
}

template<typename T, size_t N, class AP, class TV>
inline void
VectorBase<T, N, AP, TV>::erase(T* b, T *e)
{
  MOZ_ASSERT(begin() <= b);
  MOZ_ASSERT(b <= e);
  MOZ_ASSERT(e <= end());
  while (e < end())
    *b++ = Move(*e++);
  shrinkBy(e - b);
}

template<typename T, size_t N, class AP, class TV>
template<typename U>
MOZ_ALWAYS_INLINE bool
VectorBase<T, N, AP, TV>::append(const U* insBegin, const U* insEnd)
{
  MOZ_REENTRANCY_GUARD_ET_AL;
  size_t needed = PointerRangeSize(insBegin, insEnd);
  if (mLength + needed > mCapacity && !growStorageBy(needed))
    return false;

#ifdef DEBUG
  if (mLength + needed > mReserved)
    mReserved = mLength + needed;
#endif
  internalAppend(insBegin, needed);
  return true;
}

template<typename T, size_t N, class AP, class TV>
template<typename U>
MOZ_ALWAYS_INLINE void
VectorBase<T, N, AP, TV>::internalAppend(const U* insBegin, size_t insLength)
{
  MOZ_ASSERT(mLength + insLength <= mReserved);
  MOZ_ASSERT(mReserved <= mCapacity);
  Impl::copyConstruct(endNoCheck(), insBegin, insBegin + insLength);
  mLength += insLength;
}

template<typename T, size_t N, class AP, class TV>
template<typename U>
MOZ_ALWAYS_INLINE bool
VectorBase<T, N, AP, TV>::append(U&& u)
{
  MOZ_REENTRANCY_GUARD_ET_AL;
  if (mLength == mCapacity && !growStorageBy(1))
    return false;

#ifdef DEBUG
  if (mLength + 1 > mReserved)
    mReserved = mLength + 1;
#endif
  internalAppend(Forward<U>(u));
  return true;
}

template<typename T, size_t N, class AP, class TV>
template<typename U, size_t O, class BP, class UV>
MOZ_ALWAYS_INLINE bool
VectorBase<T, N, AP, TV>::appendAll(const VectorBase<U, O, BP, UV>& other)
{
  return append(other.begin(), other.length());
}

template<typename T, size_t N, class AP, class TV>
template<class U>
MOZ_ALWAYS_INLINE bool
VectorBase<T, N, AP, TV>::append(const U *insBegin, size_t insLength)
{
  return append(insBegin, insBegin + insLength);
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
    if (!ret)
      return nullptr;
    Impl::copyConstruct(ret, beginNoCheck(), endNoCheck());
    Impl::destroy(beginNoCheck(), endNoCheck());
    
    mLength = 0;
  } else {
    ret = mBegin;
    mBegin = static_cast<T*>(storage.addr());
    mLength = 0;
    mCapacity = sInlineCapacity;
#ifdef DEBUG
    mReserved = sInlineCapacity;
#endif
  }
  return ret;
}

template<typename T, size_t N, class AP, class TV>
inline void
VectorBase<T, N, AP, TV>::replaceRawBuffer(T* p, size_t aLength)
{
  MOZ_REENTRANCY_GUARD_ET_AL;

  
  Impl::destroy(beginNoCheck(), endNoCheck());
  if (!usingInlineStorage())
    this->free_(beginNoCheck());

  
  if (aLength <= sInlineCapacity) {
    




    mBegin = static_cast<T*>(storage.addr());
    mLength = aLength;
    mCapacity = sInlineCapacity;
    Impl::moveConstruct(mBegin, p, p + aLength);
    Impl::destroy(p, p + aLength);
    this->free_(p);
  } else {
    mBegin = p;
    mLength = aLength;
    mCapacity = aLength;
  }
#ifdef DEBUG
  mReserved = aLength;
#endif
}

template<typename T, size_t N, class AP, class TV>
inline size_t
VectorBase<T, N, AP, TV>::sizeOfExcludingThis(MallocSizeOf mallocSizeOf) const
{
  return usingInlineStorage() ? 0 : mallocSizeOf(beginNoCheck());
}

template<typename T, size_t N, class AP, class TV>
inline size_t
VectorBase<T, N, AP, TV>::sizeOfIncludingThis(MallocSizeOf mallocSizeOf) const
{
  return mallocSizeOf(this) + sizeOfExcludingThis(mallocSizeOf);
}

template<typename T, size_t N, class AP, class TV>
inline void
VectorBase<T, N, AP, TV>::swap(TV& other)
{
  static_assert(N == 0,
                "still need to implement this for N != 0");

  
  if (!usingInlineStorage() && other.usingInlineStorage()) {
    other.mBegin = mBegin;
    mBegin = inlineStorage();
  } else if (usingInlineStorage() && !other.usingInlineStorage()) {
    mBegin = other.mBegin;
    other.mBegin = other.inlineStorage();
  } else if (!usingInlineStorage() && !other.usingInlineStorage()) {
    Swap(mBegin, other.mBegin);
  } else {
    
  }

  Swap(mLength, other.mLength);
  Swap(mCapacity, other.mCapacity);
#ifdef DEBUG
  Swap(mReserved, other.mReserved);
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
    Vector(AllocPolicy alloc = AllocPolicy()) : Base(alloc) {}
    Vector(Vector&& vec) : Base(Move(vec)) {}
    Vector& operator=(Vector&& vec) {
      return Base::operator=(Move(vec));
    }
};

} 

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif 
