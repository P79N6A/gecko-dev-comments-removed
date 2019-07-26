






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main_FLP.h"
#include "tuning_parameters.h"





static inline silk_float warped_gain(
    const silk_float     *coefs,
    silk_float           lambda,
    opus_int             order
) {
    opus_int   i;
    silk_float gain;

    lambda = -lambda;
    gain = coefs[ order - 1 ];
    for( i = order - 2; i >= 0; i-- ) {
        gain = lambda * gain + coefs[ i ];
    }
    return (silk_float)( 1.0f / ( 1.0f - lambda * gain ) );
}



static inline void warped_true2monic_coefs(
    silk_float           *coefs_syn,
    silk_float           *coefs_ana,
    silk_float           lambda,
    silk_float           limit,
    opus_int             order
) {
    opus_int   i, iter, ind = 0;
    silk_float tmp, maxabs, chirp, gain_syn, gain_ana;

    
    for( i = order - 1; i > 0; i-- ) {
        coefs_syn[ i - 1 ] -= lambda * coefs_syn[ i ];
        coefs_ana[ i - 1 ] -= lambda * coefs_ana[ i ];
    }
    gain_syn = ( 1.0f - lambda * lambda ) / ( 1.0f + lambda * coefs_syn[ 0 ] );
    gain_ana = ( 1.0f - lambda * lambda ) / ( 1.0f + lambda * coefs_ana[ 0 ] );
    for( i = 0; i < order; i++ ) {
        coefs_syn[ i ] *= gain_syn;
        coefs_ana[ i ] *= gain_ana;
    }

    
    for( iter = 0; iter < 10; iter++ ) {
        
        maxabs = -1.0f;
        for( i = 0; i < order; i++ ) {
            tmp = silk_max( silk_abs_float( coefs_syn[ i ] ), silk_abs_float( coefs_ana[ i ] ) );
            if( tmp > maxabs ) {
                maxabs = tmp;
                ind = i;
            }
        }
        if( maxabs <= limit ) {
            
            return;
        }

        
        for( i = 1; i < order; i++ ) {
            coefs_syn[ i - 1 ] += lambda * coefs_syn[ i ];
            coefs_ana[ i - 1 ] += lambda * coefs_ana[ i ];
        }
        gain_syn = 1.0f / gain_syn;
        gain_ana = 1.0f / gain_ana;
        for( i = 0; i < order; i++ ) {
            coefs_syn[ i ] *= gain_syn;
            coefs_ana[ i ] *= gain_ana;
        }

        
        chirp = 0.99f - ( 0.8f + 0.1f * iter ) * ( maxabs - limit ) / ( maxabs * ( ind + 1 ) );
        silk_bwexpander_FLP( coefs_syn, order, chirp );
        silk_bwexpander_FLP( coefs_ana, order, chirp );

        
        for( i = order - 1; i > 0; i-- ) {
            coefs_syn[ i - 1 ] -= lambda * coefs_syn[ i ];
            coefs_ana[ i - 1 ] -= lambda * coefs_ana[ i ];
        }
        gain_syn = ( 1.0f - lambda * lambda ) / ( 1.0f + lambda * coefs_syn[ 0 ] );
        gain_ana = ( 1.0f - lambda * lambda ) / ( 1.0f + lambda * coefs_ana[ 0 ] );
        for( i = 0; i < order; i++ ) {
            coefs_syn[ i ] *= gain_syn;
            coefs_ana[ i ] *= gain_ana;
        }
    }
    silk_assert( 0 );
}


void silk_noise_shape_analysis_FLP(
    silk_encoder_state_FLP          *psEnc,                             
    silk_encoder_control_FLP        *psEncCtrl,                         
    const silk_float                *pitch_res,                         
    const silk_float                *x                                  
)
{
    silk_shape_state_FLP *psShapeSt = &psEnc->sShape;
    opus_int     k, nSamples;
    silk_float   SNR_adj_dB, HarmBoost, HarmShapeGain, Tilt;
    silk_float   nrg, pre_nrg, log_energy, log_energy_prev, energy_variation;
    silk_float   delta, BWExp1, BWExp2, gain_mult, gain_add, strength, b, warping;
    silk_float   x_windowed[ SHAPE_LPC_WIN_MAX ];
    silk_float   auto_corr[ MAX_SHAPE_LPC_ORDER + 1 ];
    const silk_float *x_ptr, *pitch_res_ptr;

    
    x_ptr = x - psEnc->sCmn.la_shape;

    
    
    
    SNR_adj_dB = psEnc->sCmn.SNR_dB_Q7 * ( 1 / 128.0f );

    
    psEncCtrl->input_quality = 0.5f * ( psEnc->sCmn.input_quality_bands_Q15[ 0 ] + psEnc->sCmn.input_quality_bands_Q15[ 1 ] ) * ( 1.0f / 32768.0f );

    
    psEncCtrl->coding_quality = silk_sigmoid( 0.25f * ( SNR_adj_dB - 20.0f ) );

    if( psEnc->sCmn.useCBR == 0 ) {
        
        b = 1.0f - psEnc->sCmn.speech_activity_Q8 * ( 1.0f /  256.0f );
        SNR_adj_dB -= BG_SNR_DECR_dB * psEncCtrl->coding_quality * ( 0.5f + 0.5f * psEncCtrl->input_quality ) * b * b;
    }

    if( psEnc->sCmn.indices.signalType == TYPE_VOICED ) {
        
        SNR_adj_dB += HARM_SNR_INCR_dB * psEnc->LTPCorr;
    } else {
        
        SNR_adj_dB += ( -0.4f * psEnc->sCmn.SNR_dB_Q7 * ( 1 / 128.0f ) + 6.0f ) * ( 1.0f - psEncCtrl->input_quality );
    }

    
    
    
    
    if( psEnc->sCmn.indices.signalType == TYPE_VOICED ) {
        
        psEnc->sCmn.indices.quantOffsetType = 0;
        psEncCtrl->sparseness = 0.0f;
    } else {
        
        nSamples = 2 * psEnc->sCmn.fs_kHz;
        energy_variation = 0.0f;
        log_energy_prev  = 0.0f;
        pitch_res_ptr = pitch_res;
        for( k = 0; k < silk_SMULBB( SUB_FRAME_LENGTH_MS, psEnc->sCmn.nb_subfr ) / 2; k++ ) {
            nrg = ( silk_float )nSamples + ( silk_float )silk_energy_FLP( pitch_res_ptr, nSamples );
            log_energy = silk_log2( nrg );
            if( k > 0 ) {
                energy_variation += silk_abs_float( log_energy - log_energy_prev );
            }
            log_energy_prev = log_energy;
            pitch_res_ptr += nSamples;
        }
        psEncCtrl->sparseness = silk_sigmoid( 0.4f * ( energy_variation - 5.0f ) );

        
        if( psEncCtrl->sparseness > SPARSENESS_THRESHOLD_QNT_OFFSET ) {
            psEnc->sCmn.indices.quantOffsetType = 0;
        } else {
            psEnc->sCmn.indices.quantOffsetType = 1;
        }

        
        SNR_adj_dB += SPARSE_SNR_INCR_dB * ( psEncCtrl->sparseness - 0.5f );
    }

    
    
    
    
    strength = FIND_PITCH_WHITE_NOISE_FRACTION * psEncCtrl->predGain;           
    BWExp1 = BWExp2 = BANDWIDTH_EXPANSION / ( 1.0f + strength * strength );
    delta  = LOW_RATE_BANDWIDTH_EXPANSION_DELTA * ( 1.0f - 0.75f * psEncCtrl->coding_quality );
    BWExp1 -= delta;
    BWExp2 += delta;
    
    BWExp1 /= BWExp2;

    if( psEnc->sCmn.warping_Q16 > 0 ) {
        
        warping = (silk_float)psEnc->sCmn.warping_Q16 / 65536.0f + 0.01f * psEncCtrl->coding_quality;
    } else {
        warping = 0.0f;
    }

    
    
    
    for( k = 0; k < psEnc->sCmn.nb_subfr; k++ ) {
        
        opus_int shift, slope_part, flat_part;
        flat_part = psEnc->sCmn.fs_kHz * 3;
        slope_part = ( psEnc->sCmn.shapeWinLength - flat_part ) / 2;

        silk_apply_sine_window_FLP( x_windowed, x_ptr, 1, slope_part );
        shift = slope_part;
        silk_memcpy( x_windowed + shift, x_ptr + shift, flat_part * sizeof(silk_float) );
        shift += flat_part;
        silk_apply_sine_window_FLP( x_windowed + shift, x_ptr + shift, 2, slope_part );

        
        x_ptr += psEnc->sCmn.subfr_length;

        if( psEnc->sCmn.warping_Q16 > 0 ) {
            
            silk_warped_autocorrelation_FLP( auto_corr, x_windowed, warping,
                psEnc->sCmn.shapeWinLength, psEnc->sCmn.shapingLPCOrder );
        } else {
            
            silk_autocorrelation_FLP( auto_corr, x_windowed, psEnc->sCmn.shapeWinLength, psEnc->sCmn.shapingLPCOrder + 1 );
        }

        
        auto_corr[ 0 ] += auto_corr[ 0 ] * SHAPE_WHITE_NOISE_FRACTION;

        
        nrg = silk_levinsondurbin_FLP( &psEncCtrl->AR2[ k * MAX_SHAPE_LPC_ORDER ], auto_corr, psEnc->sCmn.shapingLPCOrder );
        psEncCtrl->Gains[ k ] = ( silk_float )sqrt( nrg );

        if( psEnc->sCmn.warping_Q16 > 0 ) {
            
            psEncCtrl->Gains[ k ] *= warped_gain( &psEncCtrl->AR2[ k * MAX_SHAPE_LPC_ORDER ], warping, psEnc->sCmn.shapingLPCOrder );
        }

        
        silk_bwexpander_FLP( &psEncCtrl->AR2[ k * MAX_SHAPE_LPC_ORDER ], psEnc->sCmn.shapingLPCOrder, BWExp2 );

        
        silk_memcpy(
            &psEncCtrl->AR1[ k * MAX_SHAPE_LPC_ORDER ],
            &psEncCtrl->AR2[ k * MAX_SHAPE_LPC_ORDER ],
            psEnc->sCmn.shapingLPCOrder * sizeof( silk_float ) );

        
        silk_bwexpander_FLP( &psEncCtrl->AR1[ k * MAX_SHAPE_LPC_ORDER ], psEnc->sCmn.shapingLPCOrder, BWExp1 );

        
        pre_nrg = silk_LPC_inverse_pred_gain_FLP( &psEncCtrl->AR2[ k * MAX_SHAPE_LPC_ORDER ], psEnc->sCmn.shapingLPCOrder );
        nrg     = silk_LPC_inverse_pred_gain_FLP( &psEncCtrl->AR1[ k * MAX_SHAPE_LPC_ORDER ], psEnc->sCmn.shapingLPCOrder );
        psEncCtrl->GainsPre[ k ] = 1.0f - 0.7f * ( 1.0f - pre_nrg / nrg );

        
        warped_true2monic_coefs( &psEncCtrl->AR2[ k * MAX_SHAPE_LPC_ORDER ], &psEncCtrl->AR1[ k * MAX_SHAPE_LPC_ORDER ],
            warping, 3.999f, psEnc->sCmn.shapingLPCOrder );
    }

    
    
    
    
    gain_mult = (silk_float)pow( 2.0f, -0.16f * SNR_adj_dB );
    gain_add  = (silk_float)pow( 2.0f,  0.16f * MIN_QGAIN_DB );
    for( k = 0; k < psEnc->sCmn.nb_subfr; k++ ) {
        psEncCtrl->Gains[ k ] *= gain_mult;
        psEncCtrl->Gains[ k ] += gain_add;
    }

    gain_mult = 1.0f + INPUT_TILT + psEncCtrl->coding_quality * HIGH_RATE_INPUT_TILT;
    for( k = 0; k < psEnc->sCmn.nb_subfr; k++ ) {
        psEncCtrl->GainsPre[ k ] *= gain_mult;
    }

    
    
    
    
    strength = LOW_FREQ_SHAPING * ( 1.0f + LOW_QUALITY_LOW_FREQ_SHAPING_DECR * ( psEnc->sCmn.input_quality_bands_Q15[ 0 ] * ( 1.0f / 32768.0f ) - 1.0f ) );
    strength *= psEnc->sCmn.speech_activity_Q8 * ( 1.0f /  256.0f );
    if( psEnc->sCmn.indices.signalType == TYPE_VOICED ) {
        
        
        for( k = 0; k < psEnc->sCmn.nb_subfr; k++ ) {
            b = 0.2f / psEnc->sCmn.fs_kHz + 3.0f / psEncCtrl->pitchL[ k ];
            psEncCtrl->LF_MA_shp[ k ] = -1.0f + b;
            psEncCtrl->LF_AR_shp[ k ] =  1.0f - b - b * strength;
        }
        Tilt = - HP_NOISE_COEF -
            (1 - HP_NOISE_COEF) * HARM_HP_NOISE_COEF * psEnc->sCmn.speech_activity_Q8 * ( 1.0f /  256.0f );
    } else {
        b = 1.3f / psEnc->sCmn.fs_kHz;
        psEncCtrl->LF_MA_shp[ 0 ] = -1.0f + b;
        psEncCtrl->LF_AR_shp[ 0 ] =  1.0f - b - b * strength * 0.6f;
        for( k = 1; k < psEnc->sCmn.nb_subfr; k++ ) {
            psEncCtrl->LF_MA_shp[ k ] = psEncCtrl->LF_MA_shp[ 0 ];
            psEncCtrl->LF_AR_shp[ k ] = psEncCtrl->LF_AR_shp[ 0 ];
        }
        Tilt = -HP_NOISE_COEF;
    }

    
    
    
    
    HarmBoost = LOW_RATE_HARMONIC_BOOST * ( 1.0f - psEncCtrl->coding_quality ) * psEnc->LTPCorr;

    
    HarmBoost += LOW_INPUT_QUALITY_HARMONIC_BOOST * ( 1.0f - psEncCtrl->input_quality );

    if( USE_HARM_SHAPING && psEnc->sCmn.indices.signalType == TYPE_VOICED ) {
        
        HarmShapeGain = HARMONIC_SHAPING;

        
        HarmShapeGain += HIGH_RATE_OR_LOW_QUALITY_HARMONIC_SHAPING *
            ( 1.0f - ( 1.0f - psEncCtrl->coding_quality ) * psEncCtrl->input_quality );

        
        HarmShapeGain *= ( silk_float )sqrt( psEnc->LTPCorr );
    } else {
        HarmShapeGain = 0.0f;
    }

    
    
    
    for( k = 0; k < psEnc->sCmn.nb_subfr; k++ ) {
        psShapeSt->HarmBoost_smth     += SUBFR_SMTH_COEF * ( HarmBoost - psShapeSt->HarmBoost_smth );
        psEncCtrl->HarmBoost[ k ]      = psShapeSt->HarmBoost_smth;
        psShapeSt->HarmShapeGain_smth += SUBFR_SMTH_COEF * ( HarmShapeGain - psShapeSt->HarmShapeGain_smth );
        psEncCtrl->HarmShapeGain[ k ]  = psShapeSt->HarmShapeGain_smth;
        psShapeSt->Tilt_smth          += SUBFR_SMTH_COEF * ( Tilt - psShapeSt->Tilt_smth );
        psEncCtrl->Tilt[ k ]           = psShapeSt->Tilt_smth;
    }
}
