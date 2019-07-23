






































#ifndef jsvector_h_
#define jsvector_h_

#include <string.h>
#include <new>

#include "jsbit.h"


namespace JSUtils {


template <size_t i, size_t j> struct min {
    static const size_t result = i < j ? i : j;
};
template <size_t i, size_t j> struct max {
    static const size_t result = i > j ? i : j;
};


template <size_t i> struct FloorLog2 {
    static const size_t result = 1 + FloorLog2<i / 2>::result;
};
template <> struct FloorLog2<0> {  };
template <> struct FloorLog2<1> { static const size_t result = 0; };


template <size_t i> struct CeilingLog2 {
    static const size_t result = FloorLog2<2 * i - 1>::result;
};


template <class T> struct BitSize {
    static const size_t result = sizeof(T) * JS_BITS_PER_BYTE;
};


template <bool> struct StaticAssert {};
template <> struct StaticAssert<true> { typedef int result; };





template <size_t N> struct NBitMask {
    typedef typename StaticAssert<N < BitSize<size_t>::result>::result _;
    static const size_t result = ~((size_t(1) << N) - 1);
};
template <> struct NBitMask<BitSize<size_t>::result> {
    static const size_t result = size_t(-1);
};





template <size_t N> struct MulOverflowMask {
    static const size_t result =
        NBitMask<BitSize<size_t>::result - CeilingLog2<N>::result>::result;
};
template <> struct MulOverflowMask<0> {  };
template <> struct MulOverflowMask<1> { static const size_t result = 0; };







template <class T>
size_t JS_ALWAYS_INLINE
PointerRangeSize(T *begin, T *end) {
    return (size_t(end) - size_t(begin)) / sizeof(T);
}






template <class T> struct UnsafeRangeSizeMask {
    



    static const size_t result = MulOverflowMask<2 * sizeof(T)>::result;
};





template <class T> struct IsPodType           { static const bool result = false; };
template <> struct IsPodType<char>            { static const bool result = true; };
template <> struct IsPodType<signed char>     { static const bool result = true; };
template <> struct IsPodType<unsigned char>   { static const bool result = true; };
template <> struct IsPodType<short>           { static const bool result = true; };
template <> struct IsPodType<unsigned short>  { static const bool result = true; };
template <> struct IsPodType<int>             { static const bool result = true; };
template <> struct IsPodType<unsigned int>    { static const bool result = true; };
template <> struct IsPodType<long>            { static const bool result = true; };
template <> struct IsPodType<unsigned long>   { static const bool result = true; };
template <> struct IsPodType<float>           { static const bool result = true; };
template <> struct IsPodType<double>          { static const bool result = true; };

} 





template <class T, size_t N, bool IsPod>
struct JSTempVectorImpl
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

    





    static inline bool growTo(JSTempVector<T> &v, size_t newcap) {
        JS_ASSERT(!v.usingInlineStorage());
        T *newbuf = reinterpret_cast<T *>(v.mCx->malloc(newcap * sizeof(T)));
        if (!newbuf)
            return false;
        for (T *dst = newbuf, *src = v.heapBegin(); src != v.heapEnd(); ++dst, ++src)
            new(dst) T(*src);
        JSTempVectorImpl::destroy(v.heapBegin(), v.heapEnd());
        v.mCx->free(v.heapBegin());
        v.heapEnd() = newbuf + v.heapLength();
        v.heapBegin() = newbuf;
        v.heapCapacity() = newcap;
        return true;
    }
};






template <class T, size_t N>
struct JSTempVectorImpl<T, N, true>
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
        for (T *end = dst + n; dst != end; ++dst)
            *dst = t;
    }

    static inline bool growTo(JSTempVector<T,N> &v, size_t newcap) {
        JS_ASSERT(!v.usingInlineStorage());
        size_t bytes = sizeof(T) * newcap;
        T *newbuf = reinterpret_cast<T *>(v.mCx->realloc(v.heapBegin(), bytes));
        if (!newbuf)
            return false;
        v.heapEnd() = newbuf + v.heapLength();
        v.heapBegin() = newbuf;
        v.heapCapacity() = newcap;
        return true;
    }
};
















template <class T, size_t N>
class JSTempVector
{
    

    typedef JSTempVectorImpl<T, N, JSUtils::IsPodType<T>::result> Impl;
    friend struct JSTempVectorImpl<T, N, JSUtils::IsPodType<T>::result>;

    bool calculateNewCapacity(size_t curLength, size_t lengthInc, size_t &newCap);
    bool growHeapStorageBy(size_t lengthInc);
    bool convertToHeapStorage(size_t lengthInc);

    

    static const int sMaxInlineBytes = 1024;

    

    




    struct BufferPtrs {
        T *mBegin, *mEnd;
    };

    







    static const size_t sInlineCapacity =
        JSUtils::min<JSUtils::max<N, sizeof(BufferPtrs) / sizeof(T)>::result,
                     sMaxInlineBytes / sizeof(T)>::result;

    

    JSContext *mCx;

    size_t mLengthOrCapacity;
    bool usingInlineStorage() const { return mLengthOrCapacity <= sInlineCapacity; }

    union {
        BufferPtrs ptrs;
        char mBuf[sInlineCapacity * sizeof(T)];
    } u;

    
    size_t &inlineLength() {
        JS_ASSERT(usingInlineStorage());
        return mLengthOrCapacity;
    }

    size_t inlineLength() const {
        JS_ASSERT(usingInlineStorage());
        return mLengthOrCapacity;
    }

    T *inlineBegin() const {
        JS_ASSERT(usingInlineStorage());
        return (T *)u.mBuf;
    }

    T *inlineEnd() const {
        JS_ASSERT(usingInlineStorage());
        return ((T *)u.mBuf) + mLengthOrCapacity;
    }

    
    size_t heapLength() const {
        JS_ASSERT(!usingInlineStorage());
        
        JS_ASSERT(size_t(u.ptrs.mEnd - u.ptrs.mBegin) ==
                  ((size_t(u.ptrs.mEnd) - size_t(u.ptrs.mBegin)) / sizeof(T)));
        return u.ptrs.mEnd - u.ptrs.mBegin;
    }

    size_t &heapCapacity() {
        JS_ASSERT(!usingInlineStorage());
        return mLengthOrCapacity;
    }

    T *&heapBegin() {
        JS_ASSERT(!usingInlineStorage());
        return u.ptrs.mBegin;
    }

    T *&heapEnd() {
        JS_ASSERT(!usingInlineStorage());
        return u.ptrs.mEnd;
    }

    size_t heapCapacity() const {
        JS_ASSERT(!usingInlineStorage());
        return mLengthOrCapacity;
    }

    T *heapBegin() const {
        JS_ASSERT(!usingInlineStorage());
        return u.ptrs.mBegin;
    }

    T *heapEnd() const {
        JS_ASSERT(!usingInlineStorage());
        return u.ptrs.mEnd;
    }

#ifdef DEBUG
    bool mInProgress;
#endif

    class ReentrancyGuard {
        JSTempVector &mVec;
      public:
        ReentrancyGuard(JSTempVector &v)
          : mVec(v)
        {
#ifdef DEBUG
            JS_ASSERT(!mVec.mInProgress);
            mVec.mInProgress = true;
#endif
        }
        ~ReentrancyGuard()
        {
#ifdef DEBUG
            mVec.mInProgress = false;
#endif
        }
    };

    JSTempVector(const JSTempVector &);
    JSTempVector &operator=(const JSTempVector &);

  public:
    JSTempVector(JSContext *cx)
      : mCx(cx), mLengthOrCapacity(0)
#ifdef DEBUG
        , mInProgress(false)
#endif
    {}
    ~JSTempVector();

    

    size_t length() const {
        return usingInlineStorage() ? inlineLength() : heapLength();
    }

    bool empty() const {
        return usingInlineStorage() ? inlineLength() == 0 : heapBegin() == heapEnd();
    }

    size_t capacity() const {
        return usingInlineStorage() ? sInlineCapacity : heapCapacity();
    }

    T *begin() {
        JS_ASSERT(!mInProgress);
        return usingInlineStorage() ? inlineBegin() : heapBegin();
    }

    const T *begin() const {
        JS_ASSERT(!mInProgress);
        return usingInlineStorage() ? inlineBegin() : heapBegin();
    }

    T *end() {
        JS_ASSERT(!mInProgress);
        return usingInlineStorage() ? inlineEnd() : heapEnd();
    }

    const T *end() const {
        JS_ASSERT(!mInProgress);
        return usingInlineStorage() ? inlineEnd() : heapEnd();
    }

    T &operator[](size_t i) {
        JS_ASSERT(!mInProgress && i < length());
        return begin()[i];
    }

    const T &operator[](size_t i) const {
        JS_ASSERT(!mInProgress && i < length());
        return begin()[i];
    }

    T &back() {
        JS_ASSERT(!mInProgress && !empty());
        return *(end() - 1);
    }

    const T &back() const {
        JS_ASSERT(!mInProgress && !empty());
        return *(end() - 1);
    }

    

    
    bool reserve(size_t capacity);

    
    void shrinkBy(size_t incr);

    




    bool growBy(size_t incr);

    
    bool resize(size_t newLength);

    void clear();

    bool append(const T &t);
    bool appendN(const T &t, size_t n);
    template <class U> bool append(const U *begin, const U *end);
    template <class U> bool append(const U *begin, size_t length);

    void popBack();

    







    T *extractRawBuffer();

    




    void replaceRawBuffer(T *p, size_t length);
};








template <class T, size_t N, size_t ArrayLength>
bool
js_AppendLiteral(JSTempVector<T,N> &v, const char (&array)[ArrayLength])
{
    return v.append(array, array + ArrayLength - 1);
}




template <class T, size_t N>
inline
JSTempVector<T,N>::~JSTempVector()
{
    ReentrancyGuard g(*this);
    if (usingInlineStorage()) {
        Impl::destroy(inlineBegin(), inlineEnd());
    } else {
        Impl::destroy(heapBegin(), heapEnd());
        mCx->free(heapBegin());
    }
}





template <class T, size_t N>
inline bool
JSTempVector<T,N>::calculateNewCapacity(size_t curLength, size_t lengthInc,
                                        size_t &newCap)
{
    size_t newMinCap = curLength + lengthInc;

    



    if (newMinCap < curLength ||
        newMinCap & JSUtils::MulOverflowMask<2 * sizeof(T)>::result) {
        js_ReportAllocationOverflow(mCx);
        return false;
    }

    
    size_t newCapLog2;
    JS_CEILING_LOG2(newCapLog2, newMinCap);
    JS_ASSERT(newCapLog2 < JSUtils::BitSize<size_t>::result);
    newCap = size_t(1) << newCapLog2;

    



    if (newCap & JSUtils::UnsafeRangeSizeMask<T>::result) {
        js_ReportAllocationOverflow(mCx);
        return false;
    }
    return true;
}





template <class T, size_t N>
inline bool
JSTempVector<T,N>::growHeapStorageBy(size_t lengthInc)
{
    size_t newCap;
    return calculateNewCapacity(heapLength(), lengthInc, newCap) &&
           Impl::growTo(*this, newCap);
}






template <class T, size_t N>
inline bool
JSTempVector<T,N>::convertToHeapStorage(size_t lengthInc)
{
    size_t newCap;
    if (!calculateNewCapacity(inlineLength(), lengthInc, newCap))
        return false;

    
    T *newBuf = reinterpret_cast<T *>(mCx->malloc(newCap * sizeof(T)));
    if (!newBuf)
        return false;

    
    size_t length = inlineLength();
    Impl::copyConstruct(newBuf, inlineBegin(), inlineEnd());
    Impl::destroy(inlineBegin(), inlineEnd());

    
    mLengthOrCapacity = newCap;  
    heapBegin() = newBuf;
    heapEnd() = newBuf + length;
    return true;
}

template <class T, size_t N>
inline bool
JSTempVector<T,N>::reserve(size_t request)
{
    ReentrancyGuard g(*this);
    if (usingInlineStorage()) {
        if (request > sInlineCapacity)
            return convertToHeapStorage(request - inlineLength());
    } else {
        if (request > heapCapacity())
            return growHeapStorageBy(request - heapLength());
    }
    return true;
}

template <class T, size_t N>
inline void
JSTempVector<T,N>::shrinkBy(size_t incr)
{
    ReentrancyGuard g(*this);
    JS_ASSERT(incr <= length());
    if (usingInlineStorage()) {
        Impl::destroy(inlineEnd() - incr, inlineEnd());
        inlineLength() -= incr;
    } else {
        Impl::destroy(heapEnd() - incr, heapEnd());
        heapEnd() -= incr;
    }
}

template <class T, size_t N>
inline bool
JSTempVector<T,N>::growBy(size_t incr)
{
    ReentrancyGuard g(*this);
    if (usingInlineStorage()) {
        size_t freespace = sInlineCapacity - inlineLength();
        if (incr <= freespace) {
            T *newend = inlineEnd() + incr;
            if (!JSUtils::IsPodType<T>::result)
                Impl::initialize(inlineEnd(), newend);
            inlineLength() += incr;
            JS_ASSERT(usingInlineStorage());
            return true;
        }
        if (!convertToHeapStorage(incr))
            return false;
    }
    else {
        
        size_t freespace = heapCapacity() - heapLength();
        if (incr > freespace) {
            if (!growHeapStorageBy(incr))
                return false;
        }
    }

    
    JS_ASSERT(heapCapacity() - heapLength() >= incr);
    T *newend = heapEnd() + incr;
    if (!JSUtils::IsPodType<T>::result)
        Impl::initialize(heapEnd(), newend);
    heapEnd() = newend;
    return true;
}

template <class T, size_t N>
inline bool
JSTempVector<T,N>::resize(size_t newLength)
{
    size_t curLength = length();
    if (newLength > curLength)
        return growBy(newLength - curLength);
    shrinkBy(curLength - newLength);
    return true;
}

template <class T, size_t N>
inline void
JSTempVector<T,N>::clear()
{
    ReentrancyGuard g(*this);
    if (usingInlineStorage()) {
        Impl::destroy(inlineBegin(), inlineEnd());
        inlineLength() = 0;
    }
    else {
        Impl::destroy(heapBegin(), heapEnd());
        heapEnd() = heapBegin();
    }
}

template <class T, size_t N>
inline bool
JSTempVector<T,N>::append(const T &t)
{
    ReentrancyGuard g(*this);
    if (usingInlineStorage()) {
        if (inlineLength() < sInlineCapacity) {
            new(inlineEnd()) T(t);
            ++inlineLength();
            JS_ASSERT(usingInlineStorage());
            return true;
        }
        if (!convertToHeapStorage(1))
            return false;
    } else {
        if (heapLength() == heapCapacity() && !growHeapStorageBy(1))
            return false;
    }

    
    JS_ASSERT(heapLength() <= heapCapacity() && heapCapacity() - heapLength() >= 1);
    new(heapEnd()++) T(t);
    return true;
}

template <class T, size_t N>
inline bool
JSTempVector<T,N>::appendN(const T &t, size_t needed)
{
    ReentrancyGuard g(*this);
    if (usingInlineStorage()) {
        size_t freespace = sInlineCapacity - inlineLength();
        if (needed <= freespace) {
            Impl::copyConstructN(inlineEnd(), needed, t);
            inlineLength() += needed;
            JS_ASSERT(usingInlineStorage());
            return true;
        }
        if (!convertToHeapStorage(needed))
            return false;
    } else {
        size_t freespace = heapCapacity() - heapLength();
        if (needed > freespace && !growHeapStorageBy(needed))
            return false;
    }

    
    JS_ASSERT(heapLength() <= heapCapacity() && heapCapacity() - heapLength() >= needed);
    Impl::copyConstructN(heapEnd(), needed, t);
    heapEnd() += needed;
    return true;
}

template <class T, size_t N>
template <class U>
inline bool
JSTempVector<T,N>::append(const U *insBegin, const U *insEnd)
{
    ReentrancyGuard g(*this);
    size_t needed = JSUtils::PointerRangeSize(insBegin, insEnd);
    if (usingInlineStorage()) {
        size_t freespace = sInlineCapacity - inlineLength();
        if (needed <= freespace) {
            Impl::copyConstruct(inlineEnd(), insBegin, insEnd);
            inlineLength() += needed;
            JS_ASSERT(usingInlineStorage());
            return true;
        }
        if (!convertToHeapStorage(needed))
            return false;
    } else {
        size_t freespace = heapCapacity() - heapLength();
        if (needed > freespace && !growHeapStorageBy(needed))
            return false;
    }

    
    JS_ASSERT(heapLength() <= heapCapacity() && heapCapacity() - heapLength() >= needed);
    Impl::copyConstruct(heapEnd(), insBegin, insEnd);
    heapEnd() += needed;
    return true;
}

template <class T, size_t N>
template <class U>
inline bool
JSTempVector<T,N>::append(const U *insBegin, size_t length)
{
    return this->append(insBegin, insBegin + length);
}

template <class T, size_t N>
inline void
JSTempVector<T,N>::popBack()
{
    ReentrancyGuard g(*this);
    JS_ASSERT(!empty());
    if (usingInlineStorage()) {
        --inlineLength();
        inlineEnd()->~T();
    } else {
        --heapEnd();
        heapEnd()->~T();
    }
}

template <class T, size_t N>
inline T *
JSTempVector<T,N>::extractRawBuffer()
{
    if (usingInlineStorage()) {
        T *ret = reinterpret_cast<T *>(mCx->malloc(inlineLength() * sizeof(T)));
        if (!ret)
            return NULL;
        Impl::copyConstruct(ret, inlineBegin(), inlineEnd());
        Impl::destroy(inlineBegin(), inlineEnd());
        inlineLength() = 0;
        return ret;
    }

    T *ret = heapBegin();
    mLengthOrCapacity = 0;  
    return ret;
}

template <class T, size_t N>
inline void
JSTempVector<T,N>::replaceRawBuffer(T *p, size_t length)
{
    ReentrancyGuard g(*this);

    
    if (usingInlineStorage()) {
        Impl::destroy(inlineBegin(), inlineEnd());
        inlineLength() = 0;
    } else {
        Impl::destroy(heapBegin(), heapEnd());
        mCx->free(heapBegin());
    }

    
    if (length <= sInlineCapacity) {
        



        mLengthOrCapacity = length;  
        Impl::copyConstruct(inlineBegin(), p, p + length);
        Impl::destroy(p, p + length);
        mCx->free(p);
    } else {
        mLengthOrCapacity = length;  
        heapBegin() = p;
        heapEnd() = heapBegin() + length;
    }
}

#endif 
