









#include <immintrin.h>  
#include "vp9/common/vp9_idct.h"  
#include "vpx_ports/mem.h"


#define FDCT32x32_2D_AVX2 vp9_fdct32x32_rd_avx2
#define FDCT32x32_HIGH_PRECISION 0
#include "vp9/encoder/x86/vp9_dct32x32_avx2_impl.h"
#undef  FDCT32x32_2D_AVX2
#undef  FDCT32x32_HIGH_PRECISION

#define FDCT32x32_2D_AVX2 vp9_fdct32x32_avx2
#define FDCT32x32_HIGH_PRECISION 1
#include "vp9/encoder/x86/vp9_dct32x32_avx2_impl.h" 
#undef  FDCT32x32_2D_AVX2
#undef  FDCT32x32_HIGH_PRECISION
