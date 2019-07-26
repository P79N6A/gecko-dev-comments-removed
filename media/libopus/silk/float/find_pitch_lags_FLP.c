






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include "main_FLP.h"
#include "tuning_parameters.h"

void silk_find_pitch_lags_FLP(
    silk_encoder_state_FLP          *psEnc,                             
    silk_encoder_control_FLP        *psEncCtrl,                         
    silk_float                      res[],                              
    const silk_float                x[]                                 
)
{
    opus_int   buf_len;
    silk_float thrhld, res_nrg;
    const silk_float *x_buf_ptr, *x_buf;
    silk_float auto_corr[ MAX_FIND_PITCH_LPC_ORDER + 1 ];
    silk_float A[         MAX_FIND_PITCH_LPC_ORDER ];
    silk_float refl_coef[ MAX_FIND_PITCH_LPC_ORDER ];
    silk_float Wsig[      FIND_PITCH_LPC_WIN_MAX ];
    silk_float *Wsig_ptr;

    
    
    
    buf_len = psEnc->sCmn.la_pitch + psEnc->sCmn.frame_length + psEnc->sCmn.ltp_mem_length;

    
    silk_assert( buf_len >= psEnc->sCmn.pitch_LPC_win_length );

    x_buf = x - psEnc->sCmn.ltp_mem_length;

    
    
    

    

    
    x_buf_ptr = x_buf + buf_len - psEnc->sCmn.pitch_LPC_win_length;
    Wsig_ptr  = Wsig;
    silk_apply_sine_window_FLP( Wsig_ptr, x_buf_ptr, 1, psEnc->sCmn.la_pitch );

    
    Wsig_ptr  += psEnc->sCmn.la_pitch;
    x_buf_ptr += psEnc->sCmn.la_pitch;
    silk_memcpy( Wsig_ptr, x_buf_ptr, ( psEnc->sCmn.pitch_LPC_win_length - ( psEnc->sCmn.la_pitch << 1 ) ) * sizeof( silk_float ) );

    
    Wsig_ptr  += psEnc->sCmn.pitch_LPC_win_length - ( psEnc->sCmn.la_pitch << 1 );
    x_buf_ptr += psEnc->sCmn.pitch_LPC_win_length - ( psEnc->sCmn.la_pitch << 1 );
    silk_apply_sine_window_FLP( Wsig_ptr, x_buf_ptr, 2, psEnc->sCmn.la_pitch );

    
    silk_autocorrelation_FLP( auto_corr, Wsig, psEnc->sCmn.pitch_LPC_win_length, psEnc->sCmn.pitchEstimationLPCOrder + 1 );

    
    auto_corr[ 0 ] += auto_corr[ 0 ] * FIND_PITCH_WHITE_NOISE_FRACTION + 1;

    
    res_nrg = silk_schur_FLP( refl_coef, auto_corr, psEnc->sCmn.pitchEstimationLPCOrder );

    
    psEncCtrl->predGain = auto_corr[ 0 ] / silk_max_float( res_nrg, 1.0f );

    
    silk_k2a_FLP( A, refl_coef, psEnc->sCmn.pitchEstimationLPCOrder );

    
    silk_bwexpander_FLP( A, psEnc->sCmn.pitchEstimationLPCOrder, FIND_PITCH_BANDWITH_EXPANSION );

    
    
    
    silk_LPC_analysis_filter_FLP( res, A, x_buf, buf_len, psEnc->sCmn.pitchEstimationLPCOrder );

    if( psEnc->sCmn.indices.signalType != TYPE_NO_VOICE_ACTIVITY && psEnc->sCmn.first_frame_after_reset == 0 ) {
        
        thrhld  = 0.6f;
        thrhld -= 0.004f * psEnc->sCmn.pitchEstimationLPCOrder;
        thrhld -= 0.1f   * psEnc->sCmn.speech_activity_Q8 * ( 1.0f /  256.0f );
        thrhld -= 0.15f  * (psEnc->sCmn.prevSignalType >> 1);
        thrhld -= 0.1f   * psEnc->sCmn.input_tilt_Q15 * ( 1.0f / 32768.0f );

        
        
        
        if( silk_pitch_analysis_core_FLP( res, psEncCtrl->pitchL, &psEnc->sCmn.indices.lagIndex,
            &psEnc->sCmn.indices.contourIndex, &psEnc->LTPCorr, psEnc->sCmn.prevLag, psEnc->sCmn.pitchEstimationThreshold_Q16 / 65536.0f,
            thrhld, psEnc->sCmn.fs_kHz, psEnc->sCmn.pitchEstimationComplexity, psEnc->sCmn.nb_subfr ) == 0 )
        {
            psEnc->sCmn.indices.signalType = TYPE_VOICED;
        } else {
            psEnc->sCmn.indices.signalType = TYPE_UNVOICED;
        }
    } else {
        silk_memset( psEncCtrl->pitchL, 0, sizeof( psEncCtrl->pitchL ) );
        psEnc->sCmn.indices.lagIndex = 0;
        psEnc->sCmn.indices.contourIndex = 0;
        psEnc->LTPCorr = 0;
    }
}
