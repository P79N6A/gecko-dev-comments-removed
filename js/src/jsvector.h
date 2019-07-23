






































#ifndef jsvector_h_
#define jsvector_h_

#include "jscntxt.h"

#include <string.h>
#include <new>





template <class T> struct IsPodType     { static const bool result = false; };
template <> struct IsPodType<char>      { static const bool result = true; };
template <> struct IsPodType<int>       { static const bool result = true; };
template <> struct IsPodType<short>     { static const bool result = true; };
template <> struct IsPodType<long>      { static const bool result = true; };
template <> struct IsPodType<float>     { static const bool result = true; };
template <> struct IsPodType<double>    { static const bool result = true; };
template <> struct IsPodType<jschar>    { static const bool result = true; };





template <class T, bool IsPod>
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
    static inline void copyInitialize(T *dst, const U *srcbeg, const U *srcend) {
        for (const U *p = srcbeg; p != srcend; ++p, ++dst)
            new(dst) T(*p);
    }

    



    static inline bool growTo(JSTempVector<T> &vec, size_t newcap) {
        size_t bytes = sizeof(T) * newcap;
        T *newbuf = reinterpret_cast<T *>(malloc(bytes));
        if (!newbuf) {
            js_ReportOutOfMemory(vec.mCx);
            return false;
        }
        for (T *dst = newbuf, *src = vec.mBegin; src != vec.mEnd; ++dst, ++src)
            new(dst) T(*src);
        JSTempVectorImpl::destroy(vec.mBegin, vec.mEnd);
        free(vec.mBegin);
        vec.mEnd = newbuf + (vec.mEnd - vec.mBegin);
        vec.mBegin = newbuf;
        vec.mCapacity = newbuf + newcap;
        return true;
    }
};






template <class T>
struct JSTempVectorImpl<T, true>
{
    static inline void destroy(T *, T *) {}

    static inline void initialize(T *begin, T *end) {
        
        for (T *p = begin; p != end; ++p)
            *p = 0;
    }

    static inline void copyInitialize(T *dst, const T *srcbeg, const T *srcend) {
        
        for (const T *p = srcbeg; p != srcend; ++p, ++dst)
            *dst = *p;
    }

    static inline bool growTo(JSTempVector<T> &vec, size_t newcap) {
        size_t bytes = sizeof(T) * newcap;
        T *newbuf = reinterpret_cast<T *>(realloc(vec.mBegin, bytes));
        if (!newbuf) {
            js_ReportOutOfMemory(vec.mCx);
            return false;
        }
        vec.mEnd = newbuf + (vec.mEnd - vec.mBegin);
        vec.mBegin = newbuf;
        vec.mCapacity = newbuf + newcap;
        return true;
    }
};














template <class T>
class JSTempVector
{
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

  public:
    JSTempVector(JSContext *cx)
      :
#ifdef DEBUG
        mInProgress(false),
#endif
        mCx(cx), mBegin(0), mEnd(0), mCapacity(0)
    {}
    ~JSTempVector();

    JSTempVector(const JSTempVector &);
    JSTempVector &operator=(const JSTempVector &);

    

    size_t size() const     { return mEnd - mBegin; }
    size_t capacity() const { return mCapacity - mBegin; }
    bool empty() const      { return mBegin == mEnd; }

    T &operator[](size_t i) {
        JS_ASSERT(!mInProgress && i < size());
        return mBegin[i];
    }

    const T &operator[](size_t i) const {
        JS_ASSERT(!mInProgress && i < size());
        return mBegin[i];
    }

    T *begin() {
        JS_ASSERT(!mInProgress);
        return mBegin;
    }

    const T *begin() const {
        JS_ASSERT(!mInProgress);
        return mBegin;
    }

    T *end() {
        JS_ASSERT(!mInProgress);
        return mEnd;
    }

    const T *end() const {
        JS_ASSERT(!mInProgress);
        return mEnd;
    }

    T &back() {
        JS_ASSERT(!mInProgress);
        return *(mEnd - 1);
    }

    const T &back() const {
        JS_ASSERT(!mInProgress && !empty());
        return *(mEnd - 1);
    }

    

    bool reserve(size_t);
    bool growBy(size_t);
    void clear();

    bool pushBack(const T &);
    template <class U> bool pushBack(const U *begin, const U *end);

    




    T *extractRawBuffer();

    




    void replaceRawBuffer(T *, size_t length);

  private:
    typedef JSTempVectorImpl<T, IsPodType<T>::result> Impl;
    friend struct JSTempVectorImpl<T, IsPodType<T>::result>;

    static const int sGrowthFactor = 3;

    bool checkOverflow(size_t newval, size_t oldval, size_t diff) const;

    JSContext *mCx;
    T *mBegin, *mEnd, *mCapacity;
};

template <class T>
inline
JSTempVector<T>::~JSTempVector()
{
    ReentrancyGuard g(*this);
    Impl::destroy(mBegin, mEnd);
    free(mBegin);
}

template <class T>
inline bool
JSTempVector<T>::reserve(size_t newsz)
{
    ReentrancyGuard g(*this);
    size_t oldcap = capacity();
    if (newsz > oldcap) {
        size_t diff = newsz - oldcap;
        size_t newcap = diff + oldcap * sGrowthFactor;
        return checkOverflow(newcap, oldcap, diff) &&
               Impl::growTo(*this, newcap);
    }
    return true;
}

template <class T>
inline bool
JSTempVector<T>::growBy(size_t amount)
{
    
    size_t oldsize = size(), newsize = oldsize + amount;
    if (!checkOverflow(newsize, oldsize, amount) ||
        (newsize > capacity() && !reserve(newsize)))
        return false;

    
    ReentrancyGuard g(*this);
    JS_ASSERT(mCapacity - (mBegin + newsize) >= 0);
    T *newend = mBegin + newsize;
    Impl::initialize(mEnd, newend);
    mEnd = newend;
    return true;
}

template <class T>
inline void
JSTempVector<T>::clear()
{
    ReentrancyGuard g(*this);
    Impl::destroy(mBegin, mEnd);
    mEnd = mBegin;
}





template <class T>
inline bool
JSTempVector<T>::checkOverflow(size_t newval, size_t oldval, size_t diff) const
{
    size_t newbytes = newval * sizeof(T),
           oldbytes = oldval * sizeof(T),
           diffbytes = diff * sizeof(T);
    bool ok = newbytes >= oldbytes && (newbytes - oldbytes) >= diffbytes;
    if (!ok)
        js_ReportAllocationOverflow(mCx);
    return ok;
}

template <class T>
inline bool
JSTempVector<T>::pushBack(const T &t)
{
    ReentrancyGuard g(*this);
    if (mEnd == mCapacity) {
        
        size_t oldcap = capacity();
        size_t newcap = empty() ? 1 : oldcap * sGrowthFactor;
        if (!checkOverflow(newcap, oldcap, 1) ||
            !Impl::growTo(*this, newcap))
            return false;
    }
    JS_ASSERT(mEnd != mCapacity);
    new(mEnd++) T(t);
    return true;
}

template <class T>
template <class U>
inline bool
JSTempVector<T>::pushBack(const U *begin, const U *end)
{
    ReentrancyGuard g(*this);
    size_t space = mCapacity - mEnd, needed = end - begin;
    if (space < needed) {
        
        size_t oldcap = capacity();
        size_t newcap = empty() ? needed : (needed + oldcap * sGrowthFactor);
        if (!checkOverflow(newcap, oldcap, needed) ||
            !Impl::growTo(*this, newcap))
            return false;
    }
    JS_ASSERT((mCapacity - mEnd) >= (end - begin));
    Impl::copyInitialize(mEnd, begin, end);
    mEnd += needed;
    return true;
}

template <class T>
inline T *
JSTempVector<T>::extractRawBuffer()
{
    T *ret = mBegin;
    mBegin = mEnd = mCapacity = 0;
    return ret;
}

template <class T>
inline void
JSTempVector<T>::replaceRawBuffer(T *p, size_t length)
{
    ReentrancyGuard g(*this);
    Impl::destroy(mBegin, mEnd);
    free(mBegin);
    mBegin = p;
    mCapacity = mEnd = mBegin + length;
}

#endif 
