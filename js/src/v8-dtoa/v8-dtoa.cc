





























#include <math.h>

#include "v8.h"
#include "dtoa.h"
#include "double.h"
#include "fast-dtoa.h"

namespace v8 {
namespace internal {

bool DoubleToAscii(double v, DtoaMode mode, int requested_digits,
                   Vector<char> buffer, int* sign, int* length, int* point) {
  ASSERT(!Double(v).IsSpecial());
  ASSERT(mode == DTOA_SHORTEST || requested_digits >= 0);

  if (Double(v).Sign() < 0) {
    *sign = 1;
    v = -v;
  } else {
    *sign = 0;
  }

  if (v == 0) {
    buffer[0] = '0';
    buffer[1] = '\0';
    *length = 1;
    *point = 1;
    return true;
  }

  if (mode == DTOA_PRECISION && requested_digits == 0) {
    buffer[0] = '\0';
    *length = 0;
    return true;
  }

  switch (mode) {
    case DTOA_SHORTEST:
      return FastDtoa(v, buffer, length, point);
    case DTOA_FIXED:
      
      
    default:
      break;
  }
  return false;
}

} }  
