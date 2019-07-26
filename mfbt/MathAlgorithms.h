






#ifndef mozilla_MathAlgorithms_h_
#define mozilla_MathAlgorithms_h_

#include "mozilla/Assertions.h"
#include "mozilla/StandardInteger.h"
#include "mozilla/TypeTraits.h"

#include <limits.h>
#include <math.h>

namespace mozilla {


template<typename IntegerType>
MOZ_ALWAYS_INLINE IntegerType
EuclidGCD(IntegerType a, IntegerType b)
{
  
  
  MOZ_ASSERT(a > 0);
  MOZ_ASSERT(b > 0);

  while (a != b) {
    if (a > b) {
      a = a - b;
    } else {
      b = b - a;
    }
  }

  return a;
}


template<typename IntegerType>
MOZ_ALWAYS_INLINE IntegerType
EuclidLCM(IntegerType a, IntegerType b)
{
  
  return (a / EuclidGCD(a, b)) * b;
}

namespace detail {

template<typename T>
struct AllowDeprecatedAbsFixed : FalseType {};

template<> struct AllowDeprecatedAbsFixed<int8_t> : TrueType {};
template<> struct AllowDeprecatedAbsFixed<int16_t> : TrueType {};
template<> struct AllowDeprecatedAbsFixed<int32_t> : TrueType {};
template<> struct AllowDeprecatedAbsFixed<int64_t> : TrueType {};

template<typename T>
struct AllowDeprecatedAbs : AllowDeprecatedAbsFixed<T> {};

template<> struct AllowDeprecatedAbs<char> : IntegralConstant<bool, char(-1) < char(0)> {};
template<> struct AllowDeprecatedAbs<signed char> : TrueType {};
template<> struct AllowDeprecatedAbs<short> : TrueType {};
template<> struct AllowDeprecatedAbs<int> : TrueType {};
template<> struct AllowDeprecatedAbs<long> : TrueType {};
template<> struct AllowDeprecatedAbs<long long> : TrueType {};

} 



template<typename T>
inline typename mozilla::EnableIf<detail::AllowDeprecatedAbs<T>::value, T>::Type
DeprecatedAbs(const T t)
{
  
  
  
  
  
  
  
  
  
  MOZ_ASSERT(t >= 0 ||
             -(t + 1) != T((1ULL << (CHAR_BIT * sizeof(T) - 1)) - 1),
             "You can't negate the smallest possible negative integer!");
  return t >= 0 ? t : -t;
}

namespace detail {





template<typename T>
struct AbsReturnTypeFixed;

template<> struct AbsReturnTypeFixed<int8_t> { typedef uint8_t Type; };
template<> struct AbsReturnTypeFixed<int16_t> { typedef uint16_t Type; };
template<> struct AbsReturnTypeFixed<int32_t> { typedef uint32_t Type; };
template<> struct AbsReturnTypeFixed<int64_t> { typedef uint64_t Type; };

template<typename T>
struct AbsReturnType : AbsReturnTypeFixed<T> {};

template<> struct AbsReturnType<char> : EnableIf<char(-1) < char(0), unsigned char> {};
template<> struct AbsReturnType<signed char> { typedef unsigned char Type; };
template<> struct AbsReturnType<short> { typedef unsigned short Type; };
template<> struct AbsReturnType<int> { typedef unsigned int Type; };
template<> struct AbsReturnType<long> { typedef unsigned long Type; };
template<> struct AbsReturnType<long long> { typedef unsigned long long Type; };
template<> struct AbsReturnType<float> { typedef float Type; };
template<> struct AbsReturnType<double> { typedef double Type; };
template<> struct AbsReturnType<long double> { typedef long double Type; };

} 

template<typename T>
inline typename detail::AbsReturnType<T>::Type
Abs(const T t)
{
  typedef typename detail::AbsReturnType<T>::Type ReturnType;
  return t >= 0 ? ReturnType(t) : ~ReturnType(t) + 1;
}

template<>
inline float
Abs<float>(const float f)
{
  return fabsf(f);
}

template<>
inline double
Abs<double>(const double d)
{
  return fabs(d);
}

template<>
inline long double
Abs<long double>(const long double d)
{
  return fabsl(d);
}

} 

#endif  
