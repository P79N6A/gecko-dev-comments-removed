






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main.h"


void silk_stereo_decode_pred(
    ec_dec                      *psRangeDec,                    
    opus_int32                  pred_Q13[]                      
)
{
    opus_int   n, ix[ 2 ][ 3 ];
    opus_int32 low_Q13, step_Q13;

    
    n = ec_dec_icdf( psRangeDec, silk_stereo_pred_joint_iCDF, 8 );
    ix[ 0 ][ 2 ] = silk_DIV32_16( n, 5 );
    ix[ 1 ][ 2 ] = n - 5 * ix[ 0 ][ 2 ];
    for( n = 0; n < 2; n++ ) {
        ix[ n ][ 0 ] = ec_dec_icdf( psRangeDec, silk_uniform3_iCDF, 8 );
        ix[ n ][ 1 ] = ec_dec_icdf( psRangeDec, silk_uniform5_iCDF, 8 );
    }

    
    for( n = 0; n < 2; n++ ) {
        ix[ n ][ 0 ] += 3 * ix[ n ][ 2 ];
        low_Q13 = silk_stereo_pred_quant_Q13[ ix[ n ][ 0 ] ];
        step_Q13 = silk_SMULWB( silk_stereo_pred_quant_Q13[ ix[ n ][ 0 ] + 1 ] - low_Q13,
            SILK_FIX_CONST( 0.5 / STEREO_QUANT_SUB_STEPS, 16 ) );
        pred_Q13[ n ] = silk_SMLABB( low_Q13, step_Q13, 2 * ix[ n ][ 1 ] + 1 );
    }

    
    pred_Q13[ 0 ] -= pred_Q13[ 1 ];
}


void silk_stereo_decode_mid_only(
    ec_dec                      *psRangeDec,                    
    opus_int                    *decode_only_mid                
)
{
    
    *decode_only_mid = ec_dec_icdf( psRangeDec, silk_stereo_only_code_mid_iCDF, 8 );
}
