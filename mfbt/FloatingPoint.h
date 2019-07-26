






#ifndef mozilla_FloatingPoint_h_
#define mozilla_FloatingPoint_h_

#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/StandardInteger.h"

namespace mozilla {






















MOZ_STATIC_ASSERT(sizeof(double) == sizeof(uint64_t), "double must be 64 bits");

const unsigned DoubleExponentBias = 1023;
const unsigned DoubleExponentShift = 52;

namespace detail {

const uint64_t DoubleSignBit         = 0x8000000000000000ULL;
const uint64_t DoubleExponentBits    = 0x7ff0000000000000ULL;
const uint64_t DoubleSignificandBits = 0x000fffffffffffffULL;

MOZ_STATIC_ASSERT((DoubleSignBit & DoubleExponentBits) == 0,
                  "sign bit doesn't overlap exponent bits");
MOZ_STATIC_ASSERT((DoubleSignBit & DoubleSignificandBits) == 0,
                  "sign bit doesn't overlap significand bits");
MOZ_STATIC_ASSERT((DoubleExponentBits & DoubleSignificandBits) == 0,
                  "exponent bits don't overlap significand bits");

MOZ_STATIC_ASSERT((DoubleSignBit | DoubleExponentBits | DoubleSignificandBits) ==
                  ~uint64_t(0),
                  "all bits accounted for");

union DoublePun
{
    





    uint64_t u;
    double d;
};

} 


static MOZ_ALWAYS_INLINE bool
IsNaN(double d)
{
  union detail::DoublePun pun;
  pun.d = d;

  



  return (pun.u & detail::DoubleExponentBits) == detail::DoubleExponentBits &&
         (pun.u & detail::DoubleSignificandBits) != 0;
}


static MOZ_ALWAYS_INLINE bool
IsInfinite(double d)
{
  union detail::DoublePun pun;
  pun.d = d;

  
  return (pun.u & ~detail::DoubleSignBit) == detail::DoubleExponentBits;
}


static MOZ_ALWAYS_INLINE bool
IsFinite(double d)
{
  union detail::DoublePun pun;
  pun.d = d;

  



  return (pun.u & detail::DoubleExponentBits) != detail::DoubleExponentBits;
}





static MOZ_ALWAYS_INLINE bool
IsNegative(double d)
{
  MOZ_ASSERT(!IsNaN(d), "NaN does not have a sign");

  union detail::DoublePun pun;
  pun.d = d;

  
  return (pun.u & detail::DoubleSignBit) != 0;
}


static MOZ_ALWAYS_INLINE bool
IsNegativeZero(double d)
{
  union detail::DoublePun pun;
  pun.d = d;

  
  return pun.u == detail::DoubleSignBit;
}


static MOZ_ALWAYS_INLINE int_fast16_t
ExponentComponent(double d)
{
  union detail::DoublePun pun;
  pun.d = d;

  



  return int_fast16_t((pun.u & detail::DoubleExponentBits) >> DoubleExponentShift) -
         int_fast16_t(DoubleExponentBias);
}


static MOZ_ALWAYS_INLINE double
PositiveInfinity()
{
  union detail::DoublePun pun;

  



  pun.u = detail::DoubleExponentBits;
  return pun.d;
}


static MOZ_ALWAYS_INLINE double
NegativeInfinity()
{
  union detail::DoublePun pun;

  



  pun.u = detail::DoubleSignBit | detail::DoubleExponentBits;
  return pun.d;
}


static MOZ_ALWAYS_INLINE double
SpecificNaN(int signbit, uint64_t significand)
{
  MOZ_ASSERT(signbit == 0 || signbit == 1);
  MOZ_ASSERT((significand & ~detail::DoubleSignificandBits) == 0);
  MOZ_ASSERT(significand & detail::DoubleSignificandBits);

  union detail::DoublePun pun;
  pun.u = (signbit ? detail::DoubleSignBit : 0) |
          detail::DoubleExponentBits |
          significand;
  MOZ_ASSERT(IsNaN(pun.d));
  return pun.d;
}


static MOZ_ALWAYS_INLINE double
MinDoubleValue()
{
  union detail::DoublePun pun;
  pun.u = 1;
  return pun.d;
}

static MOZ_ALWAYS_INLINE bool
DoubleIsInt32(double d, int32_t* i)
{
  




  return !IsNegativeZero(d) && d == (*i = int32_t(d));
}





static MOZ_ALWAYS_INLINE double
UnspecifiedNaN()
{
  return mozilla::SpecificNaN(0, 0xfffffffffffffULL);
}

} 

#endif  
