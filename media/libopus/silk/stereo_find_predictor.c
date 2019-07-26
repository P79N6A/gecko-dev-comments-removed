






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main.h"


opus_int32 silk_stereo_find_predictor(                          
    opus_int32                  *ratio_Q14,                     
    const opus_int16            x[],                            
    const opus_int16            y[],                            
    opus_int32                  mid_res_amp_Q0[],               
    opus_int                    length,                         
    opus_int                    smooth_coef_Q16                 
)
{
    opus_int   scale, scale1, scale2;
    opus_int32 nrgx, nrgy, corr, pred_Q13, pred2_Q10;

    
    silk_sum_sqr_shift( &nrgx, &scale1, x, length );
    silk_sum_sqr_shift( &nrgy, &scale2, y, length );
    scale = silk_max_int( scale1, scale2 );
    scale = scale + ( scale & 1 );          
    nrgy = silk_RSHIFT32( nrgy, scale - scale2 );
    nrgx = silk_RSHIFT32( nrgx, scale - scale1 );
    nrgx = silk_max_int( nrgx, 1 );
    corr = silk_inner_prod_aligned_scale( x, y, scale, length );
    pred_Q13 = silk_DIV32_varQ( corr, nrgx, 13 );
    pred_Q13 = silk_LIMIT( pred_Q13, -(1 << 14), 1 << 14 );
    pred2_Q10 = silk_SMULWB( pred_Q13, pred_Q13 );

    
    smooth_coef_Q16 = (opus_int)silk_max_int( smooth_coef_Q16, silk_abs( pred2_Q10 ) );

    
    silk_assert( smooth_coef_Q16 < 32768 );
    scale = silk_RSHIFT( scale, 1 );
    mid_res_amp_Q0[ 0 ] = silk_SMLAWB( mid_res_amp_Q0[ 0 ], silk_LSHIFT( silk_SQRT_APPROX( nrgx ), scale ) - mid_res_amp_Q0[ 0 ],
        smooth_coef_Q16 );
    
    nrgy = silk_SUB_LSHIFT32( nrgy, silk_SMULWB( corr, pred_Q13 ), 3 + 1 );
    nrgy = silk_ADD_LSHIFT32( nrgy, silk_SMULWB( nrgx, pred2_Q10 ), 6 );
    mid_res_amp_Q0[ 1 ] = silk_SMLAWB( mid_res_amp_Q0[ 1 ], silk_LSHIFT( silk_SQRT_APPROX( nrgy ), scale ) - mid_res_amp_Q0[ 1 ],
        smooth_coef_Q16 );

    
    *ratio_Q14 = silk_DIV32_varQ( mid_res_amp_Q0[ 1 ], silk_max( mid_res_amp_Q0[ 0 ], 1 ), 14 );
    *ratio_Q14 = silk_LIMIT( *ratio_Q14, 0, 32767 );

    return pred_Q13;
}
