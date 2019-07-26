







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

    static const unsigned ExponentBias = 127;
    static const unsigned ExponentShift = 23;

    static const Bits SignBit         = 0x80000000UL;
    static const Bits ExponentBits    = 0x7F800000UL;
    static const Bits SignificandBits = 0x007FFFFFUL;
};

struct DoubleTypeTraits
{
    typedef uint64_t Bits;

    static const unsigned ExponentBias = 1023;
    static const unsigned ExponentShift = 52;

    static const Bits SignBit         = 0x8000000000000000ULL;
    static const Bits ExponentBits    = 0x7ff0000000000000ULL;
    static const Bits SignificandBits = 0x000fffffffffffffULL;
};

template<typename T> struct SelectTrait;
template<> struct SelectTrait<float> : public FloatTypeTraits {};
template<> struct SelectTrait<double> : public DoubleTypeTraits {};


























template<typename T>
struct FloatingPoint : public SelectTrait<T>
{
    typedef SelectTrait<T> Base;
    typedef typename Base::Bits Bits;

    static_assert((Base::SignBit & Base::ExponentBits) == 0,
                  "sign bit shouldn't overlap exponent bits");
    static_assert((Base::SignBit & Base::SignificandBits) == 0,
                  "sign bit shouldn't overlap significand bits");
    static_assert((Base::ExponentBits & Base::SignificandBits) == 0,
                  "exponent bits shouldn't overlap significand bits");

    static_assert((Base::SignBit | Base::ExponentBits | Base::SignificandBits) ==
                  ~Bits(0),
                  "all bits accounted for");

    





    static_assert(sizeof(T) == sizeof(Bits), "Bits must be same size as T");
};


template<typename T>
static MOZ_ALWAYS_INLINE bool
IsNaN(T t)
{
  



  typedef FloatingPoint<T> Traits;
  typedef typename Traits::Bits Bits;
  Bits bits = BitwiseCast<Bits>(t);
  return (bits & Traits::ExponentBits) == Traits::ExponentBits &&
         (bits & Traits::SignificandBits) != 0;
}


template<typename T>
static MOZ_ALWAYS_INLINE bool
IsInfinite(T t)
{
  
  typedef FloatingPoint<T> Traits;
  typedef typename Traits::Bits Bits;
  Bits bits = BitwiseCast<Bits>(t);
  return (bits & ~Traits::SignBit) == Traits::ExponentBits;
}


template<typename T>
static MOZ_ALWAYS_INLINE bool
IsFinite(T t)
{
  



  typedef FloatingPoint<T> Traits;
  typedef typename Traits::Bits Bits;
  Bits bits = BitwiseCast<Bits>(t);
  return (bits & Traits::ExponentBits) != Traits::ExponentBits;
}





template<typename T>
static MOZ_ALWAYS_INLINE bool
IsNegative(T t)
{
  MOZ_ASSERT(!IsNaN(t), "NaN does not have a sign");

  
  typedef FloatingPoint<T> Traits;
  typedef typename Traits::Bits Bits;
  Bits bits = BitwiseCast<Bits>(t);
  return (bits & Traits::SignBit) != 0;
}


template<typename T>
static MOZ_ALWAYS_INLINE bool
IsNegativeZero(T t)
{
  
  typedef FloatingPoint<T> Traits;
  typedef typename Traits::Bits Bits;
  Bits bits = BitwiseCast<Bits>(t);
  return bits == Traits::SignBit;
}







template<typename T>
static MOZ_ALWAYS_INLINE int_fast16_t
ExponentComponent(T t)
{
  



  typedef FloatingPoint<T> Traits;
  typedef typename Traits::Bits Bits;
  Bits bits = BitwiseCast<Bits>(t);
  return int_fast16_t((bits & Traits::ExponentBits) >> Traits::ExponentShift) -
         int_fast16_t(Traits::ExponentBias);
}


template<typename T>
static MOZ_ALWAYS_INLINE T
PositiveInfinity()
{
  



  typedef FloatingPoint<T> Traits;
  return BitwiseCast<T>(Traits::ExponentBits);
}


template<typename T>
static MOZ_ALWAYS_INLINE T
NegativeInfinity()
{
  



  typedef FloatingPoint<T> Traits;
  return BitwiseCast<T>(Traits::SignBit | Traits::ExponentBits);
}



template<typename T>
static MOZ_ALWAYS_INLINE T
SpecificNaN(int signbit, typename FloatingPoint<T>::Bits significand)
{
  typedef FloatingPoint<T> Traits;
  MOZ_ASSERT(signbit == 0 || signbit == 1);
  MOZ_ASSERT((significand & ~Traits::SignificandBits) == 0);
  MOZ_ASSERT(significand & Traits::SignificandBits);

  T t = BitwiseCast<T>((signbit ? Traits::SignBit : 0) |
                       Traits::ExponentBits |
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
NumberEqualsInt32(T t, int32_t* i)
{
  





  return t == (*i = int32_t(t));
}








template<typename T>
static MOZ_ALWAYS_INLINE bool
NumberIsInt32(T t, int32_t* i)
{
  return !IsNegativeZero(t) && NumberEqualsInt32(t, i);
}





template<typename T>
static MOZ_ALWAYS_INLINE T
UnspecifiedNaN()
{
  





  typedef FloatingPoint<T> Traits;
  return SpecificNaN<T>(1, Traits::SignificandBits);
}






template<typename T>
static inline bool
NumbersAreIdentical(T t1, T t2)
{
  typedef FloatingPoint<T> Traits;
  typedef typename Traits::Bits Bits;
  if (IsNaN(t1))
    return IsNaN(t2);
  return BitwiseCast<Bits>(t1) == BitwiseCast<Bits>(t2);
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
FuzzyEqualsAdditive(T val1, T val2, T epsilon = detail::FuzzyEqualsEpsilon<T>::value())
{
  static_assert(IsFloatingPoint<T>::value, "floating point type required");
  return Abs(val1 - val2) <= epsilon;
}













template<typename T>
static MOZ_ALWAYS_INLINE bool
FuzzyEqualsMultiplicative(T val1, T val2, T epsilon = detail::FuzzyEqualsEpsilon<T>::value())
{
  static_assert(IsFloatingPoint<T>::value, "floating point type required");
  
  T smaller = Abs(val1) < Abs(val2) ? Abs(val1) : Abs(val2);
  return Abs(val1 - val2) <= epsilon * smaller;
}









MOZ_WARN_UNUSED_RESULT
extern MFBT_API bool
IsFloat32Representable(double x);

} 

#endif 
