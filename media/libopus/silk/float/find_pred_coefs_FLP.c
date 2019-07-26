






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main_FLP.h"


void silk_find_pred_coefs_FLP(
    silk_encoder_state_FLP          *psEnc,                             
    silk_encoder_control_FLP        *psEncCtrl,                         
    const silk_float                res_pitch[],                        
    const silk_float                x[],                                
    opus_int                        condCoding                          
)
{
    opus_int         i;
    silk_float       WLTP[ MAX_NB_SUBFR * LTP_ORDER * LTP_ORDER ];
    silk_float       invGains[ MAX_NB_SUBFR ], Wght[ MAX_NB_SUBFR ];
    opus_int16       NLSF_Q15[ MAX_LPC_ORDER ];
    const silk_float *x_ptr;
    silk_float       *x_pre_ptr, LPC_in_pre[ MAX_NB_SUBFR * MAX_LPC_ORDER + MAX_FRAME_LENGTH ];
    silk_float       minInvGain;

    
    for( i = 0; i < psEnc->sCmn.nb_subfr; i++ ) {
        silk_assert( psEncCtrl->Gains[ i ] > 0.0f );
        invGains[ i ] = 1.0f / psEncCtrl->Gains[ i ];
        Wght[ i ]     = invGains[ i ] * invGains[ i ];
    }

    if( psEnc->sCmn.indices.signalType == TYPE_VOICED ) {
        
        
        
        silk_assert( psEnc->sCmn.ltp_mem_length - psEnc->sCmn.predictLPCOrder >= psEncCtrl->pitchL[ 0 ] + LTP_ORDER / 2 );

        
        silk_find_LTP_FLP( psEncCtrl->LTPCoef, WLTP, &psEncCtrl->LTPredCodGain, res_pitch,
            psEncCtrl->pitchL, Wght, psEnc->sCmn.subfr_length, psEnc->sCmn.nb_subfr, psEnc->sCmn.ltp_mem_length );

        
        silk_quant_LTP_gains_FLP( psEncCtrl->LTPCoef, psEnc->sCmn.indices.LTPIndex, &psEnc->sCmn.indices.PERIndex,
            WLTP, psEnc->sCmn.mu_LTP_Q9, psEnc->sCmn.LTPQuantLowComplexity, psEnc->sCmn.nb_subfr );

        
        silk_LTP_scale_ctrl_FLP( psEnc, psEncCtrl, condCoding );

        
        silk_LTP_analysis_filter_FLP( LPC_in_pre, x - psEnc->sCmn.predictLPCOrder, psEncCtrl->LTPCoef,
            psEncCtrl->pitchL, invGains, psEnc->sCmn.subfr_length, psEnc->sCmn.nb_subfr, psEnc->sCmn.predictLPCOrder );
    } else {
        
        
        
        
        x_ptr     = x - psEnc->sCmn.predictLPCOrder;
        x_pre_ptr = LPC_in_pre;
        for( i = 0; i < psEnc->sCmn.nb_subfr; i++ ) {
            silk_scale_copy_vector_FLP( x_pre_ptr, x_ptr, invGains[ i ],
                psEnc->sCmn.subfr_length + psEnc->sCmn.predictLPCOrder );
            x_pre_ptr += psEnc->sCmn.subfr_length + psEnc->sCmn.predictLPCOrder;
            x_ptr     += psEnc->sCmn.subfr_length;
        }
        silk_memset( psEncCtrl->LTPCoef, 0, psEnc->sCmn.nb_subfr * LTP_ORDER * sizeof( silk_float ) );
        psEncCtrl->LTPredCodGain = 0.0f;
    }

    
    if( psEnc->sCmn.first_frame_after_reset ) {
        minInvGain = 1.0f / MAX_PREDICTION_POWER_GAIN_AFTER_RESET;
    } else {
        minInvGain = (silk_float)pow( 2, psEncCtrl->LTPredCodGain / 3 ) /  MAX_PREDICTION_POWER_GAIN;
        minInvGain /= 0.25f + 0.75f * psEncCtrl->coding_quality;
    }

    
    silk_find_LPC_FLP( &psEnc->sCmn, NLSF_Q15, LPC_in_pre, minInvGain );

    
    silk_process_NLSFs_FLP( &psEnc->sCmn, psEncCtrl->PredCoef, NLSF_Q15, psEnc->sCmn.prev_NLSFq_Q15 );

    
    silk_residual_energy_FLP( psEncCtrl->ResNrg, LPC_in_pre, psEncCtrl->PredCoef, psEncCtrl->Gains,
        psEnc->sCmn.subfr_length, psEnc->sCmn.nb_subfr, psEnc->sCmn.predictLPCOrder );

    
    silk_memcpy( psEnc->sCmn.prev_NLSFq_Q15, NLSF_Q15, sizeof( psEnc->sCmn.prev_NLSFq_Q15 ) );
}

