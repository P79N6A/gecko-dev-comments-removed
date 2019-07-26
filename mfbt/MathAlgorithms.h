






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
struct SupportedForAbsFixed : FalseType {};

template<> struct SupportedForAbsFixed<int8_t> : TrueType {};
template<> struct SupportedForAbsFixed<int16_t> : TrueType {};
template<> struct SupportedForAbsFixed<int32_t> : TrueType {};
template<> struct SupportedForAbsFixed<int64_t> : TrueType {};

template<typename T>
struct SupportedForAbs : SupportedForAbsFixed<T> {};

template<> struct SupportedForAbs<char> : IntegralConstant<bool, char(-1) < char(0)> {};
template<> struct SupportedForAbs<signed char> : TrueType {};
template<> struct SupportedForAbs<short> : TrueType {};
template<> struct SupportedForAbs<int> : TrueType {};
template<> struct SupportedForAbs<long> : TrueType {};
template<> struct SupportedForAbs<long long> : TrueType {};
template<> struct SupportedForAbs<float> : TrueType {};
template<> struct SupportedForAbs<double> : TrueType {};
template<> struct SupportedForAbs<long double> : TrueType {};

} 

template<typename T>
inline typename mozilla::EnableIf<detail::SupportedForAbs<T>::value, T>::Type
Abs(const T t)
{
  
  
  
  
  
  
  
  
  
  MOZ_ASSERT(t >= 0 ||
             -(t + 1) != T((1ULL << (CHAR_BIT * sizeof(T) - 1)) - 1),
             "You can't negate the smallest possible negative integer!");
  return t >= 0 ? t : -t;
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
