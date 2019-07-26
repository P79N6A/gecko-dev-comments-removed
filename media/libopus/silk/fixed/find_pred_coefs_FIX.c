






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main_FIX.h"

void silk_find_pred_coefs_FIX(
    silk_encoder_state_FIX          *psEnc,                                 
    silk_encoder_control_FIX        *psEncCtrl,                             
    const opus_int16                res_pitch[],                            
    const opus_int16                x[],                                    
    opus_int                        condCoding                              
)
{
    opus_int         i;
    opus_int32       WLTP[ MAX_NB_SUBFR * LTP_ORDER * LTP_ORDER ];
    opus_int32       invGains_Q16[ MAX_NB_SUBFR ], local_gains[ MAX_NB_SUBFR ], Wght_Q15[ MAX_NB_SUBFR ];
    opus_int16       NLSF_Q15[ MAX_LPC_ORDER ];
    const opus_int16 *x_ptr;
    opus_int16       *x_pre_ptr, LPC_in_pre[ MAX_NB_SUBFR * MAX_LPC_ORDER + MAX_FRAME_LENGTH ];
    opus_int32       tmp, min_gain_Q16, minInvGain_Q30;
    opus_int         LTP_corrs_rshift[ MAX_NB_SUBFR ];

    
    min_gain_Q16 = silk_int32_MAX >> 6;
    for( i = 0; i < psEnc->sCmn.nb_subfr; i++ ) {
        min_gain_Q16 = silk_min( min_gain_Q16, psEncCtrl->Gains_Q16[ i ] );
    }
    for( i = 0; i < psEnc->sCmn.nb_subfr; i++ ) {
        
        silk_assert( psEncCtrl->Gains_Q16[ i ] > 0 );
        
        invGains_Q16[ i ] = silk_DIV32_varQ( min_gain_Q16, psEncCtrl->Gains_Q16[ i ], 16 - 2 );

        
        invGains_Q16[ i ] = silk_max( invGains_Q16[ i ], 363 );

        
        silk_assert( invGains_Q16[ i ] == silk_SAT16( invGains_Q16[ i ] ) );
        tmp = silk_SMULWB( invGains_Q16[ i ], invGains_Q16[ i ] );
        Wght_Q15[ i ] = silk_RSHIFT( tmp, 1 );

        
        local_gains[ i ] = silk_DIV32( ( 1 << 16 ), invGains_Q16[ i ] );
    }

    if( psEnc->sCmn.indices.signalType == TYPE_VOICED ) {
        
        
        
        silk_assert( psEnc->sCmn.ltp_mem_length - psEnc->sCmn.predictLPCOrder >= psEncCtrl->pitchL[ 0 ] + LTP_ORDER / 2 );

        
        silk_find_LTP_FIX( psEncCtrl->LTPCoef_Q14, WLTP, &psEncCtrl->LTPredCodGain_Q7,
            res_pitch, psEncCtrl->pitchL, Wght_Q15, psEnc->sCmn.subfr_length,
            psEnc->sCmn.nb_subfr, psEnc->sCmn.ltp_mem_length, LTP_corrs_rshift );

        
        silk_quant_LTP_gains( psEncCtrl->LTPCoef_Q14, psEnc->sCmn.indices.LTPIndex, &psEnc->sCmn.indices.PERIndex,
            WLTP, psEnc->sCmn.mu_LTP_Q9, psEnc->sCmn.LTPQuantLowComplexity, psEnc->sCmn.nb_subfr);

        
        silk_LTP_scale_ctrl_FIX( psEnc, psEncCtrl, condCoding );

        
        silk_LTP_analysis_filter_FIX( LPC_in_pre, x - psEnc->sCmn.predictLPCOrder, psEncCtrl->LTPCoef_Q14,
            psEncCtrl->pitchL, invGains_Q16, psEnc->sCmn.subfr_length, psEnc->sCmn.nb_subfr, psEnc->sCmn.predictLPCOrder );

    } else {
        
        
        
        
        x_ptr     = x - psEnc->sCmn.predictLPCOrder;
        x_pre_ptr = LPC_in_pre;
        for( i = 0; i < psEnc->sCmn.nb_subfr; i++ ) {
            silk_scale_copy_vector16( x_pre_ptr, x_ptr, invGains_Q16[ i ],
                psEnc->sCmn.subfr_length + psEnc->sCmn.predictLPCOrder );
            x_pre_ptr += psEnc->sCmn.subfr_length + psEnc->sCmn.predictLPCOrder;
            x_ptr     += psEnc->sCmn.subfr_length;
        }

        silk_memset( psEncCtrl->LTPCoef_Q14, 0, psEnc->sCmn.nb_subfr * LTP_ORDER * sizeof( opus_int16 ) );
        psEncCtrl->LTPredCodGain_Q7 = 0;
    }

    
    if( psEnc->sCmn.first_frame_after_reset ) {
        minInvGain_Q30 = SILK_FIX_CONST( 1.0f / MAX_PREDICTION_POWER_GAIN_AFTER_RESET, 30 );
    } else {
        minInvGain_Q30 = silk_log2lin( silk_SMLAWB( 16 << 7, psEncCtrl->LTPredCodGain_Q7, SILK_FIX_CONST( 1.0 / 3, 16 ) ) );      
        minInvGain_Q30 = silk_DIV32_varQ( minInvGain_Q30,
            silk_SMULWW( SILK_FIX_CONST( MAX_PREDICTION_POWER_GAIN, 0 ),
                silk_SMLAWB( SILK_FIX_CONST( 0.25, 18 ), SILK_FIX_CONST( 0.75, 18 ), psEncCtrl->coding_quality_Q14 ) ), 14 );
    }

    
    silk_find_LPC_FIX( &psEnc->sCmn, NLSF_Q15, LPC_in_pre, minInvGain_Q30 );

    
    silk_process_NLSFs( &psEnc->sCmn, psEncCtrl->PredCoef_Q12, NLSF_Q15, psEnc->sCmn.prev_NLSFq_Q15 );

    
    silk_residual_energy_FIX( psEncCtrl->ResNrg, psEncCtrl->ResNrgQ, LPC_in_pre, psEncCtrl->PredCoef_Q12, local_gains,
        psEnc->sCmn.subfr_length, psEnc->sCmn.nb_subfr, psEnc->sCmn.predictLPCOrder );

    
    silk_memcpy( psEnc->sCmn.prev_NLSFq_Q15, NLSF_Q15, sizeof( psEnc->sCmn.prev_NLSFq_Q15 ) );
}
