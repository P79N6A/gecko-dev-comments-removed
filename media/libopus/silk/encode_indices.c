


























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main.h"


void silk_encode_indices(
    silk_encoder_state          *psEncC,                        
    ec_enc                      *psRangeEnc,                    
    opus_int                    FrameIndex,                     
    opus_int                    encode_LBRR,                    
    opus_int                    condCoding                      
)
{
    opus_int   i, k, typeOffset;
    opus_int   encode_absolute_lagIndex, delta_lagIndex;
    opus_int16 ec_ix[ MAX_LPC_ORDER ];
    opus_uint8 pred_Q8[ MAX_LPC_ORDER ];
    const SideInfoIndices *psIndices;

    if( encode_LBRR ) {
         psIndices = &psEncC->indices_LBRR[ FrameIndex ];
    } else {
         psIndices = &psEncC->indices;
    }

    
    
    
    typeOffset = 2 * psIndices->signalType + psIndices->quantOffsetType;
    silk_assert( typeOffset >= 0 && typeOffset < 6 );
    silk_assert( encode_LBRR == 0 || typeOffset >= 2 );
    if( encode_LBRR || typeOffset >= 2 ) {
        ec_enc_icdf( psRangeEnc, typeOffset - 2, silk_type_offset_VAD_iCDF, 8 );
    } else {
        ec_enc_icdf( psRangeEnc, typeOffset, silk_type_offset_no_VAD_iCDF, 8 );
    }

    
    
    
    
    if( condCoding == CODE_CONDITIONALLY ) {
        
        silk_assert( psIndices->GainsIndices[ 0 ] >= 0 && psIndices->GainsIndices[ 0 ] < MAX_DELTA_GAIN_QUANT - MIN_DELTA_GAIN_QUANT + 1 );
        ec_enc_icdf( psRangeEnc, psIndices->GainsIndices[ 0 ], silk_delta_gain_iCDF, 8 );
    } else {
        
        silk_assert( psIndices->GainsIndices[ 0 ] >= 0 && psIndices->GainsIndices[ 0 ] < N_LEVELS_QGAIN );
        ec_enc_icdf( psRangeEnc, silk_RSHIFT( psIndices->GainsIndices[ 0 ], 3 ), silk_gain_iCDF[ psIndices->signalType ], 8 );
        ec_enc_icdf( psRangeEnc, psIndices->GainsIndices[ 0 ] & 7, silk_uniform8_iCDF, 8 );
    }

    
    for( i = 1; i < psEncC->nb_subfr; i++ ) {
        silk_assert( psIndices->GainsIndices[ i ] >= 0 && psIndices->GainsIndices[ i ] < MAX_DELTA_GAIN_QUANT - MIN_DELTA_GAIN_QUANT + 1 );
        ec_enc_icdf( psRangeEnc, psIndices->GainsIndices[ i ], silk_delta_gain_iCDF, 8 );
    }

    
    
    
    ec_enc_icdf( psRangeEnc, psIndices->NLSFIndices[ 0 ], &psEncC->psNLSF_CB->CB1_iCDF[ ( psIndices->signalType >> 1 ) * psEncC->psNLSF_CB->nVectors ], 8 );
    silk_NLSF_unpack( ec_ix, pred_Q8, psEncC->psNLSF_CB, psIndices->NLSFIndices[ 0 ] );
    silk_assert( psEncC->psNLSF_CB->order == psEncC->predictLPCOrder );
    for( i = 0; i < psEncC->psNLSF_CB->order; i++ ) {
        if( psIndices->NLSFIndices[ i+1 ] >= NLSF_QUANT_MAX_AMPLITUDE ) {
            ec_enc_icdf( psRangeEnc, 2 * NLSF_QUANT_MAX_AMPLITUDE, &psEncC->psNLSF_CB->ec_iCDF[ ec_ix[ i ] ], 8 );
            ec_enc_icdf( psRangeEnc, psIndices->NLSFIndices[ i+1 ] - NLSF_QUANT_MAX_AMPLITUDE, silk_NLSF_EXT_iCDF, 8 );
        } else if( psIndices->NLSFIndices[ i+1 ] <= -NLSF_QUANT_MAX_AMPLITUDE ) {
            ec_enc_icdf( psRangeEnc, 0, &psEncC->psNLSF_CB->ec_iCDF[ ec_ix[ i ] ], 8 );
            ec_enc_icdf( psRangeEnc, -psIndices->NLSFIndices[ i+1 ] - NLSF_QUANT_MAX_AMPLITUDE, silk_NLSF_EXT_iCDF, 8 );
        } else {
            ec_enc_icdf( psRangeEnc, psIndices->NLSFIndices[ i+1 ] + NLSF_QUANT_MAX_AMPLITUDE, &psEncC->psNLSF_CB->ec_iCDF[ ec_ix[ i ] ], 8 );
        }
    }

    
    if( psEncC->nb_subfr == MAX_NB_SUBFR ) {
        silk_assert( psIndices->NLSFInterpCoef_Q2 >= 0 && psIndices->NLSFInterpCoef_Q2 < 5 );
        ec_enc_icdf( psRangeEnc, psIndices->NLSFInterpCoef_Q2, silk_NLSF_interpolation_factor_iCDF, 8 );
    }

    if( psIndices->signalType == TYPE_VOICED )
    {
        
        
        
        
        encode_absolute_lagIndex = 1;
        if( condCoding == CODE_CONDITIONALLY && psEncC->ec_prevSignalType == TYPE_VOICED ) {
            
            delta_lagIndex = psIndices->lagIndex - psEncC->ec_prevLagIndex;
            if( delta_lagIndex < -8 || delta_lagIndex > 11 ) {
                delta_lagIndex = 0;
            } else {
                delta_lagIndex = delta_lagIndex + 9;
                encode_absolute_lagIndex = 0; 
            }
            silk_assert( delta_lagIndex >= 0 && delta_lagIndex < 21 );
            ec_enc_icdf( psRangeEnc, delta_lagIndex, silk_pitch_delta_iCDF, 8 );
        }
        if( encode_absolute_lagIndex ) {
            
            opus_int32 pitch_high_bits, pitch_low_bits;
            pitch_high_bits = silk_DIV32_16( psIndices->lagIndex, silk_RSHIFT( psEncC->fs_kHz, 1 ) );
            pitch_low_bits = psIndices->lagIndex - silk_SMULBB( pitch_high_bits, silk_RSHIFT( psEncC->fs_kHz, 1 ) );
            silk_assert( pitch_low_bits < psEncC->fs_kHz / 2 );
            silk_assert( pitch_high_bits < 32 );
            ec_enc_icdf( psRangeEnc, pitch_high_bits, silk_pitch_lag_iCDF, 8 );
            ec_enc_icdf( psRangeEnc, pitch_low_bits, psEncC->pitch_lag_low_bits_iCDF, 8 );
        }
        psEncC->ec_prevLagIndex = psIndices->lagIndex;

        
        silk_assert(   psIndices->contourIndex  >= 0 );
        silk_assert( ( psIndices->contourIndex < 34 && psEncC->fs_kHz  > 8 && psEncC->nb_subfr == 4 ) ||
                    ( psIndices->contourIndex < 11 && psEncC->fs_kHz == 8 && psEncC->nb_subfr == 4 ) ||
                    ( psIndices->contourIndex < 12 && psEncC->fs_kHz  > 8 && psEncC->nb_subfr == 2 ) ||
                    ( psIndices->contourIndex <  3 && psEncC->fs_kHz == 8 && psEncC->nb_subfr == 2 ) );
        ec_enc_icdf( psRangeEnc, psIndices->contourIndex, psEncC->pitch_contour_iCDF, 8 );

        
        
        
        
        silk_assert( psIndices->PERIndex >= 0 && psIndices->PERIndex < 3 );
        ec_enc_icdf( psRangeEnc, psIndices->PERIndex, silk_LTP_per_index_iCDF, 8 );

        
        for( k = 0; k < psEncC->nb_subfr; k++ ) {
            silk_assert( psIndices->LTPIndex[ k ] >= 0 && psIndices->LTPIndex[ k ] < ( 8 << psIndices->PERIndex ) );
            ec_enc_icdf( psRangeEnc, psIndices->LTPIndex[ k ], silk_LTP_gain_iCDF_ptrs[ psIndices->PERIndex ], 8 );
        }

        
        
        
        if( condCoding == CODE_INDEPENDENTLY ) {
            silk_assert( psIndices->LTP_scaleIndex >= 0 && psIndices->LTP_scaleIndex < 3 );
            ec_enc_icdf( psRangeEnc, psIndices->LTP_scaleIndex, silk_LTPscale_iCDF, 8 );
        }
        silk_assert( !condCoding || psIndices->LTP_scaleIndex == 0 );
    }

    psEncC->ec_prevSignalType = psIndices->signalType;

    
    
    
    silk_assert( psIndices->Seed >= 0 && psIndices->Seed < 4 );
    ec_enc_icdf( psRangeEnc, psIndices->Seed, silk_uniform4_iCDF, 8 );
}
