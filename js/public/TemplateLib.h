






#ifndef js_template_lib_h__
#define js_template_lib_h__

#include "jstypes.h"








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
    static const size_t result = size_t(1) << CeilingLog2<i>::result;
};
template <> struct RoundUpPow2<0> {
    static const size_t result = 1;
};


template <class T> struct BitSize {
    static const size_t result = sizeof(T) * JS_BITS_PER_BYTE;
};





template <size_t N> struct NBitMask {
    
    
    
    
    static const size_t checkPrecondition = 0 / size_t(N < BitSize<size_t>::result);
    static const size_t result = (size_t(1) << N) - 1 + checkPrecondition;
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

template <bool cond, typename T, T v1, T v2> struct If        { static const T result = v1; };
template <typename T, T v1, T v2> struct If<false, T, v1, v2> { static const T result = v2; };




template <class T> struct IsRelocatableHeapType { static const bool result = true; };

} 
} 

#endif  
