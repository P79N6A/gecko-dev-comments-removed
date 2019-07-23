






































#ifndef jsvector_h_
#define jsvector_h_

#include <new>

#include "jstl.h"

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
        T *newbuf = reinterpret_cast<T *>(v.malloc(newcap * sizeof(T)));
        if (!newbuf)
            return false;
        for (T *dst = newbuf, *src = v.heapBegin(); src != v.heapEnd(); ++dst, ++src)
            new(dst) T(*src);
        VectorImpl::destroy(v.heapBegin(), v.heapEnd());
        v.free(v.heapBegin());
        v.heapEnd() = newbuf + v.heapLength();
        v.heapBegin() = newbuf;
        v.heapCapacity() = newcap;
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
        for (T *end = dst + n; dst != end; ++dst)
            *dst = t;
    }

    static inline bool growTo(Vector<T,N,AP> &v, size_t newcap) {
        JS_ASSERT(!v.usingInlineStorage());
        size_t bytes = sizeof(T) * newcap;
        T *newbuf = reinterpret_cast<T *>(v.realloc(v.heapBegin(), bytes));
        if (!newbuf)
            return false;
        v.heapEnd() = newbuf + v.heapLength();
        v.heapBegin() = newbuf;
        v.heapCapacity() = newcap;
        return true;
    }
};



















template <class T, size_t N, class AllocPolicy>
class Vector : AllocPolicy
{
    

    static const bool sElemIsPod = tl::IsPodType<T>::result;
    typedef VectorImpl<T, N, AllocPolicy, sElemIsPod> Impl;
    friend struct VectorImpl<T, N, AllocPolicy, sElemIsPod>;

    bool calculateNewCapacity(size_t curLength, size_t lengthInc, size_t &newCap);
    bool growHeapStorageBy(size_t lengthInc);
    bool convertToHeapStorage(size_t lengthInc);

    

    static const int sMaxInlineBytes = 1024;

    

    




    struct BufferPtrs {
        T *mBegin, *mEnd;
    };

    







    static const size_t sInlineCapacity =
        tl::Clamp<N, sizeof(BufferPtrs) / sizeof(T),
                          sMaxInlineBytes / sizeof(T)>::result;

    
    static const size_t sInlineBytes =
        tl::Max<1, sInlineCapacity * sizeof(T)>::result;

    

    size_t mLengthOrCapacity;
    bool usingInlineStorage() const { return mLengthOrCapacity <= sInlineCapacity; }

    union {
        BufferPtrs ptrs;
        char mBuf[sInlineBytes];
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
    friend class ReentrancyGuard;
    bool mEntered;
#endif

    Vector(const Vector &);
    Vector &operator=(const Vector &);

  public:
    Vector(AllocPolicy = AllocPolicy());
    ~Vector();

    

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
        JS_ASSERT(!mEntered);
        return usingInlineStorage() ? inlineBegin() : heapBegin();
    }

    const T *begin() const {
        JS_ASSERT(!mEntered);
        return usingInlineStorage() ? inlineBegin() : heapBegin();
    }

    T *end() {
        JS_ASSERT(!mEntered);
        return usingInlineStorage() ? inlineEnd() : heapEnd();
    }

    const T *end() const {
        JS_ASSERT(!mEntered);
        return usingInlineStorage() ? inlineEnd() : heapEnd();
    }

    T &operator[](size_t i) {
        JS_ASSERT(!mEntered && i < length());
        return begin()[i];
    }

    const T &operator[](size_t i) const {
        JS_ASSERT(!mEntered && i < length());
        return begin()[i];
    }

    T &back() {
        JS_ASSERT(!mEntered && !empty());
        return *(end() - 1);
    }

    const T &back() const {
        JS_ASSERT(!mEntered && !empty());
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








template <class T, size_t N, class AP, size_t ArrayLength>
bool
js_AppendLiteral(Vector<T,N,AP> &v, const char (&array)[ArrayLength])
{
    return v.append(array, array + ArrayLength - 1);
}




template <class T, size_t N, class AP>
inline
Vector<T,N,AP>::Vector(AP ap)
  : AP(ap), mLengthOrCapacity(0)
#ifdef DEBUG
    , mEntered(false)
#endif
{}

template <class T, size_t N, class AP>
inline
Vector<T,N,AP>::~Vector()
{
    ReentrancyGuard g(*this);
    if (usingInlineStorage()) {
        Impl::destroy(inlineBegin(), inlineEnd());
    } else {
        Impl::destroy(heapBegin(), heapEnd());
        this->free(heapBegin());
    }
}





template <class T, size_t N, class AP>
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
inline bool
Vector<T,N,AP>::growHeapStorageBy(size_t lengthInc)
{
    size_t newCap;
    return calculateNewCapacity(heapLength(), lengthInc, newCap) &&
           Impl::growTo(*this, newCap);
}






template <class T, size_t N, class AP>
inline bool
Vector<T,N,AP>::convertToHeapStorage(size_t lengthInc)
{
    size_t newCap;
    if (!calculateNewCapacity(inlineLength(), lengthInc, newCap))
        return false;

    
    T *newBuf = reinterpret_cast<T *>(this->malloc(newCap * sizeof(T)));
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

template <class T, size_t N, class AP>
inline bool
Vector<T,N,AP>::reserve(size_t request)
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

template <class T, size_t N, class AP>
inline void
Vector<T,N,AP>::shrinkBy(size_t incr)
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

template <class T, size_t N, class AP>
inline bool
Vector<T,N,AP>::growBy(size_t incr)
{
    ReentrancyGuard g(*this);
    if (usingInlineStorage()) {
        size_t freespace = sInlineCapacity - inlineLength();
        if (incr <= freespace) {
            T *newend = inlineEnd() + incr;
            if (!tl::IsPodType<T>::result)
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
    if (!tl::IsPodType<T>::result)
        Impl::initialize(heapEnd(), newend);
    heapEnd() = newend;
    return true;
}

template <class T, size_t N, class AP>
inline bool
Vector<T,N,AP>::resize(size_t newLength)
{
    size_t curLength = length();
    if (newLength > curLength)
        return growBy(newLength - curLength);
    shrinkBy(curLength - newLength);
    return true;
}

template <class T, size_t N, class AP>
inline void
Vector<T,N,AP>::clear()
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

template <class T, size_t N, class AP>
inline bool
Vector<T,N,AP>::append(const T &t)
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

template <class T, size_t N, class AP>
inline bool
Vector<T,N,AP>::appendN(const T &t, size_t needed)
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

template <class T, size_t N, class AP>
template <class U>
inline bool
Vector<T,N,AP>::append(const U *insBegin, const U *insEnd)
{
    ReentrancyGuard g(*this);
    size_t needed = PointerRangeSize(insBegin, insEnd);
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

template <class T, size_t N, class AP>
template <class U>
inline bool
Vector<T,N,AP>::append(const U *insBegin, size_t length)
{
    return this->append(insBegin, insBegin + length);
}

template <class T, size_t N, class AP>
inline void
Vector<T,N,AP>::popBack()
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

template <class T, size_t N, class AP>
inline T *
Vector<T,N,AP>::extractRawBuffer()
{
    if (usingInlineStorage()) {
        T *ret = reinterpret_cast<T *>(this->malloc(inlineLength() * sizeof(T)));
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

template <class T, size_t N, class AP>
inline void
Vector<T,N,AP>::replaceRawBuffer(T *p, size_t length)
{
    ReentrancyGuard g(*this);

    
    if (usingInlineStorage()) {
        Impl::destroy(inlineBegin(), inlineEnd());
        inlineLength() = 0;
    } else {
        Impl::destroy(heapBegin(), heapEnd());
        this->free(heapBegin());
    }

    
    if (length <= sInlineCapacity) {
        



        mLengthOrCapacity = length;  
        Impl::copyConstruct(inlineBegin(), p, p + length);
        Impl::destroy(p, p + length);
        this->free(p);
    } else {
        mLengthOrCapacity = length;  
        heapBegin() = p;
        heapEnd() = heapBegin() + length;
    }
}

}  

#endif 
