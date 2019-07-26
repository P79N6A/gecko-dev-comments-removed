







#ifndef mozilla_MathAlgorithms_h
#define mozilla_MathAlgorithms_h

#include "mozilla/Assertions.h"
#include "mozilla/TypeTraits.h"

#include <cmath>
#include <limits.h>
#include <stdint.h>

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

template<> struct AllowDeprecatedAbsFixed<int32_t> : TrueType {};
template<> struct AllowDeprecatedAbsFixed<int64_t> : TrueType {};

template<typename T>
struct AllowDeprecatedAbs : AllowDeprecatedAbsFixed<T> {};

template<> struct AllowDeprecatedAbs<int> : TrueType {};
template<> struct AllowDeprecatedAbs<long> : TrueType {};

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
  return std::fabs(f);
}

template<>
inline double
Abs<double>(const double d)
{
  return std::fabs(d);
}

template<>
inline long double
Abs<long double>(const long double d)
{
  return std::fabs(d);
}

} 

#if defined(_WIN32) && (_MSC_VER >= 1300) && (defined(_M_IX86) || defined(_M_AMD64) || defined(_M_X64))
#  define MOZ_BITSCAN_WINDOWS

  extern "C" {
    unsigned char _BitScanForward(unsigned long* Index, unsigned long mask);
    unsigned char _BitScanReverse(unsigned long* Index, unsigned long mask);
#  pragma intrinsic(_BitScanForward, _BitScanReverse)

#  if defined(_M_AMD64) || defined(_M_X64)
#    define MOZ_BITSCAN_WINDOWS64
    unsigned char _BitScanForward64(unsigned long* index, unsigned __int64 mask);
    unsigned char _BitScanReverse64(unsigned long* index, unsigned __int64 mask);
#   pragma intrinsic(_BitScanForward64, _BitScanReverse64)
#  endif
  } 

#endif

namespace mozilla {

namespace detail {

#if defined(MOZ_BITSCAN_WINDOWS)

  inline uint_fast8_t
  CountLeadingZeroes32(uint32_t u)
  {
    unsigned long index;
    _BitScanReverse(&index, static_cast<unsigned long>(u));
    return uint_fast8_t(31 - index);
  }


  inline uint_fast8_t
  CountTrailingZeroes32(uint32_t u)
  {
    unsigned long index;
    _BitScanForward(&index, static_cast<unsigned long>(u));
    return uint_fast8_t(index);
  }

  inline uint_fast8_t
  CountPopulation32(uint32_t u)
  {
    uint32_t x = u - ((u >> 1) & 0x55555555);
    x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
    return (((x + (x >> 4)) & 0xf0f0f0f) * 0x1010101) >> 24;
  }

  inline uint_fast8_t
  CountLeadingZeroes64(uint64_t u)
  {
#  if defined(MOZ_BITSCAN_WINDOWS64)
    unsigned long index;
    _BitScanReverse64(&index, static_cast<unsigned __int64>(u));
    return uint_fast8_t(63 - index);
#  else
    uint32_t hi = uint32_t(u >> 32);
    if (hi != 0)
      return CountLeadingZeroes32(hi);
    return 32u + CountLeadingZeroes32(uint32_t(u));
#  endif
  }

  inline uint_fast8_t
  CountTrailingZeroes64(uint64_t u)
  {
#  if defined(MOZ_BITSCAN_WINDOWS64)
    unsigned long index;
    _BitScanForward64(&index, static_cast<unsigned __int64>(u));
    return uint_fast8_t(index);
#  else
    uint32_t lo = uint32_t(u);
    if (lo != 0)
      return CountTrailingZeroes32(lo);
    return 32u + CountTrailingZeroes32(uint32_t(u >> 32));
#  endif
  }

#  ifdef MOZ_HAVE_BITSCAN64
#    undef MOZ_HAVE_BITSCAN64
#  endif

#elif defined(__clang__) || defined(__GNUC__)

#  if defined(__clang__)
#    if !__has_builtin(__builtin_ctz) || !__has_builtin(__builtin_clz)
#      error "A clang providing __builtin_c[lt]z is required to build"
#    endif
#  else
     
#  endif

  inline uint_fast8_t
  CountLeadingZeroes32(uint32_t u)
  {
    return __builtin_clz(u);
  }

  inline uint_fast8_t
  CountTrailingZeroes32(uint32_t u)
  {
    return __builtin_ctz(u);
  }

  inline uint_fast8_t
  CountPopulation32(uint32_t u)
  {
    return __builtin_popcount(u);
  }

  inline uint_fast8_t
  CountLeadingZeroes64(uint64_t u)
  {
    return __builtin_clzll(u);
  }

  inline uint_fast8_t
  CountTrailingZeroes64(uint64_t u)
  {
    return __builtin_ctzll(u);
  }

#else
#  error "Implement these!"
  inline uint_fast8_t CountLeadingZeroes32(uint32_t u) MOZ_DELETE;
  inline uint_fast8_t CountTrailingZeroes32(uint32_t u) MOZ_DELETE;
  inline uint_fast8_t CountPopulation32(uint32_t u) MOZ_DELETE;
  inline uint_fast8_t CountLeadingZeroes64(uint64_t u) MOZ_DELETE;
  inline uint_fast8_t CountTrailingZeroes64(uint64_t u) MOZ_DELETE;
#endif

} 












inline uint_fast8_t
CountLeadingZeroes32(uint32_t u)
{
  MOZ_ASSERT(u != 0);
  return detail::CountLeadingZeroes32(u);
}












inline uint_fast8_t
CountTrailingZeroes32(uint32_t u)
{
  MOZ_ASSERT(u != 0);
  return detail::CountTrailingZeroes32(u);
}




inline uint_fast8_t
CountPopulation32(uint32_t u)
{
  return detail::CountPopulation32(u);
}


inline uint_fast8_t
CountLeadingZeroes64(uint64_t u)
{
  MOZ_ASSERT(u != 0);
  return detail::CountLeadingZeroes64(u);
}


inline uint_fast8_t
CountTrailingZeroes64(uint64_t u)
{
  MOZ_ASSERT(u != 0);
  return detail::CountTrailingZeroes64(u);
}

namespace detail {

template<typename T, size_t Size = sizeof(T)>
class CeilingLog2;

template<typename T>
class CeilingLog2<T, 4>
{
  public:
    static uint_fast8_t compute(const T t) {
      
      return t <= 1 ? 0u : 32u - CountLeadingZeroes32(t - 1);
    }
};

template<typename T>
class CeilingLog2<T, 8>
{
  public:
    static uint_fast8_t compute(const T t) {
      
      return t <= 1 ? 0 : 64 - CountLeadingZeroes64(t - 1);
    }
};

} 










template<typename T>
inline uint_fast8_t
CeilingLog2(const T t)
{
  return detail::CeilingLog2<T>::compute(t);
}


inline uint_fast8_t
CeilingLog2Size(size_t n)
{
  return CeilingLog2(n);
}

namespace detail {

template<typename T, size_t Size = sizeof(T)>
class FloorLog2;

template<typename T>
class FloorLog2<T, 4>
{
  public:
    static uint_fast8_t compute(const T t) {
      return 31u - CountLeadingZeroes32(t | 1);
    }
};

template<typename T>
class FloorLog2<T, 8>
{
  public:
    static uint_fast8_t compute(const T t) {
      return 63u - CountLeadingZeroes64(t | 1);
    }
};

} 









template<typename T>
inline uint_fast8_t
FloorLog2(const T t)
{
  return detail::FloorLog2<T>::compute(t);
}


inline uint_fast8_t
FloorLog2Size(size_t n)
{
  return FloorLog2(n);
}





inline size_t
RoundUpPow2(size_t x)
{
  MOZ_ASSERT(x <= (size_t(1) << (sizeof(size_t) * CHAR_BIT - 1)),
             "can't round up -- will overflow!");
  return size_t(1) << CeilingLog2(x);
}




template<typename T>
inline T
RotateLeft(const T t, uint_fast8_t shift)
{
  MOZ_ASSERT(shift < sizeof(T) * CHAR_BIT, "Shift value is too large!");
  static_assert(IsUnsigned<T>::value, "Rotates require unsigned values");
  return (t << shift) | (t >> (sizeof(T) * CHAR_BIT - shift));
}




template<typename T>
inline T
RotateRight(const T t, uint_fast8_t shift)
{
  MOZ_ASSERT(shift < sizeof(T) * CHAR_BIT, "Shift value is too large!");
  static_assert(IsUnsigned<T>::value, "Rotates require unsigned values");
  return (t >> shift) | (t << (sizeof(T) * CHAR_BIT - shift));
}

} 

#endif 
