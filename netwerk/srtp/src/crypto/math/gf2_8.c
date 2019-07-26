














































#include "datatypes.h"
#include "gf2_8.h"



gf2_8
gf2_8_multiply(gf2_8 x, gf2_8 y) {
  gf2_8 z = 0;

  if (y &   1) z ^= x; x = gf2_8_shift(x);
  if (y &   2) z ^= x; x = gf2_8_shift(x);
  if (y &   4) z ^= x; x = gf2_8_shift(x);
  if (y &   8) z ^= x; x = gf2_8_shift(x);
  if (y &  16) z ^= x; x = gf2_8_shift(x);
  if (y &  32) z ^= x; x = gf2_8_shift(x);
  if (y &  64) z ^= x; x = gf2_8_shift(x);
  if (y & 128) z ^= x; 
  
  return z;
}




gf2_8
gf2_8_compute_inverse(gf2_8 x) {
  unsigned int i;

  if (x == 0) return 0;    
  for (i=0; i < 256; i++)
    if (gf2_8_multiply((gf2_8) i, x) == 1)
      return (gf2_8) i;

    return 0;
}

