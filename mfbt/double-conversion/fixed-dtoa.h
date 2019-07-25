


























#ifndef DOUBLE_CONVERSION_FIXED_DTOA_H_
#define DOUBLE_CONVERSION_FIXED_DTOA_H_

#include "utils.h"

namespace double_conversion {

















bool FastFixedDtoa(double v, int fractional_count,
                   Vector<char> buffer, int* length, int* decimal_point);

}  

#endif  
