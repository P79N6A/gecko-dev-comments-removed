


























#ifndef DOUBLE_CONVERSION_BIGNUM_DTOA_H_
#define DOUBLE_CONVERSION_BIGNUM_DTOA_H_

#include "utils.h"

namespace double_conversion {

enum BignumDtoaMode {
  
  
  
  BIGNUM_DTOA_SHORTEST,
  
  BIGNUM_DTOA_SHORTEST_SINGLE,
  
  
  
  BIGNUM_DTOA_FIXED,
  
  BIGNUM_DTOA_PRECISION
};






























void BignumDtoa(double v, BignumDtoaMode mode, int requested_digits,
                Vector<char> buffer, int* length, int* point);

}  

#endif  
