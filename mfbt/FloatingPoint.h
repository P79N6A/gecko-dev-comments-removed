







#ifndef mozilla_FloatingPoint_h
#define mozilla_FloatingPoint_h

#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/Casting.h"
#include "mozilla/Types.h"

#include <stdint.h>

namespace mozilla {






















static_assert(sizeof(double) == sizeof(uint64_t), "double must be 64 bits");

const unsigned DoubleExponentBias = 1023;
const unsigned DoubleExponentShift = 52;

const uint64_t DoubleSignBit         = 0x8000000000000000ULL;
const uint64_t DoubleExponentBits    = 0x7ff0000000000000ULL;
const uint64_t DoubleSignificandBits = 0x000fffffffffffffULL;

static_assert((DoubleSignBit & DoubleExponentBits) == 0,
              "sign bit doesn't overlap exponent bits");
static_assert((DoubleSignBit & DoubleSignificandBits) == 0,
              "sign bit doesn't overlap significand bits");
static_assert((DoubleExponentBits & DoubleSignificandBits) == 0,
              "exponent bits don't overlap significand bits");

static_assert((DoubleSignBit | DoubleExponentBits | DoubleSignificandBits) ==
              ~uint64_t(0),
              "all bits accounted for");





static_assert(sizeof(float) == sizeof(uint32_t), "float must be 32bits");

const unsigned FloatExponentBias = 127;
const unsigned FloatExponentShift = 23;

const uint32_t FloatSignBit         = 0x80000000UL;
const uint32_t FloatExponentBits    = 0x7F800000UL;
const uint32_t FloatSignificandBits = 0x007FFFFFUL;

static_assert((FloatSignBit & FloatExponentBits) == 0,
              "sign bit doesn't overlap exponent bits");
static_assert((FloatSignBit & FloatSignificandBits) == 0,
              "sign bit doesn't overlap significand bits");
static_assert((FloatExponentBits & FloatSignificandBits) == 0,
              "exponent bits don't overlap significand bits");

static_assert((FloatSignBit | FloatExponentBits | FloatSignificandBits) ==
              ~uint32_t(0),
              "all bits accounted for");


static MOZ_ALWAYS_INLINE bool
IsNaN(double d)
{
  



  uint64_t bits = BitwiseCast<uint64_t>(d);
  return (bits & DoubleExponentBits) == DoubleExponentBits &&
         (bits & DoubleSignificandBits) != 0;
}


static MOZ_ALWAYS_INLINE bool
IsInfinite(double d)
{
  
  uint64_t bits = BitwiseCast<uint64_t>(d);
  return (bits & ~DoubleSignBit) == DoubleExponentBits;
}


static MOZ_ALWAYS_INLINE bool
IsFinite(double d)
{
  



  uint64_t bits = BitwiseCast<uint64_t>(d);
  return (bits & DoubleExponentBits) != DoubleExponentBits;
}





static MOZ_ALWAYS_INLINE bool
IsNegative(double d)
{
  MOZ_ASSERT(!IsNaN(d), "NaN does not have a sign");

  
  uint64_t bits = BitwiseCast<uint64_t>(d);
  return (bits & DoubleSignBit) != 0;
}


static MOZ_ALWAYS_INLINE bool
IsNegativeZero(double d)
{
  
  uint64_t bits = BitwiseCast<uint64_t>(d);
  return bits == DoubleSignBit;
}







static MOZ_ALWAYS_INLINE int_fast16_t
ExponentComponent(double d)
{
  



  uint64_t bits = BitwiseCast<uint64_t>(d);
  return int_fast16_t((bits & DoubleExponentBits) >> DoubleExponentShift) -
         int_fast16_t(DoubleExponentBias);
}


static MOZ_ALWAYS_INLINE double
PositiveInfinity()
{
  



  return BitwiseCast<double>(DoubleExponentBits);
}


static MOZ_ALWAYS_INLINE double
NegativeInfinity()
{
  



  return BitwiseCast<double>(DoubleSignBit | DoubleExponentBits);
}


static MOZ_ALWAYS_INLINE double
SpecificNaN(int signbit, uint64_t significand)
{
  MOZ_ASSERT(signbit == 0 || signbit == 1);
  MOZ_ASSERT((significand & ~DoubleSignificandBits) == 0);
  MOZ_ASSERT(significand & DoubleSignificandBits);

  double d = BitwiseCast<double>((signbit ? DoubleSignBit : 0) |
                                 DoubleExponentBits |
                                 significand);
  MOZ_ASSERT(IsNaN(d));
  return d;
}


static MOZ_ALWAYS_INLINE double
MinDoubleValue()
{
  return BitwiseCast<double>(uint64_t(1));
}

static MOZ_ALWAYS_INLINE bool
DoubleIsInt32(double d, int32_t* i)
{
  




  return !IsNegativeZero(d) && d == (*i = int32_t(d));
}





static MOZ_ALWAYS_INLINE double
UnspecifiedNaN()
{
  





  return SpecificNaN(1, 0xfffffffffffffULL);
}






static inline bool
DoublesAreIdentical(double d1, double d2)
{
  if (IsNaN(d1))
    return IsNaN(d2);
  return BitwiseCast<uint64_t>(d1) == BitwiseCast<uint64_t>(d2);
}


static MOZ_ALWAYS_INLINE bool
IsFloatNaN(float f)
{
  



  uint32_t bits = BitwiseCast<uint32_t>(f);
  return (bits & FloatExponentBits) == FloatExponentBits &&
         (bits & FloatSignificandBits) != 0;
}


static MOZ_ALWAYS_INLINE float
SpecificFloatNaN(int signbit, uint32_t significand)
{
  MOZ_ASSERT(signbit == 0 || signbit == 1);
  MOZ_ASSERT((significand & ~FloatSignificandBits) == 0);
  MOZ_ASSERT(significand & FloatSignificandBits);

  float f = BitwiseCast<float>((signbit ? FloatSignBit : 0) |
                                 FloatExponentBits |
                                 significand);
  MOZ_ASSERT(IsFloatNaN(f));
  return f;
}









MOZ_WARN_UNUSED_RESULT
extern MFBT_API bool
IsFloat32Representable(double x);

} 

#endif 
