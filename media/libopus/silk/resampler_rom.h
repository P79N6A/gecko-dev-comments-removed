






























#ifndef SILK_FIX_RESAMPLER_ROM_H
#define SILK_FIX_RESAMPLER_ROM_H

#ifdef  __cplusplus
extern "C"
{
#endif

#include "typedef.h"
#include "resampler_structs.h"

#define RESAMPLER_DOWN_ORDER_FIR0               18
#define RESAMPLER_DOWN_ORDER_FIR1               24
#define RESAMPLER_DOWN_ORDER_FIR2               36
#define RESAMPLER_ORDER_FIR_12                  8


extern const opus_int16 silk_resampler_down2_0;
extern const opus_int16 silk_resampler_down2_1;


extern const opus_int16 silk_resampler_up2_hq_0[ 3 ];
extern const opus_int16 silk_resampler_up2_hq_1[ 3 ];


extern const opus_int16 silk_Resampler_3_4_COEFS[ 2 + 3 * RESAMPLER_DOWN_ORDER_FIR0 / 2 ];
extern const opus_int16 silk_Resampler_2_3_COEFS[ 2 + 2 * RESAMPLER_DOWN_ORDER_FIR0 / 2 ];
extern const opus_int16 silk_Resampler_1_2_COEFS[ 2 +     RESAMPLER_DOWN_ORDER_FIR1 / 2 ];
extern const opus_int16 silk_Resampler_1_3_COEFS[ 2 +     RESAMPLER_DOWN_ORDER_FIR2 / 2 ];
extern const opus_int16 silk_Resampler_1_4_COEFS[ 2 +     RESAMPLER_DOWN_ORDER_FIR2 / 2 ];
extern const opus_int16 silk_Resampler_1_6_COEFS[ 2 +     RESAMPLER_DOWN_ORDER_FIR2 / 2 ];
extern const opus_int16 silk_Resampler_2_3_COEFS_LQ[ 2 + 2 * 2 ];


extern const opus_int16 silk_resampler_frac_FIR_12[ 12 ][ RESAMPLER_ORDER_FIR_12 / 2 ];

#ifdef  __cplusplus
}
#endif

#endif
