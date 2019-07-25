


























#ifndef V8_CACHED_POWERS_H_
#define V8_CACHED_POWERS_H_

#include "diy-fp.h"

namespace v8 {
namespace internal {

struct CachedPower {
  uint64_t significand;
  int16_t binary_exponent;
  int16_t decimal_exponent;
};











#define GRISU_CACHE_STRUCT CachedPower
#define GRISU_CACHE_NAME(i) kCachedPowers##i
#define GRISU_CACHE_MAX_DISTANCE(i) kCachedPowersMaxDistance##i
#define GRISU_CACHE_OFFSET kCachedPowerOffset
#define GRISU_UINT64_C V8_2PART_UINT64_C

#include "powers-ten.h"  

static const double kD_1_LOG2_10 = 0.30102999566398114;  




#define COMPUTE_FOR_CACHE(i) \
  if (!found && (gamma - alpha + 1 >= GRISU_CACHE_MAX_DISTANCE(i))) {   \
    int kQ = DiyFp::kSignificandSize;                                   \
    double k = ceiling((alpha - e + kQ - 1) * kD_1_LOG2_10);            \
    int index = (GRISU_CACHE_OFFSET + static_cast<int>(k) - 1) / i + 1; \
    cached_power = GRISU_CACHE_NAME(i)[index];                          \
    found = true;                                                       \
  }                                                                     \

static void GetCachedPower(int e, int alpha, int gamma, int* mk, DiyFp* c_mk) {
  
  
  bool found = false;
  CachedPower cached_power;
  COMPUTE_FOR_CACHE(20);
  COMPUTE_FOR_CACHE(19);
  COMPUTE_FOR_CACHE(18);
  COMPUTE_FOR_CACHE(17);
  COMPUTE_FOR_CACHE(16);
  COMPUTE_FOR_CACHE(15);
  COMPUTE_FOR_CACHE(14);
  COMPUTE_FOR_CACHE(13);
  COMPUTE_FOR_CACHE(12);
  COMPUTE_FOR_CACHE(11);
  COMPUTE_FOR_CACHE(10);
  COMPUTE_FOR_CACHE(9);
  COMPUTE_FOR_CACHE(8);
  COMPUTE_FOR_CACHE(7);
  COMPUTE_FOR_CACHE(6);
  COMPUTE_FOR_CACHE(5);
  COMPUTE_FOR_CACHE(4);
  COMPUTE_FOR_CACHE(3);
  COMPUTE_FOR_CACHE(2);
  COMPUTE_FOR_CACHE(1);
  if (!found) {
    UNIMPLEMENTED();
    
    cached_power.significand = 0;
    cached_power.binary_exponent = 0;
    cached_power.decimal_exponent = 0;
  }
  *c_mk = DiyFp(cached_power.significand, cached_power.binary_exponent);
  *mk = cached_power.decimal_exponent;
  ASSERT((alpha <= c_mk->e() + e) && (c_mk->e() + e <= gamma));
}
#undef GRISU_REDUCTION
#undef GRISU_CACHE_STRUCT
#undef GRISU_CACHE_NAME
#undef GRISU_CACHE_MAX_DISTANCE
#undef GRISU_CACHE_OFFSET
#undef GRISU_UINT64_C

} }  

#endif  
