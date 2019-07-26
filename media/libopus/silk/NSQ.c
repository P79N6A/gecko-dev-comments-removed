






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main.h"

static inline void silk_nsq_scale_states(
    const silk_encoder_state *psEncC,           
    silk_nsq_state      *NSQ,                   
    const opus_int32    x_Q3[],                 
    opus_int32          x_sc_Q10[],             
    const opus_int16    sLTP[],                 
    opus_int32          sLTP_Q15[],             
    opus_int            subfr,                  
    const opus_int      LTP_scale_Q14,          
    const opus_int32    Gains_Q16[ MAX_NB_SUBFR ], 
    const opus_int      pitchL[ MAX_NB_SUBFR ], 
    const opus_int      signal_type             
);

static inline void silk_noise_shape_quantizer(
    silk_nsq_state      *NSQ,                   
    opus_int            signalType,             
    const opus_int32    x_sc_Q10[],             
    opus_int8           pulses[],               
    opus_int16          xq[],                   
    opus_int32          sLTP_Q15[],             
    const opus_int16    a_Q12[],                
    const opus_int16    b_Q14[],                
    const opus_int16    AR_shp_Q13[],           
    opus_int            lag,                    
    opus_int32          HarmShapeFIRPacked_Q14, 
    opus_int            Tilt_Q14,               
    opus_int32          LF_shp_Q14,             
    opus_int32          Gain_Q16,               
    opus_int            Lambda_Q10,             
    opus_int            offset_Q10,             
    opus_int            length,                 
    opus_int            shapingLPCOrder,        
    opus_int            predictLPCOrder         
);

void silk_NSQ(
    const silk_encoder_state    *psEncC,                                    
    silk_nsq_state              *NSQ,                                       
    SideInfoIndices             *psIndices,                                 
    const opus_int32            x_Q3[],                                     
    opus_int8                   pulses[],                                   
    const opus_int16            PredCoef_Q12[ 2 * MAX_LPC_ORDER ],          
    const opus_int16            LTPCoef_Q14[ LTP_ORDER * MAX_NB_SUBFR ],    
    const opus_int16            AR2_Q13[ MAX_NB_SUBFR * MAX_SHAPE_LPC_ORDER ], 
    const opus_int              HarmShapeGain_Q14[ MAX_NB_SUBFR ],          
    const opus_int              Tilt_Q14[ MAX_NB_SUBFR ],                   
    const opus_int32            LF_shp_Q14[ MAX_NB_SUBFR ],                 
    const opus_int32            Gains_Q16[ MAX_NB_SUBFR ],                  
    const opus_int              pitchL[ MAX_NB_SUBFR ],                     
    const opus_int              Lambda_Q10,                                 
    const opus_int              LTP_scale_Q14                               
)
{
    opus_int            k, lag, start_idx, LSF_interpolation_flag;
    const opus_int16    *A_Q12, *B_Q14, *AR_shp_Q13;
    opus_int16          *pxq;
    opus_int32          sLTP_Q15[ 2 * MAX_FRAME_LENGTH ];
    opus_int16          sLTP[     2 * MAX_FRAME_LENGTH ];
    opus_int32          HarmShapeFIRPacked_Q14;
    opus_int            offset_Q10;
    opus_int32          x_sc_Q10[ MAX_SUB_FRAME_LENGTH ];

    NSQ->rand_seed = psIndices->Seed;

    
    lag = NSQ->lagPrev;

    silk_assert( NSQ->prev_gain_Q16 != 0 );

    offset_Q10 = silk_Quantization_Offsets_Q10[ psIndices->signalType >> 1 ][ psIndices->quantOffsetType ];

    if( psIndices->NLSFInterpCoef_Q2 == 4 ) {
        LSF_interpolation_flag = 0;
    } else {
        LSF_interpolation_flag = 1;
    }

    
    NSQ->sLTP_shp_buf_idx = psEncC->ltp_mem_length;
    NSQ->sLTP_buf_idx     = psEncC->ltp_mem_length;
    pxq                   = &NSQ->xq[ psEncC->ltp_mem_length ];
    for( k = 0; k < psEncC->nb_subfr; k++ ) {
        A_Q12      = &PredCoef_Q12[ (( k >> 1 ) | ( 1 - LSF_interpolation_flag )) * MAX_LPC_ORDER ];
        B_Q14      = &LTPCoef_Q14[ k * LTP_ORDER ];
        AR_shp_Q13 = &AR2_Q13[     k * MAX_SHAPE_LPC_ORDER ];

        
        silk_assert( HarmShapeGain_Q14[ k ] >= 0 );
        HarmShapeFIRPacked_Q14  =                          silk_RSHIFT( HarmShapeGain_Q14[ k ], 2 );
        HarmShapeFIRPacked_Q14 |= silk_LSHIFT( (opus_int32)silk_RSHIFT( HarmShapeGain_Q14[ k ], 1 ), 16 );

        NSQ->rewhite_flag = 0;
        if( psIndices->signalType == TYPE_VOICED ) {
            
            lag = pitchL[ k ];

            
            if( ( k & ( 3 - silk_LSHIFT( LSF_interpolation_flag, 1 ) ) ) == 0 ) {
                
                start_idx = psEncC->ltp_mem_length - lag - psEncC->predictLPCOrder - LTP_ORDER / 2;
                silk_assert( start_idx > 0 );

                silk_LPC_analysis_filter( &sLTP[ start_idx ], &NSQ->xq[ start_idx + k * psEncC->subfr_length ],
                    A_Q12, psEncC->ltp_mem_length - start_idx, psEncC->predictLPCOrder );

                NSQ->rewhite_flag = 1;
                NSQ->sLTP_buf_idx = psEncC->ltp_mem_length;
            }
        }

        silk_nsq_scale_states( psEncC, NSQ, x_Q3, x_sc_Q10, sLTP, sLTP_Q15, k, LTP_scale_Q14, Gains_Q16, pitchL, psIndices->signalType );

        silk_noise_shape_quantizer( NSQ, psIndices->signalType, x_sc_Q10, pulses, pxq, sLTP_Q15, A_Q12, B_Q14,
            AR_shp_Q13, lag, HarmShapeFIRPacked_Q14, Tilt_Q14[ k ], LF_shp_Q14[ k ], Gains_Q16[ k ], Lambda_Q10,
            offset_Q10, psEncC->subfr_length, psEncC->shapingLPCOrder, psEncC->predictLPCOrder );

        x_Q3   += psEncC->subfr_length;
        pulses += psEncC->subfr_length;
        pxq    += psEncC->subfr_length;
    }

    
    NSQ->lagPrev = pitchL[ psEncC->nb_subfr - 1 ];

    
    
    silk_memmove( NSQ->xq,           &NSQ->xq[           psEncC->frame_length ], psEncC->ltp_mem_length * sizeof( opus_int16 ) );
    silk_memmove( NSQ->sLTP_shp_Q14, &NSQ->sLTP_shp_Q14[ psEncC->frame_length ], psEncC->ltp_mem_length * sizeof( opus_int32 ) );
}




static inline void silk_noise_shape_quantizer(
    silk_nsq_state      *NSQ,                   
    opus_int            signalType,             
    const opus_int32    x_sc_Q10[],             
    opus_int8           pulses[],               
    opus_int16          xq[],                   
    opus_int32          sLTP_Q15[],             
    const opus_int16    a_Q12[],                
    const opus_int16    b_Q14[],                
    const opus_int16    AR_shp_Q13[],           
    opus_int            lag,                    
    opus_int32          HarmShapeFIRPacked_Q14, 
    opus_int            Tilt_Q14,               
    opus_int32          LF_shp_Q14,             
    opus_int32          Gain_Q16,               
    opus_int            Lambda_Q10,             
    opus_int            offset_Q10,             
    opus_int            length,                 
    opus_int            shapingLPCOrder,        
    opus_int            predictLPCOrder         
)
{
    opus_int     i, j;
    opus_int32   LTP_pred_Q13, LPC_pred_Q10, n_AR_Q12, n_LTP_Q13;
    opus_int32   n_LF_Q12, r_Q10, rr_Q10, q1_Q0, q1_Q10, q2_Q10, rd1_Q20, rd2_Q20;
    opus_int32   exc_Q14, LPC_exc_Q14, xq_Q14, Gain_Q10;
    opus_int32   tmp1, tmp2, sLF_AR_shp_Q14;
    opus_int32   *psLPC_Q14, *shp_lag_ptr, *pred_lag_ptr;

    shp_lag_ptr  = &NSQ->sLTP_shp_Q14[ NSQ->sLTP_shp_buf_idx - lag + HARM_SHAPE_FIR_TAPS / 2 ];
    pred_lag_ptr = &sLTP_Q15[ NSQ->sLTP_buf_idx - lag + LTP_ORDER / 2 ];
    Gain_Q10     = silk_RSHIFT( Gain_Q16, 6 );

    
    psLPC_Q14 = &NSQ->sLPC_Q14[ NSQ_LPC_BUF_LENGTH - 1 ];

    for( i = 0; i < length; i++ ) {
        
        NSQ->rand_seed = silk_RAND( NSQ->rand_seed );

        
        silk_assert( predictLPCOrder == 10 || predictLPCOrder == 16 );
        
        LPC_pred_Q10 = silk_RSHIFT( predictLPCOrder, 1 );
        LPC_pred_Q10 = silk_SMLAWB( LPC_pred_Q10, psLPC_Q14[  0 ], a_Q12[ 0 ] );
        LPC_pred_Q10 = silk_SMLAWB( LPC_pred_Q10, psLPC_Q14[ -1 ], a_Q12[ 1 ] );
        LPC_pred_Q10 = silk_SMLAWB( LPC_pred_Q10, psLPC_Q14[ -2 ], a_Q12[ 2 ] );
        LPC_pred_Q10 = silk_SMLAWB( LPC_pred_Q10, psLPC_Q14[ -3 ], a_Q12[ 3 ] );
        LPC_pred_Q10 = silk_SMLAWB( LPC_pred_Q10, psLPC_Q14[ -4 ], a_Q12[ 4 ] );
        LPC_pred_Q10 = silk_SMLAWB( LPC_pred_Q10, psLPC_Q14[ -5 ], a_Q12[ 5 ] );
        LPC_pred_Q10 = silk_SMLAWB( LPC_pred_Q10, psLPC_Q14[ -6 ], a_Q12[ 6 ] );
        LPC_pred_Q10 = silk_SMLAWB( LPC_pred_Q10, psLPC_Q14[ -7 ], a_Q12[ 7 ] );
        LPC_pred_Q10 = silk_SMLAWB( LPC_pred_Q10, psLPC_Q14[ -8 ], a_Q12[ 8 ] );
        LPC_pred_Q10 = silk_SMLAWB( LPC_pred_Q10, psLPC_Q14[ -9 ], a_Q12[ 9 ] );
        if( predictLPCOrder == 16 ) {
            LPC_pred_Q10 = silk_SMLAWB( LPC_pred_Q10, psLPC_Q14[ -10 ], a_Q12[ 10 ] );
            LPC_pred_Q10 = silk_SMLAWB( LPC_pred_Q10, psLPC_Q14[ -11 ], a_Q12[ 11 ] );
            LPC_pred_Q10 = silk_SMLAWB( LPC_pred_Q10, psLPC_Q14[ -12 ], a_Q12[ 12 ] );
            LPC_pred_Q10 = silk_SMLAWB( LPC_pred_Q10, psLPC_Q14[ -13 ], a_Q12[ 13 ] );
            LPC_pred_Q10 = silk_SMLAWB( LPC_pred_Q10, psLPC_Q14[ -14 ], a_Q12[ 14 ] );
            LPC_pred_Q10 = silk_SMLAWB( LPC_pred_Q10, psLPC_Q14[ -15 ], a_Q12[ 15 ] );
        }

        
        if( signalType == TYPE_VOICED ) {
            
            
            LTP_pred_Q13 = 2;
            LTP_pred_Q13 = silk_SMLAWB( LTP_pred_Q13, pred_lag_ptr[  0 ], b_Q14[ 0 ] );
            LTP_pred_Q13 = silk_SMLAWB( LTP_pred_Q13, pred_lag_ptr[ -1 ], b_Q14[ 1 ] );
            LTP_pred_Q13 = silk_SMLAWB( LTP_pred_Q13, pred_lag_ptr[ -2 ], b_Q14[ 2 ] );
            LTP_pred_Q13 = silk_SMLAWB( LTP_pred_Q13, pred_lag_ptr[ -3 ], b_Q14[ 3 ] );
            LTP_pred_Q13 = silk_SMLAWB( LTP_pred_Q13, pred_lag_ptr[ -4 ], b_Q14[ 4 ] );
            pred_lag_ptr++;
        } else {
            LTP_pred_Q13 = 0;
        }

        
        silk_assert( ( shapingLPCOrder & 1 ) == 0 );   
        tmp2 = psLPC_Q14[ 0 ];
        tmp1 = NSQ->sAR2_Q14[ 0 ];
        NSQ->sAR2_Q14[ 0 ] = tmp2;
        n_AR_Q12 = silk_RSHIFT( shapingLPCOrder, 1 );
        n_AR_Q12 = silk_SMLAWB( n_AR_Q12, tmp2, AR_shp_Q13[ 0 ] );
        for( j = 2; j < shapingLPCOrder; j += 2 ) {
            tmp2 = NSQ->sAR2_Q14[ j - 1 ];
            NSQ->sAR2_Q14[ j - 1 ] = tmp1;
            n_AR_Q12 = silk_SMLAWB( n_AR_Q12, tmp1, AR_shp_Q13[ j - 1 ] );
            tmp1 = NSQ->sAR2_Q14[ j + 0 ];
            NSQ->sAR2_Q14[ j + 0 ] = tmp2;
            n_AR_Q12 = silk_SMLAWB( n_AR_Q12, tmp2, AR_shp_Q13[ j ] );
        }
        NSQ->sAR2_Q14[ shapingLPCOrder - 1 ] = tmp1;
        n_AR_Q12 = silk_SMLAWB( n_AR_Q12, tmp1, AR_shp_Q13[ shapingLPCOrder - 1 ] );

        n_AR_Q12 = silk_LSHIFT32( n_AR_Q12, 1 );                                
        n_AR_Q12 = silk_SMLAWB( n_AR_Q12, NSQ->sLF_AR_shp_Q14, Tilt_Q14 );

        n_LF_Q12 = silk_SMULWB( NSQ->sLTP_shp_Q14[ NSQ->sLTP_shp_buf_idx - 1 ], LF_shp_Q14 );
        n_LF_Q12 = silk_SMLAWT( n_LF_Q12, NSQ->sLF_AR_shp_Q14, LF_shp_Q14 );

        silk_assert( lag > 0 || signalType != TYPE_VOICED );

        
        tmp1 = silk_SUB32( silk_LSHIFT32( LPC_pred_Q10, 2 ), n_AR_Q12 );        
        tmp1 = silk_SUB32( tmp1, n_LF_Q12 );                                    
        if( lag > 0 ) {
            
            n_LTP_Q13 = silk_SMULWB( silk_ADD32( shp_lag_ptr[ 0 ], shp_lag_ptr[ -2 ] ), HarmShapeFIRPacked_Q14 );
            n_LTP_Q13 = silk_SMLAWT( n_LTP_Q13, shp_lag_ptr[ -1 ],                      HarmShapeFIRPacked_Q14 );
            n_LTP_Q13 = silk_LSHIFT( n_LTP_Q13, 1 );
            shp_lag_ptr++;

            tmp2 = silk_SUB32( LTP_pred_Q13, n_LTP_Q13 );                       
            tmp1 = silk_ADD_LSHIFT32( tmp2, tmp1, 1 );                          
            tmp1 = silk_RSHIFT_ROUND( tmp1, 3 );                                
        } else {
            tmp1 = silk_RSHIFT_ROUND( tmp1, 2 );                                
        }

        r_Q10 = silk_SUB32( x_sc_Q10[ i ], tmp1 );                              

        
        if ( NSQ->rand_seed < 0 ) {
           r_Q10 = -r_Q10;
        }
        r_Q10 = silk_LIMIT_32( r_Q10, -(31 << 10), 30 << 10 );

        
        q1_Q10 = silk_SUB32( r_Q10, offset_Q10 );
        q1_Q0 = silk_RSHIFT( q1_Q10, 10 );
        if( q1_Q0 > 0 ) {
            q1_Q10  = silk_SUB32( silk_LSHIFT( q1_Q0, 10 ), QUANT_LEVEL_ADJUST_Q10 );
            q1_Q10  = silk_ADD32( q1_Q10, offset_Q10 );
            q2_Q10  = silk_ADD32( q1_Q10, 1024 );
            rd1_Q20 = silk_SMULBB( q1_Q10, Lambda_Q10 );
            rd2_Q20 = silk_SMULBB( q2_Q10, Lambda_Q10 );
        } else if( q1_Q0 == 0 ) {
            q1_Q10  = offset_Q10;
            q2_Q10  = silk_ADD32( q1_Q10, 1024 - QUANT_LEVEL_ADJUST_Q10 );
            rd1_Q20 = silk_SMULBB( q1_Q10, Lambda_Q10 );
            rd2_Q20 = silk_SMULBB( q2_Q10, Lambda_Q10 );
        } else if( q1_Q0 == -1 ) {
            q2_Q10  = offset_Q10;
            q1_Q10  = silk_SUB32( q2_Q10, 1024 - QUANT_LEVEL_ADJUST_Q10 );
            rd1_Q20 = silk_SMULBB( -q1_Q10, Lambda_Q10 );
            rd2_Q20 = silk_SMULBB(  q2_Q10, Lambda_Q10 );
        } else {            
            q1_Q10  = silk_ADD32( silk_LSHIFT( q1_Q0, 10 ), QUANT_LEVEL_ADJUST_Q10 );
            q1_Q10  = silk_ADD32( q1_Q10, offset_Q10 );
            q2_Q10  = silk_ADD32( q1_Q10, 1024 );
            rd1_Q20 = silk_SMULBB( -q1_Q10, Lambda_Q10 );
            rd2_Q20 = silk_SMULBB( -q2_Q10, Lambda_Q10 );
        }
        rr_Q10  = silk_SUB32( r_Q10, q1_Q10 );
        rd1_Q20 = silk_SMLABB( rd1_Q20, rr_Q10, rr_Q10 );
        rr_Q10  = silk_SUB32( r_Q10, q2_Q10 );
        rd2_Q20 = silk_SMLABB( rd2_Q20, rr_Q10, rr_Q10 );

        if( rd2_Q20 < rd1_Q20 ) {
            q1_Q10 = q2_Q10;
        }

        pulses[ i ] = (opus_int8)silk_RSHIFT_ROUND( q1_Q10, 10 );

        
        exc_Q14 = silk_LSHIFT( q1_Q10, 4 );
        if ( NSQ->rand_seed < 0 ) {
           exc_Q14 = -exc_Q14;
        }

        
        LPC_exc_Q14 = silk_ADD_LSHIFT32( exc_Q14, LTP_pred_Q13, 1 );
        xq_Q14      = silk_ADD_LSHIFT32( LPC_exc_Q14, LPC_pred_Q10, 4 );

        
        xq[ i ] = (opus_int16)silk_SAT16( silk_RSHIFT_ROUND( silk_SMULWW( xq_Q14, Gain_Q10 ), 8 ) );

        
        psLPC_Q14++;
        *psLPC_Q14 = xq_Q14;
        sLF_AR_shp_Q14 = silk_SUB_LSHIFT32( xq_Q14, n_AR_Q12, 2 );
        NSQ->sLF_AR_shp_Q14 = sLF_AR_shp_Q14;

        NSQ->sLTP_shp_Q14[ NSQ->sLTP_shp_buf_idx ] = silk_SUB_LSHIFT32( sLF_AR_shp_Q14, n_LF_Q12, 2 );
        sLTP_Q15[ NSQ->sLTP_buf_idx ] = silk_LSHIFT( LPC_exc_Q14, 1 );
        NSQ->sLTP_shp_buf_idx++;
        NSQ->sLTP_buf_idx++;

        
        NSQ->rand_seed = silk_ADD32_ovflw( NSQ->rand_seed, pulses[ i ] );
    }

    
    silk_memcpy( NSQ->sLPC_Q14, &NSQ->sLPC_Q14[ length ], NSQ_LPC_BUF_LENGTH * sizeof( opus_int32 ) );
}

static inline void silk_nsq_scale_states(
    const silk_encoder_state *psEncC,           
    silk_nsq_state      *NSQ,                   
    const opus_int32    x_Q3[],                 
    opus_int32          x_sc_Q10[],             
    const opus_int16    sLTP[],                 
    opus_int32          sLTP_Q15[],             
    opus_int            subfr,                  
    const opus_int      LTP_scale_Q14,          
    const opus_int32    Gains_Q16[ MAX_NB_SUBFR ], 
    const opus_int      pitchL[ MAX_NB_SUBFR ], 
    const opus_int      signal_type             
)
{
    opus_int   i, lag;
    opus_int32 gain_adj_Q16, inv_gain_Q31, inv_gain_Q23;

    lag          = pitchL[ subfr ];
    inv_gain_Q31 = silk_INVERSE32_varQ( silk_max( Gains_Q16[ subfr ], 1 ), 47 );
    silk_assert( inv_gain_Q31 != 0 );

    
    if( Gains_Q16[ subfr ] != NSQ->prev_gain_Q16 ) {
        gain_adj_Q16 =  silk_DIV32_varQ( NSQ->prev_gain_Q16, Gains_Q16[ subfr ], 16 );
    } else {
        gain_adj_Q16 = 1 << 16;
    }

    
    inv_gain_Q23 = silk_RSHIFT_ROUND( inv_gain_Q31, 8 );
    for( i = 0; i < psEncC->subfr_length; i++ ) {
        x_sc_Q10[ i ] = silk_SMULWW( x_Q3[ i ], inv_gain_Q23 );
    }

    
    NSQ->prev_gain_Q16 = Gains_Q16[ subfr ];

    
    if( NSQ->rewhite_flag ) {
        if( subfr == 0 ) {
            
            inv_gain_Q31 = silk_LSHIFT( silk_SMULWB( inv_gain_Q31, LTP_scale_Q14 ), 2 );
        }
        for( i = NSQ->sLTP_buf_idx - lag - LTP_ORDER / 2; i < NSQ->sLTP_buf_idx; i++ ) {
            silk_assert( i < MAX_FRAME_LENGTH );
            sLTP_Q15[ i ] = silk_SMULWB( inv_gain_Q31, sLTP[ i ] );
        }
    }

    
    if( gain_adj_Q16 != 1 << 16 ) {
        
        for( i = NSQ->sLTP_shp_buf_idx - psEncC->ltp_mem_length; i < NSQ->sLTP_shp_buf_idx; i++ ) {
            NSQ->sLTP_shp_Q14[ i ] = silk_SMULWW( gain_adj_Q16, NSQ->sLTP_shp_Q14[ i ] );
        }

        
        if( signal_type == TYPE_VOICED && NSQ->rewhite_flag == 0 ) {
            for( i = NSQ->sLTP_buf_idx - lag - LTP_ORDER / 2; i < NSQ->sLTP_buf_idx; i++ ) {
                sLTP_Q15[ i ] = silk_SMULWW( gain_adj_Q16, sLTP_Q15[ i ] );
            }
        }

        NSQ->sLF_AR_shp_Q14 = silk_SMULWW( gain_adj_Q16, NSQ->sLF_AR_shp_Q14 );

        
        for( i = 0; i < NSQ_LPC_BUF_LENGTH; i++ ) {
            NSQ->sLPC_Q14[ i ] = silk_SMULWW( gain_adj_Q16, NSQ->sLPC_Q14[ i ] );
        }
        for( i = 0; i < MAX_SHAPE_LPC_ORDER; i++ ) {
            NSQ->sAR2_Q14[ i ] = silk_SMULWW( gain_adj_Q16, NSQ->sAR2_Q14[ i ] );
        }
    }
}
