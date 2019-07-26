






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main_FIX.h"
#include "tuning_parameters.h"


void silk_find_pitch_lags_FIX(
    silk_encoder_state_FIX          *psEnc,                                 
    silk_encoder_control_FIX        *psEncCtrl,                             
    opus_int16                      res[],                                  
    const opus_int16                x[]                                     
)
{
    opus_int   buf_len, i, scale;
    opus_int32 thrhld_Q15, res_nrg;
    const opus_int16 *x_buf, *x_buf_ptr;
    opus_int16 Wsig[      FIND_PITCH_LPC_WIN_MAX ], *Wsig_ptr;
    opus_int32 auto_corr[ MAX_FIND_PITCH_LPC_ORDER + 1 ];
    opus_int16 rc_Q15[    MAX_FIND_PITCH_LPC_ORDER ];
    opus_int32 A_Q24[     MAX_FIND_PITCH_LPC_ORDER ];
    opus_int16 A_Q12[     MAX_FIND_PITCH_LPC_ORDER ];

    
    
    
    buf_len = psEnc->sCmn.la_pitch + psEnc->sCmn.frame_length + psEnc->sCmn.ltp_mem_length;

    
    silk_assert( buf_len >= psEnc->sCmn.pitch_LPC_win_length );

    x_buf = x - psEnc->sCmn.ltp_mem_length;

    
    
    

    

    
    x_buf_ptr = x_buf + buf_len - psEnc->sCmn.pitch_LPC_win_length;
    Wsig_ptr  = Wsig;
    silk_apply_sine_window( Wsig_ptr, x_buf_ptr, 1, psEnc->sCmn.la_pitch );

    
    Wsig_ptr  += psEnc->sCmn.la_pitch;
    x_buf_ptr += psEnc->sCmn.la_pitch;
    silk_memcpy( Wsig_ptr, x_buf_ptr, ( psEnc->sCmn.pitch_LPC_win_length - silk_LSHIFT( psEnc->sCmn.la_pitch, 1 ) ) * sizeof( opus_int16 ) );

    
    Wsig_ptr  += psEnc->sCmn.pitch_LPC_win_length - silk_LSHIFT( psEnc->sCmn.la_pitch, 1 );
    x_buf_ptr += psEnc->sCmn.pitch_LPC_win_length - silk_LSHIFT( psEnc->sCmn.la_pitch, 1 );
    silk_apply_sine_window( Wsig_ptr, x_buf_ptr, 2, psEnc->sCmn.la_pitch );

    
    silk_autocorr( auto_corr, &scale, Wsig, psEnc->sCmn.pitch_LPC_win_length, psEnc->sCmn.pitchEstimationLPCOrder + 1 );

    
    auto_corr[ 0 ] = silk_SMLAWB( auto_corr[ 0 ], auto_corr[ 0 ], SILK_FIX_CONST( FIND_PITCH_WHITE_NOISE_FRACTION, 16 ) ) + 1;

    
    res_nrg = silk_schur( rc_Q15, auto_corr, psEnc->sCmn.pitchEstimationLPCOrder );

    
    psEncCtrl->predGain_Q16 = silk_DIV32_varQ( auto_corr[ 0 ], silk_max_int( res_nrg, 1 ), 16 );

    
    silk_k2a( A_Q24, rc_Q15, psEnc->sCmn.pitchEstimationLPCOrder );

    
    for( i = 0; i < psEnc->sCmn.pitchEstimationLPCOrder; i++ ) {
        A_Q12[ i ] = (opus_int16)silk_SAT16( silk_RSHIFT( A_Q24[ i ], 12 ) );
    }

    
    silk_bwexpander( A_Q12, psEnc->sCmn.pitchEstimationLPCOrder, SILK_FIX_CONST( FIND_PITCH_BANDWITH_EXPANSION, 16 ) );

    
    
    
    silk_LPC_analysis_filter( res, x_buf, A_Q12, buf_len, psEnc->sCmn.pitchEstimationLPCOrder );

    if( psEnc->sCmn.indices.signalType != TYPE_NO_VOICE_ACTIVITY && psEnc->sCmn.first_frame_after_reset == 0 ) {
        
        thrhld_Q15 = SILK_FIX_CONST( 0.6, 15 );
        thrhld_Q15 = silk_SMLABB( thrhld_Q15, SILK_FIX_CONST( -0.004, 15 ), psEnc->sCmn.pitchEstimationLPCOrder );
        thrhld_Q15 = silk_SMLABB( thrhld_Q15, SILK_FIX_CONST( -0.1,   7  ), psEnc->sCmn.speech_activity_Q8 );
        thrhld_Q15 = silk_SMLABB( thrhld_Q15, SILK_FIX_CONST( -0.15,  15 ), silk_RSHIFT( psEnc->sCmn.prevSignalType, 1 ) );
        thrhld_Q15 = silk_SMLAWB( thrhld_Q15, SILK_FIX_CONST( -0.1,   16 ), psEnc->sCmn.input_tilt_Q15 );
        thrhld_Q15 = silk_SAT16(  thrhld_Q15 );

        
        
        
        if( silk_pitch_analysis_core( res, psEncCtrl->pitchL, &psEnc->sCmn.indices.lagIndex, &psEnc->sCmn.indices.contourIndex,
                &psEnc->LTPCorr_Q15, psEnc->sCmn.prevLag, psEnc->sCmn.pitchEstimationThreshold_Q16,
                (opus_int16)thrhld_Q15, psEnc->sCmn.fs_kHz, psEnc->sCmn.pitchEstimationComplexity, psEnc->sCmn.nb_subfr ) == 0 )
        {
            psEnc->sCmn.indices.signalType = TYPE_VOICED;
        } else {
            psEnc->sCmn.indices.signalType = TYPE_UNVOICED;
        }
    } else {
        silk_memset( psEncCtrl->pitchL, 0, sizeof( psEncCtrl->pitchL ) );
        psEnc->sCmn.indices.lagIndex = 0;
        psEnc->sCmn.indices.contourIndex = 0;
        psEnc->LTPCorr_Q15 = 0;
    }
}
