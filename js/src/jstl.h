






































#ifndef jstl_h_
#define jstl_h_

#include "jsbit.h"

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


template <class T, size_t N> inline T *ArraySize(T (&)[N]) { return N; }
template <class T, size_t N> inline T *ArrayEnd(T (&arr)[N]) { return arr + N; }

} 


class ReentrancyGuard
{
#ifdef DEBUG
    bool &entered;
#endif
  public:
    template <class T>
    ReentrancyGuard(T &obj)
#ifdef DEBUG
      : entered(obj.entered)
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





static JS_ALWAYS_INLINE size_t
RoundUpPow2(size_t x)
{
    typedef tl::StaticAssert<tl::IsSameType<size_t,JSUword>::result>::result _;
    size_t log2 = JS_CEILING_LOG2W(x);
    JS_ASSERT(log2 < tl::BitSize<size_t>::result);
    size_t result = size_t(1) << log2;
    return result;
}







template <class T>
static JS_ALWAYS_INLINE size_t
PointerRangeSize(T *begin, T *end)
{
    return (size_t(end) - size_t(begin)) / sizeof(T);
}



















class ContextAllocPolicy
{
    JSContext *cx;

  public:
    ContextAllocPolicy(JSContext *cx) : cx(cx) {}
    JSContext *context() const { return cx; }

    void *malloc(size_t bytes) { return cx->malloc(bytes); }
    void free(void *p) { cx->free(p); }
    void *realloc(void *p, size_t bytes) { return cx->realloc(p, bytes); }
    void reportAllocOverflow() const { js_ReportAllocationOverflow(cx); }
};


class SystemAllocPolicy
{
  public:
    void *malloc(size_t bytes) { return ::malloc(bytes); }
    void *realloc(void *p, size_t bytes) { return ::realloc(p, bytes); }
    void free(void *p) { ::free(p); }
    void reportAllocOverflow() const {}
};

} 

#endif 
