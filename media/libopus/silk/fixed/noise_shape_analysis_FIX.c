






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main_FIX.h"
#include "tuning_parameters.h"





static inline opus_int32 warped_gain( 
    const opus_int32     *coefs_Q24,
    opus_int             lambda_Q16,
    opus_int             order
) {
    opus_int   i;
    opus_int32 gain_Q24;

    lambda_Q16 = -lambda_Q16;
    gain_Q24 = coefs_Q24[ order - 1 ];
    for( i = order - 2; i >= 0; i-- ) {
        gain_Q24 = silk_SMLAWB( coefs_Q24[ i ], gain_Q24, lambda_Q16 );
    }
    gain_Q24  = silk_SMLAWB( SILK_FIX_CONST( 1.0, 24 ), gain_Q24, -lambda_Q16 );
    return silk_INVERSE32_varQ( gain_Q24, 40 );
}



static inline void limit_warped_coefs(
    opus_int32           *coefs_syn_Q24,
    opus_int32           *coefs_ana_Q24,
    opus_int             lambda_Q16,
    opus_int32           limit_Q24,
    opus_int             order
) {
    opus_int   i, iter, ind = 0;
    opus_int32 tmp, maxabs_Q24, chirp_Q16, gain_syn_Q16, gain_ana_Q16;
    opus_int32 nom_Q16, den_Q24;

    
    lambda_Q16 = -lambda_Q16;
    for( i = order - 1; i > 0; i-- ) {
        coefs_syn_Q24[ i - 1 ] = silk_SMLAWB( coefs_syn_Q24[ i - 1 ], coefs_syn_Q24[ i ], lambda_Q16 );
        coefs_ana_Q24[ i - 1 ] = silk_SMLAWB( coefs_ana_Q24[ i - 1 ], coefs_ana_Q24[ i ], lambda_Q16 );
    }
    lambda_Q16 = -lambda_Q16;
    nom_Q16  = silk_SMLAWB( SILK_FIX_CONST( 1.0, 16 ), -lambda_Q16,        lambda_Q16 );
    den_Q24  = silk_SMLAWB( SILK_FIX_CONST( 1.0, 24 ), coefs_syn_Q24[ 0 ], lambda_Q16 );
    gain_syn_Q16 = silk_DIV32_varQ( nom_Q16, den_Q24, 24 );
    den_Q24  = silk_SMLAWB( SILK_FIX_CONST( 1.0, 24 ), coefs_ana_Q24[ 0 ], lambda_Q16 );
    gain_ana_Q16 = silk_DIV32_varQ( nom_Q16, den_Q24, 24 );
    for( i = 0; i < order; i++ ) {
        coefs_syn_Q24[ i ] = silk_SMULWW( gain_syn_Q16, coefs_syn_Q24[ i ] );
        coefs_ana_Q24[ i ] = silk_SMULWW( gain_ana_Q16, coefs_ana_Q24[ i ] );
    }

    for( iter = 0; iter < 10; iter++ ) {
        
        maxabs_Q24 = -1;
        for( i = 0; i < order; i++ ) {
            tmp = silk_max( silk_abs_int32( coefs_syn_Q24[ i ] ), silk_abs_int32( coefs_ana_Q24[ i ] ) );
            if( tmp > maxabs_Q24 ) {
                maxabs_Q24 = tmp;
                ind = i;
            }
        }
        if( maxabs_Q24 <= limit_Q24 ) {
            
            return;
        }

        
        for( i = 1; i < order; i++ ) {
            coefs_syn_Q24[ i - 1 ] = silk_SMLAWB( coefs_syn_Q24[ i - 1 ], coefs_syn_Q24[ i ], lambda_Q16 );
            coefs_ana_Q24[ i - 1 ] = silk_SMLAWB( coefs_ana_Q24[ i - 1 ], coefs_ana_Q24[ i ], lambda_Q16 );
        }
        gain_syn_Q16 = silk_INVERSE32_varQ( gain_syn_Q16, 32 );
        gain_ana_Q16 = silk_INVERSE32_varQ( gain_ana_Q16, 32 );
        for( i = 0; i < order; i++ ) {
            coefs_syn_Q24[ i ] = silk_SMULWW( gain_syn_Q16, coefs_syn_Q24[ i ] );
            coefs_ana_Q24[ i ] = silk_SMULWW( gain_ana_Q16, coefs_ana_Q24[ i ] );
        }

        
        chirp_Q16 = SILK_FIX_CONST( 0.99, 16 ) - silk_DIV32_varQ(
            silk_SMULWB( maxabs_Q24 - limit_Q24, silk_SMLABB( SILK_FIX_CONST( 0.8, 10 ), SILK_FIX_CONST( 0.1, 10 ), iter ) ),
            silk_MUL( maxabs_Q24, ind + 1 ), 22 );
        silk_bwexpander_32( coefs_syn_Q24, order, chirp_Q16 );
        silk_bwexpander_32( coefs_ana_Q24, order, chirp_Q16 );

        
        lambda_Q16 = -lambda_Q16;
        for( i = order - 1; i > 0; i-- ) {
            coefs_syn_Q24[ i - 1 ] = silk_SMLAWB( coefs_syn_Q24[ i - 1 ], coefs_syn_Q24[ i ], lambda_Q16 );
            coefs_ana_Q24[ i - 1 ] = silk_SMLAWB( coefs_ana_Q24[ i - 1 ], coefs_ana_Q24[ i ], lambda_Q16 );
        }
        lambda_Q16 = -lambda_Q16;
        nom_Q16  = silk_SMLAWB( SILK_FIX_CONST( 1.0, 16 ), -lambda_Q16,        lambda_Q16 );
        den_Q24  = silk_SMLAWB( SILK_FIX_CONST( 1.0, 24 ), coefs_syn_Q24[ 0 ], lambda_Q16 );
        gain_syn_Q16 = silk_DIV32_varQ( nom_Q16, den_Q24, 24 );
        den_Q24  = silk_SMLAWB( SILK_FIX_CONST( 1.0, 24 ), coefs_ana_Q24[ 0 ], lambda_Q16 );
        gain_ana_Q16 = silk_DIV32_varQ( nom_Q16, den_Q24, 24 );
        for( i = 0; i < order; i++ ) {
            coefs_syn_Q24[ i ] = silk_SMULWW( gain_syn_Q16, coefs_syn_Q24[ i ] );
            coefs_ana_Q24[ i ] = silk_SMULWW( gain_ana_Q16, coefs_ana_Q24[ i ] );
        }
    }
    silk_assert( 0 );
}




void silk_noise_shape_analysis_FIX(
    silk_encoder_state_FIX          *psEnc,                                 
    silk_encoder_control_FIX        *psEncCtrl,                             
    const opus_int16                *pitch_res,                             
    const opus_int16                *x                                      
)
{
    silk_shape_state_FIX *psShapeSt = &psEnc->sShape;
    opus_int     k, i, nSamples, Qnrg, b_Q14, warping_Q16, scale = 0;
    opus_int32   SNR_adj_dB_Q7, HarmBoost_Q16, HarmShapeGain_Q16, Tilt_Q16, tmp32;
    opus_int32   nrg, pre_nrg_Q30, log_energy_Q7, log_energy_prev_Q7, energy_variation_Q7;
    opus_int32   delta_Q16, BWExp1_Q16, BWExp2_Q16, gain_mult_Q16, gain_add_Q16, strength_Q16, b_Q8;
    opus_int32   auto_corr[     MAX_SHAPE_LPC_ORDER + 1 ];
    opus_int32   refl_coef_Q16[ MAX_SHAPE_LPC_ORDER ];
    opus_int32   AR1_Q24[       MAX_SHAPE_LPC_ORDER ];
    opus_int32   AR2_Q24[       MAX_SHAPE_LPC_ORDER ];
    opus_int16   x_windowed[    SHAPE_LPC_WIN_MAX ];
    const opus_int16 *x_ptr, *pitch_res_ptr;

    
    x_ptr = x - psEnc->sCmn.la_shape;

    
    
    
    SNR_adj_dB_Q7 = psEnc->sCmn.SNR_dB_Q7;

    
    psEncCtrl->input_quality_Q14 = ( opus_int )silk_RSHIFT( (opus_int32)psEnc->sCmn.input_quality_bands_Q15[ 0 ]
        + psEnc->sCmn.input_quality_bands_Q15[ 1 ], 2 );

    
    psEncCtrl->coding_quality_Q14 = silk_RSHIFT( silk_sigm_Q15( silk_RSHIFT_ROUND( SNR_adj_dB_Q7 -
        SILK_FIX_CONST( 20.0, 7 ), 4 ) ), 1 );

    
    if( psEnc->sCmn.useCBR == 0 ) {
        b_Q8 = SILK_FIX_CONST( 1.0, 8 ) - psEnc->sCmn.speech_activity_Q8;
        b_Q8 = silk_SMULWB( silk_LSHIFT( b_Q8, 8 ), b_Q8 );
        SNR_adj_dB_Q7 = silk_SMLAWB( SNR_adj_dB_Q7,
            silk_SMULBB( SILK_FIX_CONST( -BG_SNR_DECR_dB, 7 ) >> ( 4 + 1 ), b_Q8 ),                                       
            silk_SMULWB( SILK_FIX_CONST( 1.0, 14 ) + psEncCtrl->input_quality_Q14, psEncCtrl->coding_quality_Q14 ) );     
    }

    if( psEnc->sCmn.indices.signalType == TYPE_VOICED ) {
        
        SNR_adj_dB_Q7 = silk_SMLAWB( SNR_adj_dB_Q7, SILK_FIX_CONST( HARM_SNR_INCR_dB, 8 ), psEnc->LTPCorr_Q15 );
    } else {
        
        SNR_adj_dB_Q7 = silk_SMLAWB( SNR_adj_dB_Q7,
            silk_SMLAWB( SILK_FIX_CONST( 6.0, 9 ), -SILK_FIX_CONST( 0.4, 18 ), psEnc->sCmn.SNR_dB_Q7 ),
            SILK_FIX_CONST( 1.0, 14 ) - psEncCtrl->input_quality_Q14 );
    }

    
    
    
    
    if( psEnc->sCmn.indices.signalType == TYPE_VOICED ) {
        
        psEnc->sCmn.indices.quantOffsetType = 0;
        psEncCtrl->sparseness_Q8 = 0;
    } else {
        
        nSamples = silk_LSHIFT( psEnc->sCmn.fs_kHz, 1 );
        energy_variation_Q7 = 0;
        log_energy_prev_Q7  = 0;
        pitch_res_ptr = pitch_res;
        for( k = 0; k < silk_SMULBB( SUB_FRAME_LENGTH_MS, psEnc->sCmn.nb_subfr ) / 2; k++ ) {
            silk_sum_sqr_shift( &nrg, &scale, pitch_res_ptr, nSamples );
            nrg += silk_RSHIFT( nSamples, scale );           

            log_energy_Q7 = silk_lin2log( nrg );
            if( k > 0 ) {
                energy_variation_Q7 += silk_abs( log_energy_Q7 - log_energy_prev_Q7 );
            }
            log_energy_prev_Q7 = log_energy_Q7;
            pitch_res_ptr += nSamples;
        }

        psEncCtrl->sparseness_Q8 = silk_RSHIFT( silk_sigm_Q15( silk_SMULWB( energy_variation_Q7 -
            SILK_FIX_CONST( 5.0, 7 ), SILK_FIX_CONST( 0.1, 16 ) ) ), 7 );

        
        if( psEncCtrl->sparseness_Q8 > SILK_FIX_CONST( SPARSENESS_THRESHOLD_QNT_OFFSET, 8 ) ) {
            psEnc->sCmn.indices.quantOffsetType = 0;
        } else {
            psEnc->sCmn.indices.quantOffsetType = 1;
        }

        
        SNR_adj_dB_Q7 = silk_SMLAWB( SNR_adj_dB_Q7, SILK_FIX_CONST( SPARSE_SNR_INCR_dB, 15 ), psEncCtrl->sparseness_Q8 - SILK_FIX_CONST( 0.5, 8 ) );
    }

    
    
    
    
    strength_Q16 = silk_SMULWB( psEncCtrl->predGain_Q16, SILK_FIX_CONST( FIND_PITCH_WHITE_NOISE_FRACTION, 16 ) );
    BWExp1_Q16 = BWExp2_Q16 = silk_DIV32_varQ( SILK_FIX_CONST( BANDWIDTH_EXPANSION, 16 ),
        silk_SMLAWW( SILK_FIX_CONST( 1.0, 16 ), strength_Q16, strength_Q16 ), 16 );
    delta_Q16  = silk_SMULWB( SILK_FIX_CONST( 1.0, 16 ) - silk_SMULBB( 3, psEncCtrl->coding_quality_Q14 ),
        SILK_FIX_CONST( LOW_RATE_BANDWIDTH_EXPANSION_DELTA, 16 ) );
    BWExp1_Q16 = silk_SUB32( BWExp1_Q16, delta_Q16 );
    BWExp2_Q16 = silk_ADD32( BWExp2_Q16, delta_Q16 );
    
    BWExp1_Q16 = silk_DIV32_16( silk_LSHIFT( BWExp1_Q16, 14 ), silk_RSHIFT( BWExp2_Q16, 2 ) );

    if( psEnc->sCmn.warping_Q16 > 0 ) {
        
        warping_Q16 = silk_SMLAWB( psEnc->sCmn.warping_Q16, psEncCtrl->coding_quality_Q14, SILK_FIX_CONST( 0.01, 18 ) );
    } else {
        warping_Q16 = 0;
    }

    
    
    
    for( k = 0; k < psEnc->sCmn.nb_subfr; k++ ) {
        
        opus_int shift, slope_part, flat_part;
        flat_part = psEnc->sCmn.fs_kHz * 3;
        slope_part = silk_RSHIFT( psEnc->sCmn.shapeWinLength - flat_part, 1 );

        silk_apply_sine_window( x_windowed, x_ptr, 1, slope_part );
        shift = slope_part;
        silk_memcpy( x_windowed + shift, x_ptr + shift, flat_part * sizeof(opus_int16) );
        shift += flat_part;
        silk_apply_sine_window( x_windowed + shift, x_ptr + shift, 2, slope_part );

        
        x_ptr += psEnc->sCmn.subfr_length;

        if( psEnc->sCmn.warping_Q16 > 0 ) {
            
            silk_warped_autocorrelation_FIX( auto_corr, &scale, x_windowed, warping_Q16, psEnc->sCmn.shapeWinLength, psEnc->sCmn.shapingLPCOrder );
        } else {
            
            silk_autocorr( auto_corr, &scale, x_windowed, psEnc->sCmn.shapeWinLength, psEnc->sCmn.shapingLPCOrder + 1 );
        }

        
        auto_corr[0] = silk_ADD32( auto_corr[0], silk_max_32( silk_SMULWB( silk_RSHIFT( auto_corr[ 0 ], 4 ),
            SILK_FIX_CONST( SHAPE_WHITE_NOISE_FRACTION, 20 ) ), 1 ) );

        
        nrg = silk_schur64( refl_coef_Q16, auto_corr, psEnc->sCmn.shapingLPCOrder );
        silk_assert( nrg >= 0 );

        
        silk_k2a_Q16( AR2_Q24, refl_coef_Q16, psEnc->sCmn.shapingLPCOrder );

        Qnrg = -scale;          
        silk_assert( Qnrg >= -12 );
        silk_assert( Qnrg <=  30 );

        
        if( Qnrg & 1 ) {
            Qnrg -= 1;
            nrg >>= 1;
        }

        tmp32 = silk_SQRT_APPROX( nrg );
        Qnrg >>= 1;             

        psEncCtrl->Gains_Q16[ k ] = silk_LSHIFT_SAT32( tmp32, 16 - Qnrg );

        if( psEnc->sCmn.warping_Q16 > 0 ) {
            
            gain_mult_Q16 = warped_gain( AR2_Q24, warping_Q16, psEnc->sCmn.shapingLPCOrder );
            silk_assert( psEncCtrl->Gains_Q16[ k ] >= 0 );
            psEncCtrl->Gains_Q16[ k ] = silk_SMULWW( psEncCtrl->Gains_Q16[ k ], gain_mult_Q16 );
            if( psEncCtrl->Gains_Q16[ k ] < 0 ) {
                psEncCtrl->Gains_Q16[ k ] = silk_int32_MAX;
            }
        }

        
        silk_bwexpander_32( AR2_Q24, psEnc->sCmn.shapingLPCOrder, BWExp2_Q16 );

        
        silk_memcpy( AR1_Q24, AR2_Q24, psEnc->sCmn.shapingLPCOrder * sizeof( opus_int32 ) );

        
        silk_assert( BWExp1_Q16 <= SILK_FIX_CONST( 1.0, 16 ) );
        silk_bwexpander_32( AR1_Q24, psEnc->sCmn.shapingLPCOrder, BWExp1_Q16 );

        
        pre_nrg_Q30 = silk_LPC_inverse_pred_gain_Q24( AR2_Q24, psEnc->sCmn.shapingLPCOrder );
        nrg         = silk_LPC_inverse_pred_gain_Q24( AR1_Q24, psEnc->sCmn.shapingLPCOrder );

        
        pre_nrg_Q30 = silk_LSHIFT32( silk_SMULWB( pre_nrg_Q30, SILK_FIX_CONST( 0.7, 15 ) ), 1 );
        psEncCtrl->GainsPre_Q14[ k ] = ( opus_int ) SILK_FIX_CONST( 0.3, 14 ) + silk_DIV32_varQ( pre_nrg_Q30, nrg, 14 );

        
        limit_warped_coefs( AR2_Q24, AR1_Q24, warping_Q16, SILK_FIX_CONST( 3.999, 24 ), psEnc->sCmn.shapingLPCOrder );

        
        for( i = 0; i < psEnc->sCmn.shapingLPCOrder; i++ ) {
            psEncCtrl->AR1_Q13[ k * MAX_SHAPE_LPC_ORDER + i ] = (opus_int16)silk_SAT16( silk_RSHIFT_ROUND( AR1_Q24[ i ], 11 ) );
            psEncCtrl->AR2_Q13[ k * MAX_SHAPE_LPC_ORDER + i ] = (opus_int16)silk_SAT16( silk_RSHIFT_ROUND( AR2_Q24[ i ], 11 ) );
        }
    }

    
    
    
    
    gain_mult_Q16 = silk_log2lin( -silk_SMLAWB( -SILK_FIX_CONST( 16.0, 7 ), SNR_adj_dB_Q7, SILK_FIX_CONST( 0.16, 16 ) ) );
    gain_add_Q16  = silk_log2lin(  silk_SMLAWB(  SILK_FIX_CONST( 16.0, 7 ), SILK_FIX_CONST( MIN_QGAIN_DB, 7 ), SILK_FIX_CONST( 0.16, 16 ) ) );
    silk_assert( gain_mult_Q16 > 0 );
    for( k = 0; k < psEnc->sCmn.nb_subfr; k++ ) {
        psEncCtrl->Gains_Q16[ k ] = silk_SMULWW( psEncCtrl->Gains_Q16[ k ], gain_mult_Q16 );
        silk_assert( psEncCtrl->Gains_Q16[ k ] >= 0 );
        psEncCtrl->Gains_Q16[ k ] = silk_ADD_POS_SAT32( psEncCtrl->Gains_Q16[ k ], gain_add_Q16 );
    }

    gain_mult_Q16 = SILK_FIX_CONST( 1.0, 16 ) + silk_RSHIFT_ROUND( silk_MLA( SILK_FIX_CONST( INPUT_TILT, 26 ),
        psEncCtrl->coding_quality_Q14, SILK_FIX_CONST( HIGH_RATE_INPUT_TILT, 12 ) ), 10 );
    for( k = 0; k < psEnc->sCmn.nb_subfr; k++ ) {
        psEncCtrl->GainsPre_Q14[ k ] = silk_SMULWB( gain_mult_Q16, psEncCtrl->GainsPre_Q14[ k ] );
    }

    
    
    
    
    strength_Q16 = silk_MUL( SILK_FIX_CONST( LOW_FREQ_SHAPING, 4 ), silk_SMLAWB( SILK_FIX_CONST( 1.0, 12 ),
        SILK_FIX_CONST( LOW_QUALITY_LOW_FREQ_SHAPING_DECR, 13 ), psEnc->sCmn.input_quality_bands_Q15[ 0 ] - SILK_FIX_CONST( 1.0, 15 ) ) );
    strength_Q16 = silk_RSHIFT( silk_MUL( strength_Q16, psEnc->sCmn.speech_activity_Q8 ), 8 );
    if( psEnc->sCmn.indices.signalType == TYPE_VOICED ) {
        
        
        opus_int fs_kHz_inv = silk_DIV32_16( SILK_FIX_CONST( 0.2, 14 ), psEnc->sCmn.fs_kHz );
        for( k = 0; k < psEnc->sCmn.nb_subfr; k++ ) {
            b_Q14 = fs_kHz_inv + silk_DIV32_16( SILK_FIX_CONST( 3.0, 14 ), psEncCtrl->pitchL[ k ] );
            
            psEncCtrl->LF_shp_Q14[ k ]  = silk_LSHIFT( SILK_FIX_CONST( 1.0, 14 ) - b_Q14 - silk_SMULWB( strength_Q16, b_Q14 ), 16 );
            psEncCtrl->LF_shp_Q14[ k ] |= (opus_uint16)( b_Q14 - SILK_FIX_CONST( 1.0, 14 ) );
        }
        silk_assert( SILK_FIX_CONST( HARM_HP_NOISE_COEF, 24 ) < SILK_FIX_CONST( 0.5, 24 ) ); 
        Tilt_Q16 = - SILK_FIX_CONST( HP_NOISE_COEF, 16 ) -
            silk_SMULWB( SILK_FIX_CONST( 1.0, 16 ) - SILK_FIX_CONST( HP_NOISE_COEF, 16 ),
                silk_SMULWB( SILK_FIX_CONST( HARM_HP_NOISE_COEF, 24 ), psEnc->sCmn.speech_activity_Q8 ) );
    } else {
        b_Q14 = silk_DIV32_16( 21299, psEnc->sCmn.fs_kHz ); 
        
        psEncCtrl->LF_shp_Q14[ 0 ]  = silk_LSHIFT( SILK_FIX_CONST( 1.0, 14 ) - b_Q14 -
            silk_SMULWB( strength_Q16, silk_SMULWB( SILK_FIX_CONST( 0.6, 16 ), b_Q14 ) ), 16 );
        psEncCtrl->LF_shp_Q14[ 0 ] |= (opus_uint16)( b_Q14 - SILK_FIX_CONST( 1.0, 14 ) );
        for( k = 1; k < psEnc->sCmn.nb_subfr; k++ ) {
            psEncCtrl->LF_shp_Q14[ k ] = psEncCtrl->LF_shp_Q14[ 0 ];
        }
        Tilt_Q16 = -SILK_FIX_CONST( HP_NOISE_COEF, 16 );
    }

    
    
    
    
    HarmBoost_Q16 = silk_SMULWB( silk_SMULWB( SILK_FIX_CONST( 1.0, 17 ) - silk_LSHIFT( psEncCtrl->coding_quality_Q14, 3 ),
        psEnc->LTPCorr_Q15 ), SILK_FIX_CONST( LOW_RATE_HARMONIC_BOOST, 16 ) );

    
    HarmBoost_Q16 = silk_SMLAWB( HarmBoost_Q16,
        SILK_FIX_CONST( 1.0, 16 ) - silk_LSHIFT( psEncCtrl->input_quality_Q14, 2 ), SILK_FIX_CONST( LOW_INPUT_QUALITY_HARMONIC_BOOST, 16 ) );

    if( USE_HARM_SHAPING && psEnc->sCmn.indices.signalType == TYPE_VOICED ) {
        
        HarmShapeGain_Q16 = silk_SMLAWB( SILK_FIX_CONST( HARMONIC_SHAPING, 16 ),
                SILK_FIX_CONST( 1.0, 16 ) - silk_SMULWB( SILK_FIX_CONST( 1.0, 18 ) - silk_LSHIFT( psEncCtrl->coding_quality_Q14, 4 ),
                psEncCtrl->input_quality_Q14 ), SILK_FIX_CONST( HIGH_RATE_OR_LOW_QUALITY_HARMONIC_SHAPING, 16 ) );

        
        HarmShapeGain_Q16 = silk_SMULWB( silk_LSHIFT( HarmShapeGain_Q16, 1 ),
            silk_SQRT_APPROX( silk_LSHIFT( psEnc->LTPCorr_Q15, 15 ) ) );
    } else {
        HarmShapeGain_Q16 = 0;
    }

    
    
    
    for( k = 0; k < MAX_NB_SUBFR; k++ ) {
        psShapeSt->HarmBoost_smth_Q16 =
            silk_SMLAWB( psShapeSt->HarmBoost_smth_Q16,     HarmBoost_Q16     - psShapeSt->HarmBoost_smth_Q16,     SILK_FIX_CONST( SUBFR_SMTH_COEF, 16 ) );
        psShapeSt->HarmShapeGain_smth_Q16 =
            silk_SMLAWB( psShapeSt->HarmShapeGain_smth_Q16, HarmShapeGain_Q16 - psShapeSt->HarmShapeGain_smth_Q16, SILK_FIX_CONST( SUBFR_SMTH_COEF, 16 ) );
        psShapeSt->Tilt_smth_Q16 =
            silk_SMLAWB( psShapeSt->Tilt_smth_Q16,          Tilt_Q16          - psShapeSt->Tilt_smth_Q16,          SILK_FIX_CONST( SUBFR_SMTH_COEF, 16 ) );

        psEncCtrl->HarmBoost_Q14[ k ]     = ( opus_int )silk_RSHIFT_ROUND( psShapeSt->HarmBoost_smth_Q16,     2 );
        psEncCtrl->HarmShapeGain_Q14[ k ] = ( opus_int )silk_RSHIFT_ROUND( psShapeSt->HarmShapeGain_smth_Q16, 2 );
        psEncCtrl->Tilt_Q14[ k ]          = ( opus_int )silk_RSHIFT_ROUND( psShapeSt->Tilt_smth_Q16,          2 );
    }
}
