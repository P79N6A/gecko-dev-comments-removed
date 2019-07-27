









#ifndef VP9_COMMON_VP9_MV_H_
#define VP9_COMMON_VP9_MV_H_

#include "vpx/vpx_integer.h"

#include "vp9/common/vp9_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mv {
  int16_t row;
  int16_t col;
} MV;

typedef union int_mv {
  uint32_t as_int;
  MV as_mv;
} int_mv; 

typedef struct mv32 {
  int32_t row;
  int32_t col;
} MV32;

static INLINE void clamp_mv(MV *mv, int min_col, int max_col,
                            int min_row, int max_row) {
  mv->col = clamp(mv->col, min_col, max_col);
  mv->row = clamp(mv->row, min_row, max_row);
}

#ifdef __cplusplus
}  
#endif

#endif
