






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include "main.h"


static inline void silk_VAD_GetNoiseLevels(
    const opus_int32             pX[ VAD_N_BANDS ], 
    silk_VAD_state              *psSilk_VAD         
);




opus_int silk_VAD_Init(                                         
    silk_VAD_state              *psSilk_VAD                     
)
{
    opus_int b, ret = 0;

    
    silk_memset( psSilk_VAD, 0, sizeof( silk_VAD_state ) );

    
    
    for( b = 0; b < VAD_N_BANDS; b++ ) {
        psSilk_VAD->NoiseLevelBias[ b ] = silk_max_32( silk_DIV32_16( VAD_NOISE_LEVELS_BIAS, b + 1 ), 1 );
    }

    
    for( b = 0; b < VAD_N_BANDS; b++ ) {
        psSilk_VAD->NL[ b ]     = silk_MUL( 100, psSilk_VAD->NoiseLevelBias[ b ] );
        psSilk_VAD->inv_NL[ b ] = silk_DIV32( silk_int32_MAX, psSilk_VAD->NL[ b ] );
    }
    psSilk_VAD->counter = 15;

    
    for( b = 0; b < VAD_N_BANDS; b++ ) {
        psSilk_VAD->NrgRatioSmth_Q8[ b ] = 100 * 256;       
    }

    return( ret );
}


static const opus_int32 tiltWeights[ VAD_N_BANDS ] = { 30000, 6000, -12000, -12000 };




opus_int silk_VAD_GetSA_Q8(                                     
    silk_encoder_state          *psEncC,                        
    const opus_int16            pIn[]                           
)
{
    opus_int   SA_Q15, pSNR_dB_Q7, input_tilt;
    opus_int   decimated_framelength, dec_subframe_length, dec_subframe_offset, SNR_Q7, i, b, s;
    opus_int32 sumSquared, smooth_coef_Q16;
    opus_int16 HPstateTmp;
    opus_int16 X[ VAD_N_BANDS ][ MAX_FRAME_LENGTH / 2 ];
    opus_int32 Xnrg[ VAD_N_BANDS ];
    opus_int32 NrgToNoiseRatio_Q8[ VAD_N_BANDS ];
    opus_int32 speech_nrg, x_tmp;
    opus_int   ret = 0;
    silk_VAD_state *psSilk_VAD = &psEncC->sVAD;

    
    silk_assert( VAD_N_BANDS == 4 );
    silk_assert( MAX_FRAME_LENGTH >= psEncC->frame_length );
    silk_assert( psEncC->frame_length <= 512 );
    silk_assert( psEncC->frame_length == 8 * silk_RSHIFT( psEncC->frame_length, 3 ) );

    
    
    
    
    silk_ana_filt_bank_1( pIn,          &psSilk_VAD->AnaState[  0 ], &X[ 0 ][ 0 ], &X[ 3 ][ 0 ], psEncC->frame_length );

    
    silk_ana_filt_bank_1( &X[ 0 ][ 0 ], &psSilk_VAD->AnaState1[ 0 ], &X[ 0 ][ 0 ], &X[ 2 ][ 0 ], silk_RSHIFT( psEncC->frame_length, 1 ) );

    
    silk_ana_filt_bank_1( &X[ 0 ][ 0 ], &psSilk_VAD->AnaState2[ 0 ], &X[ 0 ][ 0 ], &X[ 1 ][ 0 ], silk_RSHIFT( psEncC->frame_length, 2 ) );

    
    
    
    decimated_framelength = silk_RSHIFT( psEncC->frame_length, 3 );
    X[ 0 ][ decimated_framelength - 1 ] = silk_RSHIFT( X[ 0 ][ decimated_framelength - 1 ], 1 );
    HPstateTmp = X[ 0 ][ decimated_framelength - 1 ];
    for( i = decimated_framelength - 1; i > 0; i-- ) {
        X[ 0 ][ i - 1 ]  = silk_RSHIFT( X[ 0 ][ i - 1 ], 1 );
        X[ 0 ][ i ]     -= X[ 0 ][ i - 1 ];
    }
    X[ 0 ][ 0 ] -= psSilk_VAD->HPstate;
    psSilk_VAD->HPstate = HPstateTmp;

    
    
    
    for( b = 0; b < VAD_N_BANDS; b++ ) {
        
        decimated_framelength = silk_RSHIFT( psEncC->frame_length, silk_min_int( VAD_N_BANDS - b, VAD_N_BANDS - 1 ) );

        
        dec_subframe_length = silk_RSHIFT( decimated_framelength, VAD_INTERNAL_SUBFRAMES_LOG2 );
        dec_subframe_offset = 0;

        
        
        Xnrg[ b ] = psSilk_VAD->XnrgSubfr[ b ];
        for( s = 0; s < VAD_INTERNAL_SUBFRAMES; s++ ) {
            sumSquared = 0;
            for( i = 0; i < dec_subframe_length; i++ ) {
                
                
                x_tmp = silk_RSHIFT( X[ b ][ i + dec_subframe_offset ], 3 );
                sumSquared = silk_SMLABB( sumSquared, x_tmp, x_tmp );

                
                silk_assert( sumSquared >= 0 );
            }

            
            if( s < VAD_INTERNAL_SUBFRAMES - 1 ) {
                Xnrg[ b ] = silk_ADD_POS_SAT32( Xnrg[ b ], sumSquared );
            } else {
                
                Xnrg[ b ] = silk_ADD_POS_SAT32( Xnrg[ b ], silk_RSHIFT( sumSquared, 1 ) );
            }

            dec_subframe_offset += dec_subframe_length;
        }
        psSilk_VAD->XnrgSubfr[ b ] = sumSquared;
    }

    
    
    
    silk_VAD_GetNoiseLevels( &Xnrg[ 0 ], psSilk_VAD );

    
    
    
    sumSquared = 0;
    input_tilt = 0;
    for( b = 0; b < VAD_N_BANDS; b++ ) {
        speech_nrg = Xnrg[ b ] - psSilk_VAD->NL[ b ];
        if( speech_nrg > 0 ) {
            
            if( ( Xnrg[ b ] & 0xFF800000 ) == 0 ) {
                NrgToNoiseRatio_Q8[ b ] = silk_DIV32( silk_LSHIFT( Xnrg[ b ], 8 ), psSilk_VAD->NL[ b ] + 1 );
            } else {
                NrgToNoiseRatio_Q8[ b ] = silk_DIV32( Xnrg[ b ], silk_RSHIFT( psSilk_VAD->NL[ b ], 8 ) + 1 );
            }

            
            SNR_Q7 = silk_lin2log( NrgToNoiseRatio_Q8[ b ] ) - 8 * 128;

            
            sumSquared = silk_SMLABB( sumSquared, SNR_Q7, SNR_Q7 );          

            
            if( speech_nrg < ( 1 << 20 ) ) {
                
                SNR_Q7 = silk_SMULWB( silk_LSHIFT( silk_SQRT_APPROX( speech_nrg ), 6 ), SNR_Q7 );
            }
            input_tilt = silk_SMLAWB( input_tilt, tiltWeights[ b ], SNR_Q7 );
        } else {
            NrgToNoiseRatio_Q8[ b ] = 256;
        }
    }

    
    sumSquared = silk_DIV32_16( sumSquared, VAD_N_BANDS ); 

    
    pSNR_dB_Q7 = (opus_int16)( 3 * silk_SQRT_APPROX( sumSquared ) ); 

    
    
    
    SA_Q15 = silk_sigm_Q15( silk_SMULWB( VAD_SNR_FACTOR_Q16, pSNR_dB_Q7 ) - VAD_NEGATIVE_OFFSET_Q5 );

    
    
    
    psEncC->input_tilt_Q15 = silk_LSHIFT( silk_sigm_Q15( input_tilt ) - 16384, 1 );

    
    
    
    speech_nrg = 0;
    for( b = 0; b < VAD_N_BANDS; b++ ) {
        
        speech_nrg += ( b + 1 ) * silk_RSHIFT( Xnrg[ b ] - psSilk_VAD->NL[ b ], 4 );
    }

    
    if( speech_nrg <= 0 ) {
        SA_Q15 = silk_RSHIFT( SA_Q15, 1 );
    } else if( speech_nrg < 32768 ) {
        if( psEncC->frame_length == 10 * psEncC->fs_kHz ) {
            speech_nrg = silk_LSHIFT_SAT32( speech_nrg, 16 );
        } else {
            speech_nrg = silk_LSHIFT_SAT32( speech_nrg, 15 );
        }

        
        speech_nrg = silk_SQRT_APPROX( speech_nrg );
        SA_Q15 = silk_SMULWB( 32768 + speech_nrg, SA_Q15 );
    }

    
    psEncC->speech_activity_Q8 = silk_min_int( silk_RSHIFT( SA_Q15, 7 ), silk_uint8_MAX );

    
    
    
    
    smooth_coef_Q16 = silk_SMULWB( VAD_SNR_SMOOTH_COEF_Q18, silk_SMULWB( SA_Q15, SA_Q15 ) );

    if( psEncC->frame_length == 10 * psEncC->fs_kHz ) {
        smooth_coef_Q16 >>= 1;
    }

    for( b = 0; b < VAD_N_BANDS; b++ ) {
        
        psSilk_VAD->NrgRatioSmth_Q8[ b ] = silk_SMLAWB( psSilk_VAD->NrgRatioSmth_Q8[ b ],
            NrgToNoiseRatio_Q8[ b ] - psSilk_VAD->NrgRatioSmth_Q8[ b ], smooth_coef_Q16 );

        
        SNR_Q7 = 3 * ( silk_lin2log( psSilk_VAD->NrgRatioSmth_Q8[b] ) - 8 * 128 );
        
        psEncC->input_quality_bands_Q15[ b ] = silk_sigm_Q15( silk_RSHIFT( SNR_Q7 - 16 * 128, 4 ) );
    }

    return( ret );
}




static inline void silk_VAD_GetNoiseLevels(
    const opus_int32            pX[ VAD_N_BANDS ],  
    silk_VAD_state              *psSilk_VAD         
)
{
    opus_int   k;
    opus_int32 nl, nrg, inv_nrg;
    opus_int   coef, min_coef;

    
    if( psSilk_VAD->counter < 1000 ) { 
        min_coef = silk_DIV32_16( silk_int16_MAX, silk_RSHIFT( psSilk_VAD->counter, 4 ) + 1 );
    } else {
        min_coef = 0;
    }

    for( k = 0; k < VAD_N_BANDS; k++ ) {
        
        nl = psSilk_VAD->NL[ k ];
        silk_assert( nl >= 0 );

        
        nrg = silk_ADD_POS_SAT32( pX[ k ], psSilk_VAD->NoiseLevelBias[ k ] );
        silk_assert( nrg > 0 );

        
        inv_nrg = silk_DIV32( silk_int32_MAX, nrg );
        silk_assert( inv_nrg >= 0 );

        
        if( nrg > silk_LSHIFT( nl, 3 ) ) {
            coef = VAD_NOISE_LEVEL_SMOOTH_COEF_Q16 >> 3;
        } else if( nrg < nl ) {
            coef = VAD_NOISE_LEVEL_SMOOTH_COEF_Q16;
        } else {
            coef = silk_SMULWB( silk_SMULWW( inv_nrg, nl ), VAD_NOISE_LEVEL_SMOOTH_COEF_Q16 << 1 );
        }

        
        coef = silk_max_int( coef, min_coef );

        
        psSilk_VAD->inv_NL[ k ] = silk_SMLAWB( psSilk_VAD->inv_NL[ k ], inv_nrg - psSilk_VAD->inv_NL[ k ], coef );
        silk_assert( psSilk_VAD->inv_NL[ k ] >= 0 );

        
        nl = silk_DIV32( silk_int32_MAX, psSilk_VAD->inv_NL[ k ] );
        silk_assert( nl >= 0 );

        
        nl = silk_min( nl, 0x00FFFFFF );

        
        psSilk_VAD->NL[ k ] = nl;
    }

    
    psSilk_VAD->counter++;
}
