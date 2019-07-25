






































#ifndef jstl_h_
#define jstl_h_

#include "jsbit.h"
#include "jsstaticcheck.h"
#include "jsstdint.h"

#include <new>
#include <string.h>

namespace js {


namespace tl {


template <size_t i, size_t j> struct Min {
    static const size_t result = i < j ? i : j;
};
template <size_t i, size_t j> struct Max {
    static const size_t result = i > j ? i : j;
};
template <size_t i, size_t min, size_t max> struct Clamp {
    static const size_t result = i < min ? min : (i > max ? max : i);
};


template <size_t x, size_t y> struct Pow {
    static const size_t result = x * Pow<x, y - 1>::result;
};
template <size_t x> struct Pow<x,0> {
    static const size_t result = 1;
};


template <size_t i> struct FloorLog2 {
    static const size_t result = 1 + FloorLog2<i / 2>::result;
};
template <> struct FloorLog2<0> {  };
template <> struct FloorLog2<1> { static const size_t result = 0; };


template <size_t i> struct CeilingLog2 {
    static const size_t result = FloorLog2<2 * i - 1>::result;
};


template <size_t i> struct RoundUpPow2 {
    static const size_t result = 1u << CeilingLog2<i>::result;
};
template <> struct RoundUpPow2<0> {
    static const size_t result = 1;
};


template <class T> struct BitSize {
    static const size_t result = sizeof(T) * JS_BITS_PER_BYTE;
};


template <bool> struct StaticAssert {};
template <> struct StaticAssert<true> { typedef int result; };


template <class T, class U> struct IsSameType {
    static const bool result = false;
};
template <class T> struct IsSameType<T,T> {
    static const bool result = true;
};





template <size_t N> struct NBitMask {
    typedef typename StaticAssert<N < BitSize<size_t>::result>::result _;
    static const size_t result = (size_t(1) << N) - 1;
};
template <> struct NBitMask<BitSize<size_t>::result> {
    static const size_t result = size_t(-1);
};





template <size_t N> struct MulOverflowMask {
    static const size_t result =
        ~NBitMask<BitSize<size_t>::result - CeilingLog2<N>::result>::result;
};
template <> struct MulOverflowMask<0> {  };
template <> struct MulOverflowMask<1> { static const size_t result = 0; };






template <class T> struct UnsafeRangeSizeMask {
    



    static const size_t result = MulOverflowMask<2 * sizeof(T)>::result;
};


template <class T> struct StripConst          { typedef T result; };
template <class T> struct StripConst<const T> { typedef T result; };





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


template <class T, size_t N> inline T *ArraySize(T (&)[N]) { return N; }
template <class T, size_t N> inline T *ArrayEnd(T (&arr)[N]) { return arr + N; }

template <bool cond, typename T, T v1, T v2> struct If        { static const T result = v1; };
template <typename T, T v1, T v2> struct If<false, T, v1, v2> { static const T result = v2; };

} 


class ReentrancyGuard
{
    
    ReentrancyGuard(const ReentrancyGuard &);
    void operator=(const ReentrancyGuard &);

#ifdef DEBUG
    bool &entered;
#endif
  public:
    template <class T>
#ifdef DEBUG
    ReentrancyGuard(T &obj)
      : entered(obj.entered)
#else
    ReentrancyGuard(T &/*obj*/)
#endif
    {
#ifdef DEBUG
        JS_ASSERT(!entered);
        entered = true;
#endif
    }
    ~ReentrancyGuard()
    {
#ifdef DEBUG
        entered = false;
#endif
    }
};





STATIC_POSTCONDITION_ASSUME(return >= x)
JS_ALWAYS_INLINE size_t
RoundUpPow2(size_t x)
{
    size_t log2 = JS_CEILING_LOG2W(x);
    JS_ASSERT(log2 < tl::BitSize<size_t>::result);
    size_t result = size_t(1) << log2;
    return result;
}







template <class T>
JS_ALWAYS_INLINE size_t
PointerRangeSize(T *begin, T *end)
{
    return (size_t(end) - size_t(begin)) / sizeof(T);
}














class SystemAllocPolicy
{
  public:
    void *malloc_(size_t bytes) { return js::OffTheBooks::malloc_(bytes); }
    void *realloc_(void *p, size_t bytes) { return js::OffTheBooks::realloc_(p, bytes); }
    void free_(void *p) { js::UnwantedForeground::free_(p); }
    void reportAllocOverflow() const {}
};











template <size_t nbytes>
struct AlignedStorage
{
    union U {
        char bytes[nbytes];
        uint64 _;
    } u;

    const void *addr() const { return u.bytes; }
    void *addr() { return u.bytes; }
};

template <class T>
struct AlignedStorage2
{
    union U {
        char bytes[sizeof(T)];
        uint64 _;
    } u;

    const T *addr() const { return (const T *)u.bytes; }
    T *addr() { return (T *)u.bytes; }
};












template <class T>
class LazilyConstructed
{
    AlignedStorage2<T> storage;
    bool constructed;

    T &asT() { return *storage.addr(); }

    explicit LazilyConstructed(const LazilyConstructed &other);
    const LazilyConstructed &operator=(const LazilyConstructed &other);

  public:
    LazilyConstructed() { constructed = false; }
    ~LazilyConstructed() { if (constructed) asT().~T(); }

    bool empty() const { return !constructed; }

    void construct() {
        JS_ASSERT(!constructed);
        new(storage.addr()) T();
        constructed = true;
    }

    template <class T1>
    void construct(const T1 &t1) {
        JS_ASSERT(!constructed);
        new(storage.addr()) T(t1);
        constructed = true;
    }

    template <class T1, class T2>
    void construct(const T1 &t1, const T2 &t2) {
        JS_ASSERT(!constructed);
        new(storage.addr()) T(t1, t2);
        constructed = true;
    }

    template <class T1, class T2, class T3>
    void construct(const T1 &t1, const T2 &t2, const T3 &t3) {
        JS_ASSERT(!constructed);
        new(storage.addr()) T(t1, t2, t3);
        constructed = true;
    }

    template <class T1, class T2, class T3, class T4>
    void construct(const T1 &t1, const T2 &t2, const T3 &t3, const T4 &t4) {
        JS_ASSERT(!constructed);
        new(storage.addr()) T(t1, t2, t3, t4);
        constructed = true;
    }

    T *addr() {
        JS_ASSERT(constructed);
        return &asT();
    }

    T &ref() {
        JS_ASSERT(constructed);
        return asT();
    }

    void destroy() {
        ref().~T();
        constructed = false;
    }

    void destroyIfConstructed() {
        if (!empty())
            destroy();
    }
};






template <class T>
class Conditionally {
    LazilyConstructed<T> t;

  public:
    Conditionally(bool b) { if (b) t.construct(); }

    template <class T1>
    Conditionally(bool b, const T1 &t1) { if (b) t.construct(t1); }

    template <class T1, class T2>
    Conditionally(bool b, const T1 &t1, const T2 &t2) { if (b) t.construct(t1, t2); }
};

template <class T>
class AlignedPtrAndFlag
{
    uintptr_t bits;

  public:
    AlignedPtrAndFlag(T *t, bool flag) {
        JS_ASSERT((uintptr_t(t) & 1) == 0);
        bits = uintptr_t(t) | uintptr_t(flag);
    }

    T *ptr() const {
        return (T *)(bits & ~uintptr_t(1));
    }

    bool flag() const {
        return (bits & 1) != 0;
    }

    void setPtr(T *t) {
        JS_ASSERT((uintptr_t(t) & 1) == 0);
        bits = uintptr_t(t) | uintptr_t(flag());
    }

    void setFlag() {
        bits |= 1;
    }

    void unsetFlag() {
        bits &= ~uintptr_t(1);
    }

    void set(T *t, bool flag) {
        JS_ASSERT((uintptr_t(t) & 1) == 0);
        bits = uintptr_t(t) | flag;
    }
};

template <class T>
static inline void
Reverse(T *beg, T *end)
{
    while (beg != end) {
        if (--end == beg)
            return;
        T tmp = *beg;
        *beg = *end;
        *end = tmp;
        ++beg;
    }
}

template <class T>
static inline T *
Find(T *beg, T *end, const T &v)
{
    for (T *p = beg; p != end; ++p) {
        if (*p == v)
            return p;
    }
    return end;
}

template <class Container>
static inline typename Container::ElementType *
Find(Container &c, const typename Container::ElementType &v)
{
    return Find(c.begin(), c.end(), v);
}

template <typename InputIterT, typename CallableT>
void
ForEach(InputIterT begin, InputIterT end, CallableT f)
{
    for (; begin != end; ++begin)
        f(*begin);
}

template <class T>
static inline T
Min(T t1, T t2)
{
    return t1 < t2 ? t1 : t2;
}

template <class T>
static inline T
Max(T t1, T t2)
{
    return t1 > t2 ? t1 : t2;
}


template <class T>
static T&
InitConst(const T &t)
{
    return const_cast<T &>(t);
}


template <class T>
class RangeCheckedPointer
{
    T *ptr;

#ifdef DEBUG
    T * const rangeStart;
    T * const rangeEnd;
#endif

    void sanityChecks() {
        JS_ASSERT(rangeStart <= ptr);
        JS_ASSERT(ptr <= rangeEnd);
    }

    
    RangeCheckedPointer<T> create(T *ptr) const {
#ifdef DEBUG
        return RangeCheckedPointer<T>(ptr, rangeStart, rangeEnd);
#else
        return RangeCheckedPointer<T>(ptr, NULL, size_t(0));
#endif
    }

  public:
    RangeCheckedPointer(T *p, T *start, T *end)
      : ptr(p)
#ifdef DEBUG
      , rangeStart(start), rangeEnd(end)
#endif
    {
        JS_ASSERT(rangeStart <= rangeEnd);
        sanityChecks();
    }
    RangeCheckedPointer(T *p, T *start, size_t length)
      : ptr(p)
#ifdef DEBUG
      , rangeStart(start), rangeEnd(start + length)
#endif
    {
        JS_ASSERT(length <= size_t(-1) / sizeof(T));
        JS_ASSERT(uintptr_t(rangeStart) + length * sizeof(T) >= uintptr_t(rangeStart));
        sanityChecks();
    }

    RangeCheckedPointer<T> &operator=(const RangeCheckedPointer<T> &other) {
        JS_ASSERT(rangeStart == other.rangeStart);
        JS_ASSERT(rangeEnd == other.rangeEnd);
        ptr = other.ptr;
        sanityChecks();
        return *this;
    }

    RangeCheckedPointer<T> operator+(size_t inc) {
        JS_ASSERT(inc <= size_t(-1) / sizeof(T));
        JS_ASSERT(ptr + inc > ptr);
        return create(ptr + inc);
    }

    RangeCheckedPointer<T> operator-(size_t dec) {
        JS_ASSERT(dec <= size_t(-1) / sizeof(T));
        JS_ASSERT(ptr - dec < ptr);
        return create(ptr - dec);
    }

    template <class U>
    RangeCheckedPointer<T> &operator=(U *p) {
        *this = create(p);
        return *this;
    }

    template <class U>
    RangeCheckedPointer<T> &operator=(const RangeCheckedPointer<U> &p) {
        JS_ASSERT(rangeStart <= p.ptr);
        JS_ASSERT(p.ptr <= rangeEnd);
        ptr = p.ptr;
        sanityChecks();
        return *this;
    }

    RangeCheckedPointer<T> &operator++() {
        return (*this += 1);
    }

    RangeCheckedPointer<T> operator++(int) {
        RangeCheckedPointer<T> rcp = *this;
        ++*this;
        return rcp;
    }

    RangeCheckedPointer<T> &operator--() {
        return (*this -= 1);
    }

    RangeCheckedPointer<T> operator--(int) {
        RangeCheckedPointer<T> rcp = *this;
        --*this;
        return rcp;
    }

    RangeCheckedPointer<T> &operator+=(size_t inc) {
        this->operator=<T>(*this + inc);
        return *this;
    }

    RangeCheckedPointer<T> &operator-=(size_t dec) {
        this->operator=<T>(*this - dec);
        return *this;
    }

    T &operator[](int index) const {
        JS_ASSERT(size_t(index > 0 ? index : -index) <= size_t(-1) / sizeof(T));
        return *create(ptr + index);
    }

    T &operator*() const {
        return *ptr;
    }

    operator T*() const {
        return ptr;
    }

    template <class U>
    bool operator==(const RangeCheckedPointer<U> &other) const {
        return ptr == other.ptr;
    }
    template <class U>
    bool operator!=(const RangeCheckedPointer<U> &other) const {
        return !(*this == other);
    }

    template <class U>
    bool operator<(const RangeCheckedPointer<U> &other) const {
        return ptr < other.ptr;
    }
    template <class U>
    bool operator<=(const RangeCheckedPointer<U> &other) const {
        return ptr <= other.ptr;
    }

    template <class U>
    bool operator>(const RangeCheckedPointer<U> &other) const {
        return ptr > other.ptr;
    }
    template <class U>
    bool operator>=(const RangeCheckedPointer<U> &other) const {
        return ptr >= other.ptr;
    }

    size_t operator-(const RangeCheckedPointer<T> &other) const {
        JS_ASSERT(ptr >= other.ptr);
        return PointerRangeSize(other.ptr, ptr);
    }

  private:
    RangeCheckedPointer();
    T *operator&();
};

} 

#endif 
