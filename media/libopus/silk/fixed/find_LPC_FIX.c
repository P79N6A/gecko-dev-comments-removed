






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main_FIX.h"
#include "tuning_parameters.h"


void silk_find_LPC_FIX(
    silk_encoder_state              *psEncC,                                
    opus_int16                      NLSF_Q15[],                             
    const opus_int16                x[],                                    
    const opus_int32                minInvGain_Q30                          
)
{
    opus_int     k, subfr_length;
    opus_int32   a_Q16[ MAX_LPC_ORDER ];
    opus_int     isInterpLower, shift;
    opus_int32   res_nrg0, res_nrg1;
    opus_int     rshift0, rshift1;

    
    opus_int32   a_tmp_Q16[ MAX_LPC_ORDER ], res_nrg_interp, res_nrg, res_tmp_nrg;
    opus_int     res_nrg_interp_Q, res_nrg_Q, res_tmp_nrg_Q;
    opus_int16   a_tmp_Q12[ MAX_LPC_ORDER ];
    opus_int16   NLSF0_Q15[ MAX_LPC_ORDER ];
    opus_int16   LPC_res[ MAX_FRAME_LENGTH + MAX_NB_SUBFR * MAX_LPC_ORDER ];

    subfr_length = psEncC->subfr_length + psEncC->predictLPCOrder;

    
    psEncC->indices.NLSFInterpCoef_Q2 = 4;

    
    silk_burg_modified( &res_nrg, &res_nrg_Q, a_Q16, x, minInvGain_Q30, subfr_length, psEncC->nb_subfr, psEncC->predictLPCOrder );

    if( psEncC->useInterpolatedNLSFs && !psEncC->first_frame_after_reset && psEncC->nb_subfr == MAX_NB_SUBFR ) {
        
        silk_burg_modified( &res_tmp_nrg, &res_tmp_nrg_Q, a_tmp_Q16, x + 2 * subfr_length, minInvGain_Q30, subfr_length, 2, psEncC->predictLPCOrder );

        
        
        shift = res_tmp_nrg_Q - res_nrg_Q;
        if( shift >= 0 ) {
            if( shift < 32 ) {
                res_nrg = res_nrg - silk_RSHIFT( res_tmp_nrg, shift );
            }
        } else {
            silk_assert( shift > -32 );
            res_nrg   = silk_RSHIFT( res_nrg, -shift ) - res_tmp_nrg;
            res_nrg_Q = res_tmp_nrg_Q;
        }

        
        silk_A2NLSF( NLSF_Q15, a_tmp_Q16, psEncC->predictLPCOrder );

        
        for( k = 3; k >= 0; k-- ) {
            
            silk_interpolate( NLSF0_Q15, psEncC->prev_NLSFq_Q15, NLSF_Q15, k, psEncC->predictLPCOrder );

            
            silk_NLSF2A( a_tmp_Q12, NLSF0_Q15, psEncC->predictLPCOrder );

            
            silk_LPC_analysis_filter( LPC_res, x, a_tmp_Q12, 2 * subfr_length, psEncC->predictLPCOrder );

            silk_sum_sqr_shift( &res_nrg0, &rshift0, LPC_res + psEncC->predictLPCOrder,                subfr_length - psEncC->predictLPCOrder );
            silk_sum_sqr_shift( &res_nrg1, &rshift1, LPC_res + psEncC->predictLPCOrder + subfr_length, subfr_length - psEncC->predictLPCOrder );

            
            shift = rshift0 - rshift1;
            if( shift >= 0 ) {
                res_nrg1         = silk_RSHIFT( res_nrg1, shift );
                res_nrg_interp_Q = -rshift0;
            } else {
                res_nrg0         = silk_RSHIFT( res_nrg0, -shift );
                res_nrg_interp_Q = -rshift1;
            }
            res_nrg_interp = silk_ADD32( res_nrg0, res_nrg1 );

            
            shift = res_nrg_interp_Q - res_nrg_Q;
            if( shift >= 0 ) {
                if( silk_RSHIFT( res_nrg_interp, shift ) < res_nrg ) {
                    isInterpLower = silk_TRUE;
                } else {
                    isInterpLower = silk_FALSE;
                }
            } else {
                if( -shift < 32 ) {
                    if( res_nrg_interp < silk_RSHIFT( res_nrg, -shift ) ) {
                        isInterpLower = silk_TRUE;
                    } else {
                        isInterpLower = silk_FALSE;
                    }
                } else {
                    isInterpLower = silk_FALSE;
                }
            }

            
            if( isInterpLower == silk_TRUE ) {
                
                res_nrg   = res_nrg_interp;
                res_nrg_Q = res_nrg_interp_Q;
                psEncC->indices.NLSFInterpCoef_Q2 = (opus_int8)k;
            }
        }
    }

    if( psEncC->indices.NLSFInterpCoef_Q2 == 4 ) {
        
        silk_A2NLSF( NLSF_Q15, a_Q16, psEncC->predictLPCOrder );
    }

    silk_assert( psEncC->indices.NLSFInterpCoef_Q2 == 4 || ( psEncC->useInterpolatedNLSFs && !psEncC->first_frame_after_reset && psEncC->nb_subfr == MAX_NB_SUBFR ) );
}
