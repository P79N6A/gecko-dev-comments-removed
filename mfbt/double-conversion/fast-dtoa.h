


























#ifndef DOUBLE_CONVERSION_FAST_DTOA_H_
#define DOUBLE_CONVERSION_FAST_DTOA_H_

#include "utils.h"

namespace double_conversion {

enum FastDtoaMode {
  
  
  
  FAST_DTOA_SHORTEST,
  
  FAST_DTOA_SHORTEST_SINGLE,
  
  
  FAST_DTOA_PRECISION
};



static const int kFastDtoaMaximalLength = 17;

static const int kFastDtoaMaximalSingleLength = 9;



























bool FastDtoa(double d,
              FastDtoaMode mode,
              int requested_digits,
              Vector<char> buffer,
              int* length,
              int* decimal_point);

}  

#endif  
