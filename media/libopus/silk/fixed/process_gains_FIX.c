






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main_FIX.h"
#include "tuning_parameters.h"


void silk_process_gains_FIX(
    silk_encoder_state_FIX          *psEnc,                                 
    silk_encoder_control_FIX        *psEncCtrl,                             
    opus_int                        condCoding                              
)
{
    silk_shape_state_FIX *psShapeSt = &psEnc->sShape;
    opus_int     k;
    opus_int32   s_Q16, InvMaxSqrVal_Q16, gain, gain_squared, ResNrg, ResNrgPart, quant_offset_Q10;

    
    if( psEnc->sCmn.indices.signalType == TYPE_VOICED ) {
        
        s_Q16 = -silk_sigm_Q15( silk_RSHIFT_ROUND( psEncCtrl->LTPredCodGain_Q7 - SILK_FIX_CONST( 12.0, 7 ), 4 ) );
        for( k = 0; k < psEnc->sCmn.nb_subfr; k++ ) {
            psEncCtrl->Gains_Q16[ k ] = silk_SMLAWB( psEncCtrl->Gains_Q16[ k ], psEncCtrl->Gains_Q16[ k ], s_Q16 );
        }
    }

    
    
    InvMaxSqrVal_Q16 = silk_DIV32_16( silk_log2lin(
        silk_SMULWB( SILK_FIX_CONST( 21 + 16 / 0.33, 7 ) - psEnc->sCmn.SNR_dB_Q7, SILK_FIX_CONST( 0.33, 16 ) ) ), psEnc->sCmn.subfr_length );

    for( k = 0; k < psEnc->sCmn.nb_subfr; k++ ) {
        
        ResNrg     = psEncCtrl->ResNrg[ k ];
        ResNrgPart = silk_SMULWW( ResNrg, InvMaxSqrVal_Q16 );
        if( psEncCtrl->ResNrgQ[ k ] > 0 ) {
            ResNrgPart = silk_RSHIFT_ROUND( ResNrgPart, psEncCtrl->ResNrgQ[ k ] );
        } else {
            if( ResNrgPart >= silk_RSHIFT( silk_int32_MAX, -psEncCtrl->ResNrgQ[ k ] ) ) {
                ResNrgPart = silk_int32_MAX;
            } else {
                ResNrgPart = silk_LSHIFT( ResNrgPart, -psEncCtrl->ResNrgQ[ k ] );
            }
        }
        gain = psEncCtrl->Gains_Q16[ k ];
        gain_squared = silk_ADD_SAT32( ResNrgPart, silk_SMMUL( gain, gain ) );
        if( gain_squared < silk_int16_MAX ) {
            
            gain_squared = silk_SMLAWW( silk_LSHIFT( ResNrgPart, 16 ), gain, gain );
            silk_assert( gain_squared > 0 );
            gain = silk_SQRT_APPROX( gain_squared );                    
            gain = silk_min( gain, silk_int32_MAX >> 8 );
            psEncCtrl->Gains_Q16[ k ] = silk_LSHIFT_SAT32( gain, 8 );   
        } else {
            gain = silk_SQRT_APPROX( gain_squared );                    
            gain = silk_min( gain, silk_int32_MAX >> 16 );
            psEncCtrl->Gains_Q16[ k ] = silk_LSHIFT_SAT32( gain, 16 );  
        }
    }

    
    silk_memcpy( psEncCtrl->GainsUnq_Q16, psEncCtrl->Gains_Q16, psEnc->sCmn.nb_subfr * sizeof( opus_int32 ) );
    psEncCtrl->lastGainIndexPrev = psShapeSt->LastGainIndex;

    
    silk_gains_quant( psEnc->sCmn.indices.GainsIndices, psEncCtrl->Gains_Q16,
        &psShapeSt->LastGainIndex, condCoding == CODE_CONDITIONALLY, psEnc->sCmn.nb_subfr );

    
    if( psEnc->sCmn.indices.signalType == TYPE_VOICED ) {
        if( psEncCtrl->LTPredCodGain_Q7 + silk_RSHIFT( psEnc->sCmn.input_tilt_Q15, 8 ) > SILK_FIX_CONST( 1.0, 7 ) ) {
            psEnc->sCmn.indices.quantOffsetType = 0;
        } else {
            psEnc->sCmn.indices.quantOffsetType = 1;
        }
    }

    
    quant_offset_Q10 = silk_Quantization_Offsets_Q10[ psEnc->sCmn.indices.signalType >> 1 ][ psEnc->sCmn.indices.quantOffsetType ];
    psEncCtrl->Lambda_Q10 = SILK_FIX_CONST( LAMBDA_OFFSET, 10 )
                          + silk_SMULBB( SILK_FIX_CONST( LAMBDA_DELAYED_DECISIONS, 10 ), psEnc->sCmn.nStatesDelayedDecision )
                          + silk_SMULWB( SILK_FIX_CONST( LAMBDA_SPEECH_ACT,        18 ), psEnc->sCmn.speech_activity_Q8     )
                          + silk_SMULWB( SILK_FIX_CONST( LAMBDA_INPUT_QUALITY,     12 ), psEncCtrl->input_quality_Q14       )
                          + silk_SMULWB( SILK_FIX_CONST( LAMBDA_CODING_QUALITY,    12 ), psEncCtrl->coding_quality_Q14      )
                          + silk_SMULWB( SILK_FIX_CONST( LAMBDA_QUANT_OFFSET,      16 ), quant_offset_Q10                   );

    silk_assert( psEncCtrl->Lambda_Q10 > 0 );
    silk_assert( psEncCtrl->Lambda_Q10 < SILK_FIX_CONST( 2, 10 ) );
}
