






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "define.h"
#include "main_FLP.h"
#include "tuning_parameters.h"


void silk_find_LPC_FLP(
    silk_encoder_state              *psEncC,                            
    opus_int16                      NLSF_Q15[],                         
    const silk_float                x[],                                
    const silk_float                minInvGain                          
)
{
    opus_int    k, subfr_length;
    silk_float  a[ MAX_LPC_ORDER ];

    
    silk_float  res_nrg, res_nrg_2nd, res_nrg_interp;
    opus_int16  NLSF0_Q15[ MAX_LPC_ORDER ];
    silk_float  a_tmp[ MAX_LPC_ORDER ];
    silk_float  LPC_res[ MAX_FRAME_LENGTH + MAX_NB_SUBFR * MAX_LPC_ORDER ];

    subfr_length = psEncC->subfr_length + psEncC->predictLPCOrder;

    
    psEncC->indices.NLSFInterpCoef_Q2 = 4;

    
    res_nrg = silk_burg_modified_FLP( a, x, minInvGain, subfr_length, psEncC->nb_subfr, psEncC->predictLPCOrder );

    if( psEncC->useInterpolatedNLSFs && !psEncC->first_frame_after_reset && psEncC->nb_subfr == MAX_NB_SUBFR ) {
        
        
        res_nrg -= silk_burg_modified_FLP( a_tmp, x + ( MAX_NB_SUBFR / 2 ) * subfr_length, minInvGain, subfr_length, MAX_NB_SUBFR / 2, psEncC->predictLPCOrder );

        
        silk_A2NLSF_FLP( NLSF_Q15, a_tmp, psEncC->predictLPCOrder );

        
        res_nrg_2nd = silk_float_MAX;
        for( k = 3; k >= 0; k-- ) {
            
            silk_interpolate( NLSF0_Q15, psEncC->prev_NLSFq_Q15, NLSF_Q15, k, psEncC->predictLPCOrder );

            
            silk_NLSF2A_FLP( a_tmp, NLSF0_Q15, psEncC->predictLPCOrder );

            
            silk_LPC_analysis_filter_FLP( LPC_res, a_tmp, x, 2 * subfr_length, psEncC->predictLPCOrder );
            res_nrg_interp = (silk_float)(
                silk_energy_FLP( LPC_res + psEncC->predictLPCOrder,                subfr_length - psEncC->predictLPCOrder ) +
                silk_energy_FLP( LPC_res + psEncC->predictLPCOrder + subfr_length, subfr_length - psEncC->predictLPCOrder ) );

            
            if( res_nrg_interp < res_nrg ) {
                
                res_nrg = res_nrg_interp;
                psEncC->indices.NLSFInterpCoef_Q2 = (opus_int8)k;
            } else if( res_nrg_interp > res_nrg_2nd ) {
                
                break;
            }
            res_nrg_2nd = res_nrg_interp;
        }
    }

    if( psEncC->indices.NLSFInterpCoef_Q2 == 4 ) {
        
        silk_A2NLSF_FLP( NLSF_Q15, a, psEncC->predictLPCOrder );
    }

    silk_assert( psEncC->indices.NLSFInterpCoef_Q2 == 4 ||
        ( psEncC->useInterpolatedNLSFs && !psEncC->first_frame_after_reset && psEncC->nb_subfr == MAX_NB_SUBFR ) );
}
