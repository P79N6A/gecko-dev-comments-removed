






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main.h"


void silk_stereo_encode_pred(
    ec_enc                      *psRangeEnc,                    
    opus_int8                   ix[ 2 ][ 3 ]                    
)
{
    opus_int   n;

    
    n = 5 * ix[ 0 ][ 2 ] + ix[ 1 ][ 2 ];
    silk_assert( n < 25 );
    ec_enc_icdf( psRangeEnc, n, silk_stereo_pred_joint_iCDF, 8 );
    for( n = 0; n < 2; n++ ) {
        silk_assert( ix[ n ][ 0 ] < 3 );
        silk_assert( ix[ n ][ 1 ] < STEREO_QUANT_SUB_STEPS );
        ec_enc_icdf( psRangeEnc, ix[ n ][ 0 ], silk_uniform3_iCDF, 8 );
        ec_enc_icdf( psRangeEnc, ix[ n ][ 1 ], silk_uniform5_iCDF, 8 );
    }
}


void silk_stereo_encode_mid_only(
    ec_enc                      *psRangeEnc,                    
    opus_int8                   mid_only_flag
)
{
    
    ec_enc_icdf( psRangeEnc, mid_only_flag, silk_stereo_only_code_mid_iCDF, 8 );
}
