







#ifndef mozilla_FloatingPoint_h
#define mozilla_FloatingPoint_h

#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/Casting.h"
#include "mozilla/MathAlgorithms.h"
#include "mozilla/Types.h"

#include <stdint.h>

namespace mozilla {
















struct FloatTypeTraits
{
  typedef uint32_t Bits;

  static const unsigned kExponentBias = 127;
  static const unsigned kExponentShift = 23;

  static const Bits kSignBit         = 0x80000000UL;
  static const Bits kExponentBits    = 0x7F800000UL;
  static const Bits kSignificandBits = 0x007FFFFFUL;
};

struct DoubleTypeTraits
{
  typedef uint64_t Bits;

  static const unsigned kExponentBias = 1023;
  static const unsigned kExponentShift = 52;

  static const Bits kSignBit         = 0x8000000000000000ULL;
  static const Bits kExponentBits    = 0x7ff0000000000000ULL;
  static const Bits kSignificandBits = 0x000fffffffffffffULL;
};

template<typename T> struct SelectTrait;
template<> struct SelectTrait<float> : public FloatTypeTraits {};
template<> struct SelectTrait<double> : public DoubleTypeTraits {};



























template<typename T>
struct FloatingPoint : public SelectTrait<T>
{
  typedef SelectTrait<T> Base;
  typedef typename Base::Bits Bits;

  static_assert((Base::kSignBit & Base::kExponentBits) == 0,
                "sign bit shouldn't overlap exponent bits");
  static_assert((Base::kSignBit & Base::kSignificandBits) == 0,
                "sign bit shouldn't overlap significand bits");
  static_assert((Base::kExponentBits & Base::kSignificandBits) == 0,
                "exponent bits shouldn't overlap significand bits");

  static_assert((Base::kSignBit | Base::kExponentBits | Base::kSignificandBits) ==
                ~Bits(0),
                "all bits accounted for");

  






  static_assert(sizeof(T) == sizeof(Bits), "Bits must be same size as T");
};


template<typename T>
static MOZ_ALWAYS_INLINE bool
IsNaN(T aValue)
{
  



  typedef FloatingPoint<T> Traits;
  typedef typename Traits::Bits Bits;
  Bits bits = BitwiseCast<Bits>(aValue);
  return (bits & Traits::kExponentBits) == Traits::kExponentBits &&
         (bits & Traits::kSignificandBits) != 0;
}


template<typename T>
static MOZ_ALWAYS_INLINE bool
IsInfinite(T aValue)
{
  
  typedef FloatingPoint<T> Traits;
  typedef typename Traits::Bits Bits;
  Bits bits = BitwiseCast<Bits>(aValue);
  return (bits & ~Traits::kSignBit) == Traits::kExponentBits;
}


template<typename T>
static MOZ_ALWAYS_INLINE bool
IsFinite(T aValue)
{
  



  typedef FloatingPoint<T> Traits;
  typedef typename Traits::Bits Bits;
  Bits bits = BitwiseCast<Bits>(aValue);
  return (bits & Traits::kExponentBits) != Traits::kExponentBits;
}





template<typename T>
static MOZ_ALWAYS_INLINE bool
IsNegative(T aValue)
{
  MOZ_ASSERT(!IsNaN(aValue), "NaN does not have a sign");

  
  typedef FloatingPoint<T> Traits;
  typedef typename Traits::Bits Bits;
  Bits bits = BitwiseCast<Bits>(aValue);
  return (bits & Traits::kSignBit) != 0;
}


template<typename T>
static MOZ_ALWAYS_INLINE bool
IsNegativeZero(T aValue)
{
  
  typedef FloatingPoint<T> Traits;
  typedef typename Traits::Bits Bits;
  Bits bits = BitwiseCast<Bits>(aValue);
  return bits == Traits::kSignBit;
}





template<typename T>
static MOZ_ALWAYS_INLINE T
ToZeroIfNonfinite(T aValue)
{
  return IsFinite(aValue) ? aValue : 0;
}







template<typename T>
static MOZ_ALWAYS_INLINE int_fast16_t
ExponentComponent(T aValue)
{
  



  typedef FloatingPoint<T> Traits;
  typedef typename Traits::Bits Bits;
  Bits bits = BitwiseCast<Bits>(aValue);
  return int_fast16_t((bits & Traits::kExponentBits) >> Traits::kExponentShift) -
         int_fast16_t(Traits::kExponentBias);
}


template<typename T>
static MOZ_ALWAYS_INLINE T
PositiveInfinity()
{
  



  typedef FloatingPoint<T> Traits;
  return BitwiseCast<T>(Traits::kExponentBits);
}


template<typename T>
static MOZ_ALWAYS_INLINE T
NegativeInfinity()
{
  



  typedef FloatingPoint<T> Traits;
  return BitwiseCast<T>(Traits::kSignBit | Traits::kExponentBits);
}



template<typename T>
static MOZ_ALWAYS_INLINE T
SpecificNaN(int signbit, typename FloatingPoint<T>::Bits significand)
{
  typedef FloatingPoint<T> Traits;
  MOZ_ASSERT(signbit == 0 || signbit == 1);
  MOZ_ASSERT((significand & ~Traits::kSignificandBits) == 0);
  MOZ_ASSERT(significand & Traits::kSignificandBits);

  T t = BitwiseCast<T>((signbit ? Traits::kSignBit : 0) |
                       Traits::kExponentBits |
                       significand);
  MOZ_ASSERT(IsNaN(t));
  return t;
}


template<typename T>
static MOZ_ALWAYS_INLINE T
MinNumberValue()
{
  typedef FloatingPoint<T> Traits;
  typedef typename Traits::Bits Bits;
  return BitwiseCast<T>(Bits(1));
}








template<typename T>
static MOZ_ALWAYS_INLINE bool
NumberEqualsInt32(T aValue, int32_t* aInt32)
{
  





  return aValue == (*aInt32 = int32_t(aValue));
}








template<typename T>
static MOZ_ALWAYS_INLINE bool
NumberIsInt32(T aValue, int32_t* aInt32)
{
  return !IsNegativeZero(aValue) && NumberEqualsInt32(aValue, aInt32);
}





template<typename T>
static MOZ_ALWAYS_INLINE T
UnspecifiedNaN()
{
  





  typedef FloatingPoint<T> Traits;
  return SpecificNaN<T>(1, Traits::kSignificandBits);
}






template<typename T>
static inline bool
NumbersAreIdentical(T aValue1, T aValue2)
{
  typedef FloatingPoint<T> Traits;
  typedef typename Traits::Bits Bits;
  if (IsNaN(aValue1)) {
    return IsNaN(aValue2);
  }
  return BitwiseCast<Bits>(aValue1) == BitwiseCast<Bits>(aValue2);
}

namespace detail {

template<typename T>
struct FuzzyEqualsEpsilon;

template<>
struct FuzzyEqualsEpsilon<float>
{
  
  static float value() { return 1.0f / (1 << 17); }
};

template<>
struct FuzzyEqualsEpsilon<double>
{
  
  static double value() { return 1.0 / (1LL << 40); }
};

} 













template<typename T>
static MOZ_ALWAYS_INLINE bool
FuzzyEqualsAdditive(T aValue1, T aValue2,
                    T aEpsilon = detail::FuzzyEqualsEpsilon<T>::value())
{
  static_assert(IsFloatingPoint<T>::value, "floating point type required");
  return Abs(aValue1 - aValue2) <= aEpsilon;
}













template<typename T>
static MOZ_ALWAYS_INLINE bool
FuzzyEqualsMultiplicative(T aValue1, T aValue2,
                          T aEpsilon = detail::FuzzyEqualsEpsilon<T>::value())
{
  static_assert(IsFloatingPoint<T>::value, "floating point type required");
  
  T smaller = Abs(aValue1) < Abs(aValue2) ? Abs(aValue1) : Abs(aValue2);
  return Abs(aValue1 - aValue2) <= aEpsilon * smaller;
}









MOZ_WARN_UNUSED_RESULT
extern MFBT_API bool
IsFloat32Representable(double aFloat32);

} 

#endif 
