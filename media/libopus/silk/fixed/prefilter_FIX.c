






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main_FIX.h"
#include "tuning_parameters.h"


static inline void silk_prefilt_FIX(
    silk_prefilter_state_FIX    *P,                         
    opus_int32                  st_res_Q12[],               
    opus_int32                  xw_Q3[],                    
    opus_int32                  HarmShapeFIRPacked_Q12,     
    opus_int                    Tilt_Q14,                   
    opus_int32                  LF_shp_Q14,                 
    opus_int                    lag,                        
    opus_int                    length                      
);

void silk_warped_LPC_analysis_filter_FIX(
          opus_int32            state[],                    
          opus_int32            res_Q2[],                   
    const opus_int16            coef_Q13[],                 
    const opus_int16            input[],                    
    const opus_int16            lambda_Q16,                 
    const opus_int              length,                     
    const opus_int              order                       
)
{
    opus_int     n, i;
    opus_int32   acc_Q11, tmp1, tmp2;

    
    silk_assert( ( order & 1 ) == 0 );

    for( n = 0; n < length; n++ ) {
        
        tmp2 = silk_SMLAWB( state[ 0 ], state[ 1 ], lambda_Q16 );
        state[ 0 ] = silk_LSHIFT( input[ n ], 14 );
        
        tmp1 = silk_SMLAWB( state[ 1 ], state[ 2 ] - tmp2, lambda_Q16 );
        state[ 1 ] = tmp2;
        acc_Q11 = silk_RSHIFT( order, 1 );
        acc_Q11 = silk_SMLAWB( acc_Q11, tmp2, coef_Q13[ 0 ] );
        
        for( i = 2; i < order; i += 2 ) {
            
            tmp2 = silk_SMLAWB( state[ i ], state[ i + 1 ] - tmp1, lambda_Q16 );
            state[ i ] = tmp1;
            acc_Q11 = silk_SMLAWB( acc_Q11, tmp1, coef_Q13[ i - 1 ] );
            
            tmp1 = silk_SMLAWB( state[ i + 1 ], state[ i + 2 ] - tmp2, lambda_Q16 );
            state[ i + 1 ] = tmp2;
            acc_Q11 = silk_SMLAWB( acc_Q11, tmp2, coef_Q13[ i ] );
        }
        state[ order ] = tmp1;
        acc_Q11 = silk_SMLAWB( acc_Q11, tmp1, coef_Q13[ order - 1 ] );
        res_Q2[ n ] = silk_LSHIFT( (opus_int32)input[ n ], 2 ) - silk_RSHIFT_ROUND( acc_Q11, 9 );
    }
}

void silk_prefilter_FIX(
    silk_encoder_state_FIX          *psEnc,                                 
    const silk_encoder_control_FIX  *psEncCtrl,                             
    opus_int32                      xw_Q3[],                                
    const opus_int16                x[]                                     
)
{
    silk_prefilter_state_FIX *P = &psEnc->sPrefilt;
    opus_int   j, k, lag;
    opus_int32 tmp_32;
    const opus_int16 *AR1_shp_Q13;
    const opus_int16 *px;
    opus_int32 *pxw_Q3;
    opus_int   HarmShapeGain_Q12, Tilt_Q14;
    opus_int32 HarmShapeFIRPacked_Q12, LF_shp_Q14;
    opus_int32 x_filt_Q12[ MAX_SUB_FRAME_LENGTH ];
    opus_int32 st_res_Q2[ MAX_SUB_FRAME_LENGTH + MAX_LPC_ORDER ];
    opus_int16 B_Q10[ 2 ];

    
    px  = x;
    pxw_Q3 = xw_Q3;
    lag = P->lagPrev;
    for( k = 0; k < psEnc->sCmn.nb_subfr; k++ ) {
        
        if( psEnc->sCmn.indices.signalType == TYPE_VOICED ) {
            lag = psEncCtrl->pitchL[ k ];
        }

        
        HarmShapeGain_Q12 = silk_SMULWB( psEncCtrl->HarmShapeGain_Q14[ k ], 16384 - psEncCtrl->HarmBoost_Q14[ k ] );
        silk_assert( HarmShapeGain_Q12 >= 0 );
        HarmShapeFIRPacked_Q12  =                          silk_RSHIFT( HarmShapeGain_Q12, 2 );
        HarmShapeFIRPacked_Q12 |= silk_LSHIFT( (opus_int32)silk_RSHIFT( HarmShapeGain_Q12, 1 ), 16 );
        Tilt_Q14    = psEncCtrl->Tilt_Q14[   k ];
        LF_shp_Q14  = psEncCtrl->LF_shp_Q14[ k ];
        AR1_shp_Q13 = &psEncCtrl->AR1_Q13[   k * MAX_SHAPE_LPC_ORDER ];

        
        silk_warped_LPC_analysis_filter_FIX( P->sAR_shp, st_res_Q2, AR1_shp_Q13, px,
            psEnc->sCmn.warping_Q16, psEnc->sCmn.subfr_length, psEnc->sCmn.shapingLPCOrder );

        
        B_Q10[ 0 ] = silk_RSHIFT_ROUND( psEncCtrl->GainsPre_Q14[ k ], 4 );
        tmp_32 = silk_SMLABB( SILK_FIX_CONST( INPUT_TILT, 26 ), psEncCtrl->HarmBoost_Q14[ k ], HarmShapeGain_Q12 );   
        tmp_32 = silk_SMLABB( tmp_32, psEncCtrl->coding_quality_Q14, SILK_FIX_CONST( HIGH_RATE_INPUT_TILT, 12 ) );    
        tmp_32 = silk_SMULWB( tmp_32, -psEncCtrl->GainsPre_Q14[ k ] );                                                
        tmp_32 = silk_RSHIFT_ROUND( tmp_32, 14 );                                                                     
        B_Q10[ 1 ]= silk_SAT16( tmp_32 );
        x_filt_Q12[ 0 ] = silk_SMLABB( silk_SMULBB( st_res_Q2[ 0 ], B_Q10[ 0 ] ), P->sHarmHP_Q2, B_Q10[ 1 ] );
        for( j = 1; j < psEnc->sCmn.subfr_length; j++ ) {
            x_filt_Q12[ j ] = silk_SMLABB( silk_SMULBB( st_res_Q2[ j ], B_Q10[ 0 ] ), st_res_Q2[ j - 1 ], B_Q10[ 1 ] );
        }
        P->sHarmHP_Q2 = st_res_Q2[ psEnc->sCmn.subfr_length - 1 ];

        silk_prefilt_FIX( P, x_filt_Q12, pxw_Q3, HarmShapeFIRPacked_Q12, Tilt_Q14, LF_shp_Q14, lag, psEnc->sCmn.subfr_length );

        px  += psEnc->sCmn.subfr_length;
        pxw_Q3 += psEnc->sCmn.subfr_length;
    }

    P->lagPrev = psEncCtrl->pitchL[ psEnc->sCmn.nb_subfr - 1 ];
}


static inline void silk_prefilt_FIX(
    silk_prefilter_state_FIX    *P,                         
    opus_int32                  st_res_Q12[],               
    opus_int32                  xw_Q3[],                    
    opus_int32                  HarmShapeFIRPacked_Q12,     
    opus_int                    Tilt_Q14,                   
    opus_int32                  LF_shp_Q14,                 
    opus_int                    lag,                        
    opus_int                    length                      
)
{
    opus_int   i, idx, LTP_shp_buf_idx;
    opus_int32 n_LTP_Q12, n_Tilt_Q10, n_LF_Q10;
    opus_int32 sLF_MA_shp_Q12, sLF_AR_shp_Q12;
    opus_int16 *LTP_shp_buf;

    
    LTP_shp_buf     = P->sLTP_shp;
    LTP_shp_buf_idx = P->sLTP_shp_buf_idx;
    sLF_AR_shp_Q12  = P->sLF_AR_shp_Q12;
    sLF_MA_shp_Q12  = P->sLF_MA_shp_Q12;

    for( i = 0; i < length; i++ ) {
        if( lag > 0 ) {
            
            silk_assert( HARM_SHAPE_FIR_TAPS == 3 );
            idx = lag + LTP_shp_buf_idx;
            n_LTP_Q12 = silk_SMULBB(            LTP_shp_buf[ ( idx - HARM_SHAPE_FIR_TAPS / 2 - 1) & LTP_MASK ], HarmShapeFIRPacked_Q12 );
            n_LTP_Q12 = silk_SMLABT( n_LTP_Q12, LTP_shp_buf[ ( idx - HARM_SHAPE_FIR_TAPS / 2    ) & LTP_MASK ], HarmShapeFIRPacked_Q12 );
            n_LTP_Q12 = silk_SMLABB( n_LTP_Q12, LTP_shp_buf[ ( idx - HARM_SHAPE_FIR_TAPS / 2 + 1) & LTP_MASK ], HarmShapeFIRPacked_Q12 );
        } else {
            n_LTP_Q12 = 0;
        }

        n_Tilt_Q10 = silk_SMULWB( sLF_AR_shp_Q12, Tilt_Q14 );
        n_LF_Q10   = silk_SMLAWB( silk_SMULWT( sLF_AR_shp_Q12, LF_shp_Q14 ), sLF_MA_shp_Q12, LF_shp_Q14 );

        sLF_AR_shp_Q12 = silk_SUB32( st_res_Q12[ i ], silk_LSHIFT( n_Tilt_Q10, 2 ) );
        sLF_MA_shp_Q12 = silk_SUB32( sLF_AR_shp_Q12,  silk_LSHIFT( n_LF_Q10,   2 ) );

        LTP_shp_buf_idx = ( LTP_shp_buf_idx - 1 ) & LTP_MASK;
        LTP_shp_buf[ LTP_shp_buf_idx ] = (opus_int16)silk_SAT16( silk_RSHIFT_ROUND( sLF_MA_shp_Q12, 12 ) );

        xw_Q3[i] = silk_RSHIFT_ROUND( silk_SUB32( sLF_MA_shp_Q12, n_LTP_Q12 ), 9 );
    }

    
    P->sLF_AR_shp_Q12   = sLF_AR_shp_Q12;
    P->sLF_MA_shp_Q12   = sLF_MA_shp_Q12;
    P->sLTP_shp_buf_idx = LTP_shp_buf_idx;
}
