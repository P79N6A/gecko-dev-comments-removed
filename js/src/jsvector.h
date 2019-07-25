







































#ifndef jsvector_h_
#define jsvector_h_

#include "jsalloc.h"
#include "jstl.h"
#include "jsprvtd.h"


#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4345)
#endif

namespace js {





template <class T, size_t N, class AP, bool IsPod>
struct VectorImpl
{
    
    static inline void destroy(T *begin, T *end) {
        for (T *p = begin; p != end; ++p)
            p->~T();
    }

    
    static inline void initialize(T *begin, T *end) {
        for (T *p = begin; p != end; ++p)
            new(p) T();
    }

    



    template <class U>
    static inline void copyConstruct(T *dst, const U *srcbeg, const U *srcend) {
        for (const U *p = srcbeg; p != srcend; ++p, ++dst)
            new(dst) T(*p);
    }

    



    template <class U>
    static inline void copyConstructN(T *dst, size_t n, const U &u) {
        for (T *end = dst + n; dst != end; ++dst)
            new(dst) T(u);
    }

    





    static inline bool growTo(Vector<T,N,AP> &v, size_t newcap) {
        JS_ASSERT(!v.usingInlineStorage());
        T *newbuf = reinterpret_cast<T *>(v.malloc_(newcap * sizeof(T)));
        if (!newbuf)
            return false;
        for (T *dst = newbuf, *src = v.beginNoCheck(); src != v.endNoCheck(); ++dst, ++src)
            new(dst) T(*src);
        VectorImpl::destroy(v.beginNoCheck(), v.endNoCheck());
        v.free_(v.mBegin);
        v.mBegin = newbuf;
        
        v.mCapacity = newcap;
        return true;
    }
};






template <class T, size_t N, class AP>
struct VectorImpl<T, N, AP, true>
{
    static inline void destroy(T *, T *) {}

    static inline void initialize(T *begin, T *end) {
        







        for (T *p = begin; p != end; ++p)
            new(p) T();
    }

    template <class U>
    static inline void copyConstruct(T *dst, const U *srcbeg, const U *srcend) {
        






        for (const U *p = srcbeg; p != srcend; ++p, ++dst)
            *dst = *p;
    }

    static inline void copyConstructN(T *dst, size_t n, const T &t) {
        for (T *p = dst, *end = dst + n; p != end; ++p)
            *p = t;
    }

    static inline bool growTo(Vector<T,N,AP> &v, size_t newcap) {
        JS_ASSERT(!v.usingInlineStorage());
        size_t bytes = sizeof(T) * newcap;
        size_t oldBytes = sizeof(T) * v.mCapacity;
        T *newbuf = reinterpret_cast<T *>(v.realloc_(v.mBegin, oldBytes, bytes));
        if (!newbuf)
            return false;
        v.mBegin = newbuf;
        
        v.mCapacity = newcap;
        return true;
    }
};



















template <class T, size_t N, class AllocPolicy>
class Vector : private AllocPolicy
{
    

    static const bool sElemIsPod = tl::IsPodType<T>::result;
    typedef VectorImpl<T, N, AllocPolicy, sElemIsPod> Impl;
    friend struct VectorImpl<T, N, AllocPolicy, sElemIsPod>;

    bool calculateNewCapacity(size_t curLength, size_t lengthInc, size_t &newCap);
    bool growStorageBy(size_t lengthInc);
    bool growHeapStorageBy(size_t lengthInc);
    bool convertToHeapStorage(size_t lengthInc);

    template <bool InitNewElems> inline bool growByImpl(size_t inc);

    

    static const int sMaxInlineBytes = 1024;

    

    








    template <int M, int Dummy>
    struct ElemSize {
        static const size_t result = sizeof(T);
    };
    template <int Dummy>
    struct ElemSize<0, Dummy> {
        static const size_t result = 1;
    };

    static const size_t sInlineCapacity =
        tl::Min<N, sMaxInlineBytes / ElemSize<N, 0>::result>::result;

    
    static const size_t sInlineBytes =
        tl::Max<1, sInlineCapacity * ElemSize<N, 0>::result>::result;

    

    






    T *mBegin;
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

    Vector(const Vector &);
    Vector &operator=(const Vector &);

    

    bool usingInlineStorage() const {
        return mBegin == (T *)storage.addr();
    }

    T *beginNoCheck() const {
        return mBegin;
    }

    T *endNoCheck() {
        return mBegin + mLength;
    }

    const T *endNoCheck() const {
        return mBegin + mLength;
    }

#ifdef DEBUG
    size_t reserved() const {
        JS_ASSERT(mReserved <= mCapacity);
        JS_ASSERT(mLength <= mReserved);
        return mReserved;
    }
#endif

    
    void internalAppend(const T &t);
    void internalAppendN(const T &t, size_t n);
    template <class U> void internalAppend(const U *begin, size_t length);
    template <class U, size_t O, class BP> void internalAppend(const Vector<U,O,BP> &other);

  public:
    static const size_t sMaxInlineStorage = N;

    typedef T ElementType;

    Vector(AllocPolicy = AllocPolicy());
    ~Vector();

    

    const AllocPolicy &allocPolicy() const {
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

    T *begin() const {
        JS_ASSERT(!entered);
        return mBegin;
    }

    T *end() {
        JS_ASSERT(!entered);
        return mBegin + mLength;
    }

    const T *end() const {
        JS_ASSERT(!entered);
        return mBegin + mLength;
    }

    T &operator[](size_t i) {
        JS_ASSERT(!entered && i < mLength);
        return begin()[i];
    }

    const T &operator[](size_t i) const {
        JS_ASSERT(!entered && i < mLength);
        return begin()[i];
    }

    T &back() {
        JS_ASSERT(!entered && !empty());
        return *(end() - 1);
    }

    const T &back() const {
        JS_ASSERT(!entered && !empty());
        return *(end() - 1);
    }

    

    
    bool reserve(size_t capacity);

    



    void shrinkBy(size_t incr);

    
    bool growBy(size_t incr);

    
    bool resize(size_t newLength);

    
    bool growByUninitialized(size_t incr);
    bool resizeUninitialized(size_t newLength);

    
    void clear();

    
    void clearAndFree();

    
    bool append(const T &t);
    bool appendN(const T &t, size_t n);
    template <class U> bool append(const U *begin, const U *end);
    template <class U> bool append(const U *begin, size_t length);
    template <class U, size_t O, class BP> bool append(const Vector<U,O,BP> &other);

    



    void infallibleAppend(const T &t) {
        internalAppend(t);
    }
    void infallibleAppendN(const T &t, size_t n) {
        internalAppendN(t, n);
    }
    template <class U> void infallibleAppend(const U *begin, const U *end) {
        internalAppend(begin, PointerRangeSize(begin, end));
    }
    template <class U> void infallibleAppend(const U *begin, size_t length) {
        internalAppend(begin, length);
    }
    template <class U, size_t O, class BP> void infallibleAppend(const Vector<U,O,BP> &other) {
        internalAppend(other);
    }

    void popBack();

    T popCopy();

    







    T *extractRawBuffer();

    




    void replaceRawBuffer(T *p, size_t length);

    



    bool insert(T *p, const T &val);

    



    void erase(T *t);
};


#define REENTRANCY_GUARD_ET_AL \
    ReentrancyGuard g(*this); \
    JS_ASSERT_IF(usingInlineStorage(), mCapacity == sInlineCapacity); \
    JS_ASSERT(reserved() <= mCapacity); \
    JS_ASSERT(mLength <= reserved()); \
    JS_ASSERT(mLength <= mCapacity)



template <class T, size_t N, class AllocPolicy>
JS_ALWAYS_INLINE
Vector<T,N,AllocPolicy>::Vector(AllocPolicy ap)
  : AllocPolicy(ap), mBegin((T *)storage.addr()), mLength(0),
    mCapacity(sInlineCapacity)
#ifdef DEBUG
  , mReserved(0), entered(false)
#endif
{}

template <class T, size_t N, class AP>
JS_ALWAYS_INLINE
Vector<T,N,AP>::~Vector()
{
    REENTRANCY_GUARD_ET_AL;
    Impl::destroy(beginNoCheck(), endNoCheck());
    if (!usingInlineStorage())
        this->free_(beginNoCheck());
}





template <class T, size_t N, class AP>
STATIC_POSTCONDITION(!return || newCap >= curLength + lengthInc)
inline bool
Vector<T,N,AP>::calculateNewCapacity(size_t curLength, size_t lengthInc,
                                     size_t &newCap)
{
    size_t newMinCap = curLength + lengthInc;

    



    if (newMinCap < curLength ||
        newMinCap & tl::MulOverflowMask<2 * sizeof(T)>::result) {
        this->reportAllocOverflow();
        return false;
    }

    
    newCap = RoundUpPow2(newMinCap);

    



    if (newCap & tl::UnsafeRangeSizeMask<T>::result) {
        this->reportAllocOverflow();
        return false;
    }
    return true;
}





template <class T, size_t N, class AP>
JS_ALWAYS_INLINE bool
Vector<T,N,AP>::growHeapStorageBy(size_t lengthInc)
{
    JS_ASSERT(!usingInlineStorage());
    size_t newCap;
    return calculateNewCapacity(mLength, lengthInc, newCap) &&
           Impl::growTo(*this, newCap);
}






template <class T, size_t N, class AP>
inline bool
Vector<T,N,AP>::convertToHeapStorage(size_t lengthInc)
{
    JS_ASSERT(usingInlineStorage());
    size_t newCap;
    if (!calculateNewCapacity(mLength, lengthInc, newCap))
        return false;

    
    T *newBuf = reinterpret_cast<T *>(this->malloc_(newCap * sizeof(T)));
    if (!newBuf)
        return false;

    
    Impl::copyConstruct(newBuf, beginNoCheck(), endNoCheck());
    Impl::destroy(beginNoCheck(), endNoCheck());

    
    mBegin = newBuf;
    
    mCapacity = newCap;
    return true;
}

template <class T, size_t N, class AP>
JS_NEVER_INLINE bool
Vector<T,N,AP>::growStorageBy(size_t incr)
{
    JS_ASSERT(mLength + incr > mCapacity);
    return usingInlineStorage()
         ? convertToHeapStorage(incr)
         : growHeapStorageBy(incr);
}

template <class T, size_t N, class AP>
inline bool
Vector<T,N,AP>::reserve(size_t request)
{
    REENTRANCY_GUARD_ET_AL;
    if (request <= mCapacity || growStorageBy(request - mLength)) {
#ifdef DEBUG
        if (request > mReserved)
            mReserved = request;
        JS_ASSERT(mLength <= mReserved);
        JS_ASSERT(mReserved <= mCapacity);
#endif
        return true;
    }
    return false;
}

template <class T, size_t N, class AP>
inline void
Vector<T,N,AP>::shrinkBy(size_t incr)
{
    REENTRANCY_GUARD_ET_AL;
    JS_ASSERT(incr <= mLength);
    Impl::destroy(endNoCheck() - incr, endNoCheck());
    mLength -= incr;
}

template <class T, size_t N, class AP>
template <bool InitNewElems>
JS_ALWAYS_INLINE bool
Vector<T,N,AP>::growByImpl(size_t incr)
{
    REENTRANCY_GUARD_ET_AL;
    if (incr > mCapacity - mLength && !growStorageBy(incr))
        return false;

    JS_ASSERT(mLength + incr <= mCapacity);
    T *newend = endNoCheck() + incr;
    if (InitNewElems)
        Impl::initialize(endNoCheck(), newend);
    mLength += incr;
#ifdef DEBUG
    if (mLength > mReserved)
        mReserved = mLength;
#endif
    return true;
}

template <class T, size_t N, class AP>
JS_ALWAYS_INLINE bool
Vector<T,N,AP>::growBy(size_t incr)
{
    return growByImpl<true>(incr);
}

template <class T, size_t N, class AP>
JS_ALWAYS_INLINE bool
Vector<T,N,AP>::growByUninitialized(size_t incr)
{
    return growByImpl<false>(incr);
}

template <class T, size_t N, class AP>
STATIC_POSTCONDITION(!return || ubound(this->begin()) >= newLength)
inline bool
Vector<T,N,AP>::resize(size_t newLength)
{
    size_t curLength = mLength;
    if (newLength > curLength)
        return growBy(newLength - curLength);
    shrinkBy(curLength - newLength);
    return true;
}

template <class T, size_t N, class AP>
JS_ALWAYS_INLINE bool
Vector<T,N,AP>::resizeUninitialized(size_t newLength)
{
    size_t curLength = mLength;
    if (newLength > curLength)
        return growByUninitialized(newLength - curLength);
    shrinkBy(curLength - newLength);
    return true;
}

template <class T, size_t N, class AP>
inline void
Vector<T,N,AP>::clear()
{
    REENTRANCY_GUARD_ET_AL;
    Impl::destroy(beginNoCheck(), endNoCheck());
    mLength = 0;
}

template <class T, size_t N, class AP>
inline void
Vector<T,N,AP>::clearAndFree()
{
    clear();

    if (usingInlineStorage())
        return;

    this->free_(beginNoCheck());
    mBegin = (T *)storage.addr();
    mCapacity = sInlineCapacity;
#ifdef DEBUG
    mReserved = 0;
#endif
}

template <class T, size_t N, class AP>
JS_ALWAYS_INLINE bool
Vector<T,N,AP>::append(const T &t)
{
    REENTRANCY_GUARD_ET_AL;
    if (mLength == mCapacity && !growStorageBy(1))
        return false;

#ifdef DEBUG
    if (mLength + 1 > mReserved)
        mReserved = mLength + 1;
#endif
    internalAppend(t);
    return true;
}

template <class T, size_t N, class AP>
JS_ALWAYS_INLINE void
Vector<T,N,AP>::internalAppend(const T &t)
{
    JS_ASSERT(mLength + 1 <= mReserved);
    JS_ASSERT(mReserved <= mCapacity);
    new(endNoCheck()) T(t);
    ++mLength;
}

template <class T, size_t N, class AP>
JS_ALWAYS_INLINE bool
Vector<T,N,AP>::appendN(const T &t, size_t needed)
{
    REENTRANCY_GUARD_ET_AL;
    if (mLength + needed > mCapacity && !growStorageBy(needed))
        return false;

#ifdef DEBUG
    if (mLength + needed > mReserved)
        mReserved = mLength + needed;
#endif
    internalAppendN(t, needed);
    return true;
}

template <class T, size_t N, class AP>
JS_ALWAYS_INLINE void
Vector<T,N,AP>::internalAppendN(const T &t, size_t needed)
{
    JS_ASSERT(mLength + needed <= mReserved);
    JS_ASSERT(mReserved <= mCapacity);
    Impl::copyConstructN(endNoCheck(), needed, t);
    mLength += needed;
}

template <class T, size_t N, class AP>
inline bool
Vector<T,N,AP>::insert(T *p, const T &val)
{
    JS_ASSERT(begin() <= p && p <= end());
    size_t pos = p - begin();
    JS_ASSERT(pos <= mLength);
    size_t oldLength = mLength;
    if (pos == oldLength)
        return append(val);
    {
        T oldBack = back();
        if (!append(oldBack)) 
            return false;
    }
    for (size_t i = oldLength; i > pos; --i)
        (*this)[i] = (*this)[i - 1];
    (*this)[pos] = val;
    return true;
}

template<typename T, size_t N, class AP>
inline void
Vector<T,N,AP>::erase(T *it)
{
    JS_ASSERT(begin() <= it && it < end());
    while (it + 1 != end()) {
        *it = *(it + 1);
        ++it;
    }
    popBack();
}

template <class T, size_t N, class AP>
template <class U>
JS_ALWAYS_INLINE bool
Vector<T,N,AP>::append(const U *insBegin, const U *insEnd)
{
    REENTRANCY_GUARD_ET_AL;
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

template <class T, size_t N, class AP>
template <class U>
JS_ALWAYS_INLINE void
Vector<T,N,AP>::internalAppend(const U *insBegin, size_t length)
{
    JS_ASSERT(mLength + length <= mReserved);
    JS_ASSERT(mReserved <= mCapacity);
    Impl::copyConstruct(endNoCheck(), insBegin, insBegin + length);
    mLength += length;
}

template <class T, size_t N, class AP>
template <class U, size_t O, class BP>
inline bool
Vector<T,N,AP>::append(const Vector<U,O,BP> &other)
{
    return append(other.begin(), other.end());
}

template <class T, size_t N, class AP>
template <class U, size_t O, class BP>
inline void
Vector<T,N,AP>::internalAppend(const Vector<U,O,BP> &other)
{
    internalAppend(other.begin(), other.length());
}

template <class T, size_t N, class AP>
template <class U>
JS_ALWAYS_INLINE bool
Vector<T,N,AP>::append(const U *insBegin, size_t length)
{
    return this->append(insBegin, insBegin + length);
}

template <class T, size_t N, class AP>
JS_ALWAYS_INLINE void
Vector<T,N,AP>::popBack()
{
    REENTRANCY_GUARD_ET_AL;
    JS_ASSERT(!empty());
    --mLength;
    endNoCheck()->~T();
}

template <class T, size_t N, class AP>
JS_ALWAYS_INLINE T
Vector<T,N,AP>::popCopy()
{
    T ret = back();
    popBack();
    return ret;
}

template <class T, size_t N, class AP>
inline T *
Vector<T,N,AP>::extractRawBuffer()
{
    T *ret;
    if (usingInlineStorage()) {
        ret = reinterpret_cast<T *>(this->malloc_(mLength * sizeof(T)));
        if (!ret)
            return NULL;
        Impl::copyConstruct(ret, beginNoCheck(), endNoCheck());
        Impl::destroy(beginNoCheck(), endNoCheck());
        
        mLength = 0;
    } else {
        ret = mBegin;
        mBegin = (T *)storage.addr();
        mLength = 0;
        mCapacity = sInlineCapacity;
#ifdef DEBUG
        mReserved = 0;
#endif
    }
    return ret;
}

template <class T, size_t N, class AP>
inline void
Vector<T,N,AP>::replaceRawBuffer(T *p, size_t length)
{
    REENTRANCY_GUARD_ET_AL;

    
    Impl::destroy(beginNoCheck(), endNoCheck());
    if (!usingInlineStorage())
        this->free_(beginNoCheck());

    
    if (length <= sInlineCapacity) {
        




        mBegin = (T *)storage.addr();
        mLength = length;
        mCapacity = sInlineCapacity;
        Impl::copyConstruct(mBegin, p, p + length);
        Impl::destroy(p, p + length);
        this->free_(p);
    } else {
        mBegin = p;
        mLength = length;
        mCapacity = length;
    }
#ifdef DEBUG
    mReserved = length;
#endif
}

}  

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif 
