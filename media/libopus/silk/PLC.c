






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main.h"
#include "PLC.h"

#define NB_ATT 2
static const opus_int16 HARM_ATT_Q15[NB_ATT]              = { 32440, 31130 }; 
static const opus_int16 PLC_RAND_ATTENUATE_V_Q15[NB_ATT]  = { 31130, 26214 }; 
static const opus_int16 PLC_RAND_ATTENUATE_UV_Q15[NB_ATT] = { 32440, 29491 }; 

static inline void silk_PLC_update(
    silk_decoder_state                  *psDec,             
    silk_decoder_control                *psDecCtrl          
);

static inline void silk_PLC_conceal(
    silk_decoder_state                  *psDec,             
    silk_decoder_control                *psDecCtrl,         
    opus_int16                          signal[]            
);


void silk_PLC_Reset(
    silk_decoder_state                  *psDec              
)
{
    psDec->sPLC.pitchL_Q8 = silk_LSHIFT( psDec->frame_length, 8 - 1 );
    psDec->sPLC.prevGain_Q16[ 0 ] = SILK_FIX_CONST( 1, 16 );
    psDec->sPLC.prevGain_Q16[ 1 ] = SILK_FIX_CONST( 1, 16 );
    psDec->sPLC.subfr_length = 20;
    psDec->sPLC.nb_subfr = 2;
}

void silk_PLC(
    silk_decoder_state                  *psDec,             
    silk_decoder_control                *psDecCtrl,         
    opus_int16                          frame[],            
    opus_int                            lost                
)
{
    
    if( psDec->fs_kHz != psDec->sPLC.fs_kHz ) {
        silk_PLC_Reset( psDec );
        psDec->sPLC.fs_kHz = psDec->fs_kHz;
    }

    if( lost ) {
        
        
        
        silk_PLC_conceal( psDec, psDecCtrl, frame );

        psDec->lossCnt++;
    } else {
        
        
        
        silk_PLC_update( psDec, psDecCtrl );
    }
}




static inline void silk_PLC_update(
    silk_decoder_state                  *psDec,             
    silk_decoder_control                *psDecCtrl          
)
{
    opus_int32 LTP_Gain_Q14, temp_LTP_Gain_Q14;
    opus_int   i, j;
    silk_PLC_struct *psPLC;

    psPLC = &psDec->sPLC;

    
    psDec->prevSignalType = psDec->indices.signalType;
    LTP_Gain_Q14 = 0;
    if( psDec->indices.signalType == TYPE_VOICED ) {
        
        for( j = 0; j * psDec->subfr_length < psDecCtrl->pitchL[ psDec->nb_subfr - 1 ]; j++ ) {
            if( j == psDec->nb_subfr ) {
                break;
            }
            temp_LTP_Gain_Q14 = 0;
            for( i = 0; i < LTP_ORDER; i++ ) {
                temp_LTP_Gain_Q14 += psDecCtrl->LTPCoef_Q14[ ( psDec->nb_subfr - 1 - j ) * LTP_ORDER  + i ];
            }
            if( temp_LTP_Gain_Q14 > LTP_Gain_Q14 ) {
                LTP_Gain_Q14 = temp_LTP_Gain_Q14;
                silk_memcpy( psPLC->LTPCoef_Q14,
                    &psDecCtrl->LTPCoef_Q14[ silk_SMULBB( psDec->nb_subfr - 1 - j, LTP_ORDER ) ],
                    LTP_ORDER * sizeof( opus_int16 ) );

                psPLC->pitchL_Q8 = silk_LSHIFT( psDecCtrl->pitchL[ psDec->nb_subfr - 1 - j ], 8 );
            }
        }

        silk_memset( psPLC->LTPCoef_Q14, 0, LTP_ORDER * sizeof( opus_int16 ) );
        psPLC->LTPCoef_Q14[ LTP_ORDER / 2 ] = LTP_Gain_Q14;

        
        if( LTP_Gain_Q14 < V_PITCH_GAIN_START_MIN_Q14 ) {
            opus_int   scale_Q10;
            opus_int32 tmp;

            tmp = silk_LSHIFT( V_PITCH_GAIN_START_MIN_Q14, 10 );
            scale_Q10 = silk_DIV32( tmp, silk_max( LTP_Gain_Q14, 1 ) );
            for( i = 0; i < LTP_ORDER; i++ ) {
                psPLC->LTPCoef_Q14[ i ] = silk_RSHIFT( silk_SMULBB( psPLC->LTPCoef_Q14[ i ], scale_Q10 ), 10 );
            }
        } else if( LTP_Gain_Q14 > V_PITCH_GAIN_START_MAX_Q14 ) {
            opus_int   scale_Q14;
            opus_int32 tmp;

            tmp = silk_LSHIFT( V_PITCH_GAIN_START_MAX_Q14, 14 );
            scale_Q14 = silk_DIV32( tmp, silk_max( LTP_Gain_Q14, 1 ) );
            for( i = 0; i < LTP_ORDER; i++ ) {
                psPLC->LTPCoef_Q14[ i ] = silk_RSHIFT( silk_SMULBB( psPLC->LTPCoef_Q14[ i ], scale_Q14 ), 14 );
            }
        }
    } else {
        psPLC->pitchL_Q8 = silk_LSHIFT( silk_SMULBB( psDec->fs_kHz, 18 ), 8 );
        silk_memset( psPLC->LTPCoef_Q14, 0, LTP_ORDER * sizeof( opus_int16 ));
    }

    
    silk_memcpy( psPLC->prevLPC_Q12, psDecCtrl->PredCoef_Q12[ 1 ], psDec->LPC_order * sizeof( opus_int16 ) );
    psPLC->prevLTP_scale_Q14 = psDecCtrl->LTP_scale_Q14;

    
    silk_memcpy( psPLC->prevGain_Q16, &psDecCtrl->Gains_Q16[ psDec->nb_subfr - 2 ], 2 * sizeof( opus_int32 ) );

    psPLC->subfr_length = psDec->subfr_length;
    psPLC->nb_subfr = psDec->nb_subfr;
}

static inline void silk_PLC_conceal(
    silk_decoder_state                  *psDec,             
    silk_decoder_control                *psDecCtrl,         
    opus_int16                          frame[]             
)
{
    opus_int   i, j, k;
    opus_int   lag, idx, sLTP_buf_idx, shift1, shift2;
    opus_int32 rand_seed, harm_Gain_Q15, rand_Gain_Q15, inv_gain_Q30;
    opus_int32 energy1, energy2, *rand_ptr, *pred_lag_ptr;
    opus_int32 LPC_pred_Q10, LTP_pred_Q12;
    opus_int16 rand_scale_Q14;
    opus_int16 *B_Q14, *exc_buf_ptr;
    opus_int32 *sLPC_Q14_ptr;
    opus_int16 exc_buf[ 2 * MAX_SUB_FRAME_LENGTH ];
    opus_int16 A_Q12[ MAX_LPC_ORDER ];
    opus_int16 sLTP[ MAX_FRAME_LENGTH ];
    opus_int32 sLTP_Q14[ 2 * MAX_FRAME_LENGTH ];
    silk_PLC_struct *psPLC = &psDec->sPLC;
    opus_int32 prevGain_Q10[2];

    prevGain_Q10[0] = silk_RSHIFT( psPLC->prevGain_Q16[ 0 ], 6);
    prevGain_Q10[1] = silk_RSHIFT( psPLC->prevGain_Q16[ 1 ], 6);

    if( psDec->first_frame_after_reset ) {
       silk_memset( psPLC->prevLPC_Q12, 0, sizeof( psPLC->prevLPC_Q12 ) );
    }

    
    
    exc_buf_ptr = exc_buf;
    for( k = 0; k < 2; k++ ) {
        for( i = 0; i < psPLC->subfr_length; i++ ) {
            exc_buf_ptr[ i ] = (opus_int16)silk_SAT16( silk_RSHIFT(
                silk_SMULWW( psDec->exc_Q14[ i + ( k + psPLC->nb_subfr - 2 ) * psPLC->subfr_length ], prevGain_Q10[ k ] ), 8 ) );
        }
        exc_buf_ptr += psPLC->subfr_length;
    }
    
    silk_sum_sqr_shift( &energy1, &shift1, exc_buf,                         psPLC->subfr_length );
    silk_sum_sqr_shift( &energy2, &shift2, &exc_buf[ psPLC->subfr_length ], psPLC->subfr_length );

    if( silk_RSHIFT( energy1, shift2 ) < silk_RSHIFT( energy2, shift1 ) ) {
        
        rand_ptr = &psDec->exc_Q14[ silk_max_int( 0, ( psPLC->nb_subfr - 1 ) * psPLC->subfr_length - RAND_BUF_SIZE ) ];
    } else {
        
        rand_ptr = &psDec->exc_Q14[ silk_max_int( 0, psPLC->nb_subfr * psPLC->subfr_length - RAND_BUF_SIZE ) ];
    }

    
    B_Q14          = psPLC->LTPCoef_Q14;
    rand_scale_Q14 = psPLC->randScale_Q14;

    
    harm_Gain_Q15 = HARM_ATT_Q15[ silk_min_int( NB_ATT - 1, psDec->lossCnt ) ];
    if( psDec->prevSignalType == TYPE_VOICED ) {
        rand_Gain_Q15 = PLC_RAND_ATTENUATE_V_Q15[  silk_min_int( NB_ATT - 1, psDec->lossCnt ) ];
    } else {
        rand_Gain_Q15 = PLC_RAND_ATTENUATE_UV_Q15[ silk_min_int( NB_ATT - 1, psDec->lossCnt ) ];
    }

    
    silk_bwexpander( psPLC->prevLPC_Q12, psDec->LPC_order, SILK_FIX_CONST( BWE_COEF, 16 ) );

    
    silk_memcpy( A_Q12, psPLC->prevLPC_Q12, psDec->LPC_order * sizeof( opus_int16 ) );

    
    if( psDec->lossCnt == 0 ) {
        rand_scale_Q14 = 1 << 14;

        
        if( psDec->prevSignalType == TYPE_VOICED ) {
            for( i = 0; i < LTP_ORDER; i++ ) {
                rand_scale_Q14 -= B_Q14[ i ];
            }
            rand_scale_Q14 = silk_max_16( 3277, rand_scale_Q14 ); 
            rand_scale_Q14 = (opus_int16)silk_RSHIFT( silk_SMULBB( rand_scale_Q14, psPLC->prevLTP_scale_Q14 ), 14 );
        } else {
            
            opus_int32 invGain_Q30, down_scale_Q30;

            invGain_Q30 = silk_LPC_inverse_pred_gain( psPLC->prevLPC_Q12, psDec->LPC_order );

            down_scale_Q30 = silk_min_32( silk_RSHIFT( 1 << 30, LOG2_INV_LPC_GAIN_HIGH_THRES ), invGain_Q30 );
            down_scale_Q30 = silk_max_32( silk_RSHIFT( 1 << 30, LOG2_INV_LPC_GAIN_LOW_THRES ), down_scale_Q30 );
            down_scale_Q30 = silk_LSHIFT( down_scale_Q30, LOG2_INV_LPC_GAIN_HIGH_THRES );

            rand_Gain_Q15 = silk_RSHIFT( silk_SMULWB( down_scale_Q30, rand_Gain_Q15 ), 14 );
        }
    }

    rand_seed    = psPLC->rand_seed;
    lag          = silk_RSHIFT_ROUND( psPLC->pitchL_Q8, 8 );
    sLTP_buf_idx = psDec->ltp_mem_length;

    
    idx = psDec->ltp_mem_length - lag - psDec->LPC_order - LTP_ORDER / 2;
    silk_assert( idx > 0 );
    silk_LPC_analysis_filter( &sLTP[ idx ], &psDec->outBuf[ idx ], A_Q12, psDec->ltp_mem_length - idx, psDec->LPC_order );
    
    inv_gain_Q30 = silk_INVERSE32_varQ( psPLC->prevGain_Q16[ 1 ], 46 );
    inv_gain_Q30 = silk_min( inv_gain_Q30, silk_int32_MAX >> 1 );
    for( i = idx + psDec->LPC_order; i < psDec->ltp_mem_length; i++ ) {
        sLTP_Q14[ i ] = silk_SMULWB( inv_gain_Q30, sLTP[ i ] );
    }

    
    
    
    for( k = 0; k < psDec->nb_subfr; k++ ) {
        
        pred_lag_ptr = &sLTP_Q14[ sLTP_buf_idx - lag + LTP_ORDER / 2 ];
        for( i = 0; i < psDec->subfr_length; i++ ) {
            
            
            LTP_pred_Q12 = 2;
            LTP_pred_Q12 = silk_SMLAWB( LTP_pred_Q12, pred_lag_ptr[  0 ], B_Q14[ 0 ] );
            LTP_pred_Q12 = silk_SMLAWB( LTP_pred_Q12, pred_lag_ptr[ -1 ], B_Q14[ 1 ] );
            LTP_pred_Q12 = silk_SMLAWB( LTP_pred_Q12, pred_lag_ptr[ -2 ], B_Q14[ 2 ] );
            LTP_pred_Q12 = silk_SMLAWB( LTP_pred_Q12, pred_lag_ptr[ -3 ], B_Q14[ 3 ] );
            LTP_pred_Q12 = silk_SMLAWB( LTP_pred_Q12, pred_lag_ptr[ -4 ], B_Q14[ 4 ] );
            pred_lag_ptr++;

            
            rand_seed = silk_RAND( rand_seed );
            idx = silk_RSHIFT( rand_seed, 25 ) & RAND_BUF_MASK;
            sLTP_Q14[ sLTP_buf_idx ] = silk_LSHIFT32( silk_SMLAWB( LTP_pred_Q12, rand_ptr[ idx ], rand_scale_Q14 ), 2 );
            sLTP_buf_idx++;
        }

        
        for( j = 0; j < LTP_ORDER; j++ ) {
            B_Q14[ j ] = silk_RSHIFT( silk_SMULBB( harm_Gain_Q15, B_Q14[ j ] ), 15 );
        }
        
        rand_scale_Q14 = silk_RSHIFT( silk_SMULBB( rand_scale_Q14, rand_Gain_Q15 ), 15 );

        
        psPLC->pitchL_Q8 = silk_SMLAWB( psPLC->pitchL_Q8, psPLC->pitchL_Q8, PITCH_DRIFT_FAC_Q16 );
        psPLC->pitchL_Q8 = silk_min_32( psPLC->pitchL_Q8, silk_LSHIFT( silk_SMULBB( MAX_PITCH_LAG_MS, psDec->fs_kHz ), 8 ) );
        lag = silk_RSHIFT_ROUND( psPLC->pitchL_Q8, 8 );
    }

    
    
    
    sLPC_Q14_ptr = &sLTP_Q14[ psDec->ltp_mem_length - MAX_LPC_ORDER ];

    
    silk_memcpy( sLPC_Q14_ptr, psDec->sLPC_Q14_buf, MAX_LPC_ORDER * sizeof( opus_int32 ) );

    silk_assert( psDec->LPC_order >= 10 ); 
    for( i = 0; i < psDec->frame_length; i++ ) {
        
        
        LPC_pred_Q10 = silk_RSHIFT( psDec->LPC_order, 1 );
        LPC_pred_Q10 = silk_SMLAWB( LPC_pred_Q10, sLPC_Q14_ptr[ MAX_LPC_ORDER + i -  1 ], A_Q12[ 0 ] );
        LPC_pred_Q10 = silk_SMLAWB( LPC_pred_Q10, sLPC_Q14_ptr[ MAX_LPC_ORDER + i -  2 ], A_Q12[ 1 ] );
        LPC_pred_Q10 = silk_SMLAWB( LPC_pred_Q10, sLPC_Q14_ptr[ MAX_LPC_ORDER + i -  3 ], A_Q12[ 2 ] );
        LPC_pred_Q10 = silk_SMLAWB( LPC_pred_Q10, sLPC_Q14_ptr[ MAX_LPC_ORDER + i -  4 ], A_Q12[ 3 ] );
        LPC_pred_Q10 = silk_SMLAWB( LPC_pred_Q10, sLPC_Q14_ptr[ MAX_LPC_ORDER + i -  5 ], A_Q12[ 4 ] );
        LPC_pred_Q10 = silk_SMLAWB( LPC_pred_Q10, sLPC_Q14_ptr[ MAX_LPC_ORDER + i -  6 ], A_Q12[ 5 ] );
        LPC_pred_Q10 = silk_SMLAWB( LPC_pred_Q10, sLPC_Q14_ptr[ MAX_LPC_ORDER + i -  7 ], A_Q12[ 6 ] );
        LPC_pred_Q10 = silk_SMLAWB( LPC_pred_Q10, sLPC_Q14_ptr[ MAX_LPC_ORDER + i -  8 ], A_Q12[ 7 ] );
        LPC_pred_Q10 = silk_SMLAWB( LPC_pred_Q10, sLPC_Q14_ptr[ MAX_LPC_ORDER + i -  9 ], A_Q12[ 8 ] );
        LPC_pred_Q10 = silk_SMLAWB( LPC_pred_Q10, sLPC_Q14_ptr[ MAX_LPC_ORDER + i - 10 ], A_Q12[ 9 ] );
        for( j = 10; j < psDec->LPC_order; j++ ) {
            LPC_pred_Q10 = silk_SMLAWB( LPC_pred_Q10, sLPC_Q14_ptr[ MAX_LPC_ORDER + i - j - 1 ], A_Q12[ j ] );
        }

        
        sLPC_Q14_ptr[ MAX_LPC_ORDER + i ] = silk_ADD_LSHIFT32( sLPC_Q14_ptr[ MAX_LPC_ORDER + i ], LPC_pred_Q10, 4 );

        
        frame[ i ] = (opus_int16)silk_SAT16( silk_SAT16( silk_RSHIFT_ROUND( silk_SMULWW( sLPC_Q14_ptr[ MAX_LPC_ORDER + i ], prevGain_Q10[ 1 ] ), 8 ) ) );
    }

    
    silk_memcpy( psDec->sLPC_Q14_buf, &sLPC_Q14_ptr[ psDec->frame_length ], MAX_LPC_ORDER * sizeof( opus_int32 ) );

    
    
    
    psPLC->rand_seed     = rand_seed;
    psPLC->randScale_Q14 = rand_scale_Q14;
    for( i = 0; i < MAX_NB_SUBFR; i++ ) {
        psDecCtrl->pitchL[ i ] = lag;
    }
}


void silk_PLC_glue_frames(
    silk_decoder_state                  *psDec,             
    opus_int16                          frame[],            
    opus_int                            length              
)
{
    opus_int   i, energy_shift;
    opus_int32 energy;
    silk_PLC_struct *psPLC;
    psPLC = &psDec->sPLC;

    if( psDec->lossCnt ) {
        
        silk_sum_sqr_shift( &psPLC->conc_energy, &psPLC->conc_energy_shift, frame, length );

        psPLC->last_frame_lost = 1;
    } else {
        if( psDec->sPLC.last_frame_lost ) {
            
            silk_sum_sqr_shift( &energy, &energy_shift, frame, length );

            
            if( energy_shift > psPLC->conc_energy_shift ) {
                psPLC->conc_energy = silk_RSHIFT( psPLC->conc_energy, energy_shift - psPLC->conc_energy_shift );
            } else if( energy_shift < psPLC->conc_energy_shift ) {
                energy = silk_RSHIFT( energy, psPLC->conc_energy_shift - energy_shift );
            }

            
            if( energy > psPLC->conc_energy ) {
                opus_int32 frac_Q24, LZ;
                opus_int32 gain_Q16, slope_Q16;

                LZ = silk_CLZ32( psPLC->conc_energy );
                LZ = LZ - 1;
                psPLC->conc_energy = silk_LSHIFT( psPLC->conc_energy, LZ );
                energy = silk_RSHIFT( energy, silk_max_32( 24 - LZ, 0 ) );

                frac_Q24 = silk_DIV32( psPLC->conc_energy, silk_max( energy, 1 ) );

                gain_Q16 = silk_LSHIFT( silk_SQRT_APPROX( frac_Q24 ), 4 );
                slope_Q16 = silk_DIV32_16( ( 1 << 16 ) - gain_Q16, length );
                
                slope_Q16 = silk_LSHIFT( slope_Q16, 2 );

                for( i = 0; i < length; i++ ) {
                    frame[ i ] = silk_SMULWB( gain_Q16, frame[ i ] );
                    gain_Q16 += slope_Q16;
                    if( gain_Q16 > 1 << 16 ) {
                        break;
                    }
                }
            }
        }
        psPLC->last_frame_lost = 0;
    }
}
