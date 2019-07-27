






















#ifndef _QCMS_TRANSFORM_UTIL_H
#define _QCMS_TRANSFORM_UTIL_H

#include <stdlib.h>

#define CLU(table,x,y,z) table[(x*len + y*x_len + z*xy_len)*3]


typedef uint16_t uint16_fract_t;

float lut_interp_linear(double value, uint16_t *table, int length);
float lut_interp_linear_float(float value, float *table, int length);
uint16_t lut_interp_linear16(uint16_t input_value, uint16_t *table, int length);


static inline float lerp(float a, float b, float t)
{
        return a*(1.f-t) + b*t;
}

static inline unsigned char clamp_u8(float v)
{
  if (v > 255.)
    return 255;
  else if (v < 0)
    return 0;
  else
    return floorf(v+.5);
}

static inline float clamp_float(float a)
{
  











  if (a > 1.)
    return 1.;
  else if (a >= 0)
    return a;
  else 
    return 0;
}

static inline float u8Fixed8Number_to_float(uint16_t x)
{
  
  
  
  return x/256.;
}

float *build_input_gamma_table(struct curveType *TRC);
struct matrix build_colorant_matrix(qcms_profile *p);
void build_output_lut(struct curveType *trc,
                      uint16_t **output_gamma_lut, size_t *output_gamma_lut_length);

struct matrix matrix_invert(struct matrix mat);
qcms_bool compute_precache(struct curveType *trc, uint8_t *output);


#ifdef  __cplusplus
extern "C" {
#endif

uint16_fract_t lut_inverse_interp16(uint16_t Value, uint16_t LutTable[], int length);

#ifdef  __cplusplus
}
#endif

#endif
