















#ifndef mozilla_TemplateLib_h
#define mozilla_TemplateLib_h

#include <limits.h>
#include <stddef.h>

namespace mozilla {

namespace tl {


template<size_t I, size_t J>
struct Min
{
  static const size_t value = I < J ? I : J;
};
template<size_t I, size_t J>
struct Max
{
  static const size_t value = I > J ? I : J;
};


template<size_t I>
struct FloorLog2
{
  static const size_t value = 1 + FloorLog2<I / 2>::value;
};
template<> struct FloorLog2<0> {  };
template<> struct FloorLog2<1> { static const size_t value = 0; };


template<size_t I>
struct CeilingLog2
{
  static const size_t value = FloorLog2<2 * I - 1>::value;
};


template<size_t I>
struct RoundUpPow2
{
  static const size_t value = size_t(1) << CeilingLog2<I>::value;
};
template<>
struct RoundUpPow2<0>
{
  static const size_t value = 1;
};


template<typename T>
struct BitSize
{
  static const size_t value = sizeof(T) * CHAR_BIT;
};





template<size_t N>
struct NBitMask
{
  
  
  
  
  static const size_t checkPrecondition =
    0 / size_t(N < BitSize<size_t>::value);
  static const size_t value = (size_t(1) << N) - 1 + checkPrecondition;
};
template<>
struct NBitMask<BitSize<size_t>::value>
{
  static const size_t value = size_t(-1);
};





template<size_t N>
struct MulOverflowMask
{
  static const size_t value =
    ~NBitMask<BitSize<size_t>::value - CeilingLog2<N>::value>::value;
};
template<> struct MulOverflowMask<0> {  };
template<> struct MulOverflowMask<1> { static const size_t value = 0; };

} 

} 

#endif 
