






#ifndef mozilla_FloatingPoint_h_
#define mozilla_FloatingPoint_h_

#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/StandardInteger.h"


























MOZ_STATIC_ASSERT(sizeof(double) == sizeof(uint64_t), "double must be 64 bits");





#define MOZ_DOUBLE_SIGN_BIT          0x8000000000000000ULL
#define MOZ_DOUBLE_EXPONENT_BITS     0x7ff0000000000000ULL
#define MOZ_DOUBLE_SIGNIFICAND_BITS  0x000fffffffffffffULL

#define MOZ_DOUBLE_EXPONENT_BIAS   1023
#define MOZ_DOUBLE_EXPONENT_SHIFT  52

MOZ_STATIC_ASSERT((MOZ_DOUBLE_SIGN_BIT & MOZ_DOUBLE_EXPONENT_BITS) == 0,
                  "sign bit doesn't overlap exponent bits");
MOZ_STATIC_ASSERT((MOZ_DOUBLE_SIGN_BIT & MOZ_DOUBLE_SIGNIFICAND_BITS) == 0,
                  "sign bit doesn't overlap significand bits");
MOZ_STATIC_ASSERT((MOZ_DOUBLE_EXPONENT_BITS & MOZ_DOUBLE_SIGNIFICAND_BITS) == 0,
                  "exponent bits don't overlap significand bits");

MOZ_STATIC_ASSERT((MOZ_DOUBLE_SIGN_BIT | MOZ_DOUBLE_EXPONENT_BITS | MOZ_DOUBLE_SIGNIFICAND_BITS)
                  == ~(uint64_t)0,
                  "all bits accounted for");

#ifdef __cplusplus
extern "C" {
#endif





union MozDoublePun {
    





    uint64_t u;
    double d;
};


static MOZ_ALWAYS_INLINE int
MOZ_DOUBLE_IS_NaN(double d)
{
  union MozDoublePun pun;
  pun.d = d;

  



  return (pun.u & MOZ_DOUBLE_EXPONENT_BITS) == MOZ_DOUBLE_EXPONENT_BITS &&
         (pun.u & MOZ_DOUBLE_SIGNIFICAND_BITS) != 0;
}


static MOZ_ALWAYS_INLINE int
MOZ_DOUBLE_IS_INFINITE(double d)
{
  union MozDoublePun pun;
  pun.d = d;

  
  return (pun.u & ~MOZ_DOUBLE_SIGN_BIT) == MOZ_DOUBLE_EXPONENT_BITS;
}


static MOZ_ALWAYS_INLINE int
MOZ_DOUBLE_IS_FINITE(double d)
{
  union MozDoublePun pun;
  pun.d = d;

  



  return (pun.u & MOZ_DOUBLE_EXPONENT_BITS) != MOZ_DOUBLE_EXPONENT_BITS;
}





static MOZ_ALWAYS_INLINE int
MOZ_DOUBLE_IS_NEGATIVE(double d)
{
  union MozDoublePun pun;
  pun.d = d;

  MOZ_ASSERT(!MOZ_DOUBLE_IS_NaN(d), "NaN does not have a sign");

  
  return (pun.u & MOZ_DOUBLE_SIGN_BIT) != 0;
}


static MOZ_ALWAYS_INLINE int
MOZ_DOUBLE_IS_NEGATIVE_ZERO(double d)
{
  union MozDoublePun pun;
  pun.d = d;

  
  return pun.u == MOZ_DOUBLE_SIGN_BIT;
}


static MOZ_ALWAYS_INLINE int_fast16_t
MOZ_DOUBLE_EXPONENT(double d)
{
  union MozDoublePun pun;
  pun.d = d;

  



  return (int_fast16_t)((pun.u & MOZ_DOUBLE_EXPONENT_BITS) >> MOZ_DOUBLE_EXPONENT_SHIFT) -
                        MOZ_DOUBLE_EXPONENT_BIAS;
}


static MOZ_ALWAYS_INLINE double
MOZ_DOUBLE_POSITIVE_INFINITY()
{
  union MozDoublePun pun;

  



  pun.u = MOZ_DOUBLE_EXPONENT_BITS;
  return pun.d;
}


static MOZ_ALWAYS_INLINE double
MOZ_DOUBLE_NEGATIVE_INFINITY()
{
  union MozDoublePun pun;

  



  pun.u = MOZ_DOUBLE_SIGN_BIT | MOZ_DOUBLE_EXPONENT_BITS;
  return pun.d;
}


static MOZ_ALWAYS_INLINE double
MOZ_DOUBLE_SPECIFIC_NaN(int signbit, uint64_t significand)
{
  union MozDoublePun pun;

  MOZ_ASSERT(signbit == 0 || signbit == 1);
  MOZ_ASSERT((significand & ~MOZ_DOUBLE_SIGNIFICAND_BITS) == 0);
  MOZ_ASSERT(significand & MOZ_DOUBLE_SIGNIFICAND_BITS);

  pun.u = (signbit ? MOZ_DOUBLE_SIGN_BIT : 0) |
          MOZ_DOUBLE_EXPONENT_BITS |
          significand;
  MOZ_ASSERT(MOZ_DOUBLE_IS_NaN(pun.d));
  return pun.d;
}





static MOZ_ALWAYS_INLINE double
MOZ_DOUBLE_NaN()
{
  return MOZ_DOUBLE_SPECIFIC_NaN(0, 0xfffffffffffffULL);
}


static MOZ_ALWAYS_INLINE double
MOZ_DOUBLE_MIN_VALUE()
{
  union MozDoublePun pun;
  pun.u = 1;
  return pun.d;
}


static MOZ_ALWAYS_INLINE uint32_t
MOZ_HASH_DOUBLE(double d)
{
  union MozDoublePun pun;
  pun.d = d;

  return ((uint32_t)(pun.u >> 32)) ^ ((uint32_t)(pun.u));
}

static MOZ_ALWAYS_INLINE int
MOZ_DOUBLE_IS_INT32(double d, int32_t* i)
{
  




  return !MOZ_DOUBLE_IS_NEGATIVE_ZERO(d) && d == (*i = (int32_t)d);
}

#ifdef __cplusplus
} 
#endif

#endif
