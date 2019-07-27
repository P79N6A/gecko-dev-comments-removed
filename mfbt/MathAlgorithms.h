







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
EuclidGCD(IntegerType aA, IntegerType aB)
{
  
  
  MOZ_ASSERT(aA > IntegerType(0));
  MOZ_ASSERT(aB > IntegerType(0));

  while (aA != aB) {
    if (aA > aB) {
      aA = aA - aB;
    } else {
      aB = aB - aA;
    }
  }

  return aA;
}


template<typename IntegerType>
MOZ_ALWAYS_INLINE IntegerType
EuclidLCM(IntegerType aA, IntegerType aB)
{
  
  return (aA / EuclidGCD(aA, aB)) * aB;
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
DeprecatedAbs(const T aValue)
{
  
  
  
  
  
  
  
  
  
  MOZ_ASSERT(aValue >= 0 ||
             -(aValue + 1) != T((1ULL << (CHAR_BIT * sizeof(T) - 1)) - 1),
             "You can't negate the smallest possible negative integer!");
  return aValue >= 0 ? aValue : -aValue;
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

template<> struct AbsReturnType<char> :
  EnableIf<char(-1) < char(0), unsigned char> {};
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
Abs(const T aValue)
{
  typedef typename detail::AbsReturnType<T>::Type ReturnType;
  return aValue >= 0 ? ReturnType(aValue) : ~ReturnType(aValue) + 1;
}

template<>
inline float
Abs<float>(const float aFloat)
{
  return std::fabs(aFloat);
}

template<>
inline double
Abs<double>(const double aDouble)
{
  return std::fabs(aDouble);
}

template<>
inline long double
Abs<long double>(const long double aLongDouble)
{
  return std::fabs(aLongDouble);
}

} 

#if defined(_WIN32) && (_MSC_VER >= 1300) && \
    (defined(_M_IX86) || defined(_M_AMD64) || defined(_M_X64))
#  define MOZ_BITSCAN_WINDOWS

#  include <intrin.h>
#  pragma intrinsic(_BitScanForward, _BitScanReverse)

#  if defined(_M_AMD64) || defined(_M_X64)
#    define MOZ_BITSCAN_WINDOWS64
#   pragma intrinsic(_BitScanForward64, _BitScanReverse64)
#  endif

#endif

namespace mozilla {

namespace detail {

#if defined(MOZ_BITSCAN_WINDOWS)

inline uint_fast8_t
CountLeadingZeroes32(uint32_t aValue)
{
  unsigned long index;
  _BitScanReverse(&index, static_cast<unsigned long>(aValue));
  return uint_fast8_t(31 - index);
}


inline uint_fast8_t
CountTrailingZeroes32(uint32_t aValue)
{
  unsigned long index;
  _BitScanForward(&index, static_cast<unsigned long>(aValue));
  return uint_fast8_t(index);
}

inline uint_fast8_t
CountPopulation32(uint32_t aValue)
{
  uint32_t x = aValue - ((aValue >> 1) & 0x55555555);
  x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
  return (((x + (x >> 4)) & 0xf0f0f0f) * 0x1010101) >> 24;
}
inline uint_fast8_t
CountPopulation64(uint64_t aValue)
{
  return uint_fast8_t(CountPopulation32(aValue & 0xffffffff) +
                      CountPopulation32(aValue >> 32));
}

inline uint_fast8_t
CountLeadingZeroes64(uint64_t aValue)
{
#if defined(MOZ_BITSCAN_WINDOWS64)
  unsigned long index;
  _BitScanReverse64(&index, static_cast<unsigned __int64>(aValue));
  return uint_fast8_t(63 - index);
#else
  uint32_t hi = uint32_t(aValue >> 32);
  if (hi != 0) {
    return CountLeadingZeroes32(hi);
  }
  return 32u + CountLeadingZeroes32(uint32_t(aValue));
#endif
}

inline uint_fast8_t
CountTrailingZeroes64(uint64_t aValue)
{
#if defined(MOZ_BITSCAN_WINDOWS64)
  unsigned long index;
  _BitScanForward64(&index, static_cast<unsigned __int64>(aValue));
  return uint_fast8_t(index);
#else
  uint32_t lo = uint32_t(aValue);
  if (lo != 0) {
    return CountTrailingZeroes32(lo);
  }
  return 32u + CountTrailingZeroes32(uint32_t(aValue >> 32));
#endif
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
CountLeadingZeroes32(uint32_t aValue)
{
  return __builtin_clz(aValue);
}

inline uint_fast8_t
CountTrailingZeroes32(uint32_t aValue)
{
  return __builtin_ctz(aValue);
}

inline uint_fast8_t
CountPopulation32(uint32_t aValue)
{
  return __builtin_popcount(aValue);
}

inline uint_fast8_t
CountPopulation64(uint64_t aValue)
{
  return __builtin_popcountll(aValue);
}

inline uint_fast8_t
CountLeadingZeroes64(uint64_t aValue)
{
  return __builtin_clzll(aValue);
}

inline uint_fast8_t
CountTrailingZeroes64(uint64_t aValue)
{
  return __builtin_ctzll(aValue);
}

#else
#  error "Implement these!"
inline uint_fast8_t CountLeadingZeroes32(uint32_t aValue) MOZ_DELETE;
inline uint_fast8_t CountTrailingZeroes32(uint32_t aValue) MOZ_DELETE;
inline uint_fast8_t CountPopulation32(uint32_t aValue) MOZ_DELETE;
inline uint_fast8_t CountPopulation64(uint64_t aValue) MOZ_DELETE;
inline uint_fast8_t CountLeadingZeroes64(uint64_t aValue) MOZ_DELETE;
inline uint_fast8_t CountTrailingZeroes64(uint64_t aValue) MOZ_DELETE;
#endif

} 












inline uint_fast8_t
CountLeadingZeroes32(uint32_t aValue)
{
  MOZ_ASSERT(aValue != 0);
  return detail::CountLeadingZeroes32(aValue);
}












inline uint_fast8_t
CountTrailingZeroes32(uint32_t aValue)
{
  MOZ_ASSERT(aValue != 0);
  return detail::CountTrailingZeroes32(aValue);
}




inline uint_fast8_t
CountPopulation32(uint32_t aValue)
{
  return detail::CountPopulation32(aValue);
}


inline uint_fast8_t
CountPopulation64(uint64_t aValue)
{
  return detail::CountPopulation64(aValue);
}


inline uint_fast8_t
CountLeadingZeroes64(uint64_t aValue)
{
  MOZ_ASSERT(aValue != 0);
  return detail::CountLeadingZeroes64(aValue);
}


inline uint_fast8_t
CountTrailingZeroes64(uint64_t aValue)
{
  MOZ_ASSERT(aValue != 0);
  return detail::CountTrailingZeroes64(aValue);
}

namespace detail {

template<typename T, size_t Size = sizeof(T)>
class CeilingLog2;

template<typename T>
class CeilingLog2<T, 4>
{
public:
  static uint_fast8_t compute(const T aValue)
  {
    
    return aValue <= 1 ? 0u : 32u - CountLeadingZeroes32(aValue - 1);
  }
};

template<typename T>
class CeilingLog2<T, 8>
{
public:
  static uint_fast8_t compute(const T aValue)
  {
    
    return aValue <= 1 ? 0 : 64 - CountLeadingZeroes64(aValue - 1);
  }
};

} 










template<typename T>
inline uint_fast8_t
CeilingLog2(const T aValue)
{
  return detail::CeilingLog2<T>::compute(aValue);
}


inline uint_fast8_t
CeilingLog2Size(size_t aValue)
{
  return CeilingLog2(aValue);
}

namespace detail {

template<typename T, size_t Size = sizeof(T)>
class FloorLog2;

template<typename T>
class FloorLog2<T, 4>
{
public:
  static uint_fast8_t compute(const T aValue)
  {
    return 31u - CountLeadingZeroes32(aValue | 1);
  }
};

template<typename T>
class FloorLog2<T, 8>
{
public:
  static uint_fast8_t compute(const T aValue)
  {
    return 63u - CountLeadingZeroes64(aValue | 1);
  }
};

} 









template<typename T>
inline uint_fast8_t
FloorLog2(const T aValue)
{
  return detail::FloorLog2<T>::compute(aValue);
}


inline uint_fast8_t
FloorLog2Size(size_t aValue)
{
  return FloorLog2(aValue);
}





inline size_t
RoundUpPow2(size_t aValue)
{
  MOZ_ASSERT(aValue <= (size_t(1) << (sizeof(size_t) * CHAR_BIT - 1)),
             "can't round up -- will overflow!");
  return size_t(1) << CeilingLog2(aValue);
}




template<typename T>
inline T
RotateLeft(const T aValue, uint_fast8_t aShift)
{
  MOZ_ASSERT(aShift < sizeof(T) * CHAR_BIT, "Shift value is too large!");
  static_assert(IsUnsigned<T>::value, "Rotates require unsigned values");
  return (aValue << aShift) | (aValue >> (sizeof(T) * CHAR_BIT - aShift));
}




template<typename T>
inline T
RotateRight(const T aValue, uint_fast8_t aShift)
{
  MOZ_ASSERT(aShift < sizeof(T) * CHAR_BIT, "Shift value is too large!");
  static_assert(IsUnsigned<T>::value, "Rotates require unsigned values");
  return (aValue >> aShift) | (aValue << (sizeof(T) * CHAR_BIT - aShift));
}

} 

#endif 
