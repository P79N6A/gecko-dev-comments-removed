













































#ifndef GF2_8_H
#define GF2_8_H

#include "datatypes.h"  

typedef uint8_t gf2_8;

#define gf2_8_field_polynomial 0x1B












#define gf2_8_shift(z) (((z) & 128) ? \
       (((z) << 1) ^ gf2_8_field_polynomial) : ((z) << 1))

gf2_8
gf2_8_compute_inverse(gf2_8 x);

void
test_gf2_8(void);

gf2_8
gf2_8_multiply(gf2_8 x, gf2_8 y);

#endif 
