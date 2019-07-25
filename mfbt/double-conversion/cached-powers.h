


























#ifndef DOUBLE_CONVERSION_CACHED_POWERS_H_
#define DOUBLE_CONVERSION_CACHED_POWERS_H_

#include "diy-fp.h"

namespace double_conversion {

class PowersOfTenCache {
 public:

  
  
  static const int kDecimalExponentDistance;

  static const int kMinDecimalExponent;
  static const int kMaxDecimalExponent;

  
  
  static void GetCachedPowerForBinaryExponentRange(int min_exponent,
                                                   int max_exponent,
                                                   DiyFp* power,
                                                   int* decimal_exponent);

  
  
  
  
  
  static void GetCachedPowerForDecimalExponent(int requested_exponent,
                                               DiyFp* power,
                                               int* found_exponent);
};

}  

#endif  
