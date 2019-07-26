






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main.h"


void silk_stereo_LR_to_MS(
    stereo_enc_state            *state,                         
    opus_int16                  x1[],                           
    opus_int16                  x2[],                           
    opus_int8                   ix[ 2 ][ 3 ],                   
    opus_int8                   *mid_only_flag,                 
    opus_int32                  mid_side_rates_bps[],           
    opus_int32                  total_rate_bps,                 
    opus_int                    prev_speech_act_Q8,             
    opus_int                    toMono,                         
    opus_int                    fs_kHz,                         
    opus_int                    frame_length                    
)
{
    opus_int   n, is10msFrame, denom_Q16, delta0_Q13, delta1_Q13;
    opus_int32 sum, diff, smooth_coef_Q16, pred_Q13[ 2 ], pred0_Q13, pred1_Q13;
    opus_int32 LP_ratio_Q14, HP_ratio_Q14, frac_Q16, frac_3_Q16, min_mid_rate_bps, width_Q14, w_Q24, deltaw_Q24;
    opus_int16 side[ MAX_FRAME_LENGTH + 2 ];
    opus_int16 LP_mid[  MAX_FRAME_LENGTH ], HP_mid[  MAX_FRAME_LENGTH ];
    opus_int16 LP_side[ MAX_FRAME_LENGTH ], HP_side[ MAX_FRAME_LENGTH ];
    opus_int16 *mid = &x1[ -2 ];

    
    for( n = 0; n < frame_length + 2; n++ ) {
        sum  = x1[ n - 2 ] + (opus_int32)x2[ n - 2 ];
        diff = x1[ n - 2 ] - (opus_int32)x2[ n - 2 ];
        mid[  n ] = (opus_int16)silk_RSHIFT_ROUND( sum, 1 );
        side[ n ] = (opus_int16)silk_SAT16( silk_RSHIFT_ROUND( diff, 1 ) );
    }

    
    silk_memcpy( mid,  state->sMid,  2 * sizeof( opus_int16 ) );
    silk_memcpy( side, state->sSide, 2 * sizeof( opus_int16 ) );
    silk_memcpy( state->sMid,  &mid[  frame_length ], 2 * sizeof( opus_int16 ) );
    silk_memcpy( state->sSide, &side[ frame_length ], 2 * sizeof( opus_int16 ) );

    
    for( n = 0; n < frame_length; n++ ) {
        sum = silk_RSHIFT_ROUND( silk_ADD_LSHIFT( mid[ n ] + mid[ n + 2 ], mid[ n + 1 ], 1 ), 2 );
        LP_mid[ n ] = sum;
        HP_mid[ n ] = mid[ n + 1 ] - sum;
    }

    
    for( n = 0; n < frame_length; n++ ) {
        sum = silk_RSHIFT_ROUND( silk_ADD_LSHIFT( side[ n ] + side[ n + 2 ], side[ n + 1 ], 1 ), 2 );
        LP_side[ n ] = sum;
        HP_side[ n ] = side[ n + 1 ] - sum;
    }

    
    is10msFrame = frame_length == 10 * fs_kHz;
    smooth_coef_Q16 = is10msFrame ?
        SILK_FIX_CONST( STEREO_RATIO_SMOOTH_COEF / 2, 16 ) :
        SILK_FIX_CONST( STEREO_RATIO_SMOOTH_COEF,     16 );
    smooth_coef_Q16 = silk_SMULWB( silk_SMULBB( prev_speech_act_Q8, prev_speech_act_Q8 ), smooth_coef_Q16 );

    pred_Q13[ 0 ] = silk_stereo_find_predictor( &LP_ratio_Q14, LP_mid, LP_side, &state->mid_side_amp_Q0[ 0 ], frame_length, smooth_coef_Q16 );
    pred_Q13[ 1 ] = silk_stereo_find_predictor( &HP_ratio_Q14, HP_mid, HP_side, &state->mid_side_amp_Q0[ 2 ], frame_length, smooth_coef_Q16 );
    
    frac_Q16 = silk_SMLABB( HP_ratio_Q14, LP_ratio_Q14, 3 );
    frac_Q16 = silk_min( frac_Q16, SILK_FIX_CONST( 1, 16 ) );

    
    total_rate_bps -= is10msFrame ? 1200 : 600;      
    if( total_rate_bps < 1 ) {
        total_rate_bps = 1;
    }
    min_mid_rate_bps = silk_SMLABB( 2000, fs_kHz, 900 );
    silk_assert( min_mid_rate_bps < 32767 );
    
    frac_3_Q16 = silk_MUL( 3, frac_Q16 );
    mid_side_rates_bps[ 0 ] = silk_DIV32_varQ( total_rate_bps, SILK_FIX_CONST( 8 + 5, 16 ) + frac_3_Q16, 16+3 );
    
    if( mid_side_rates_bps[ 0 ] < min_mid_rate_bps ) {
        mid_side_rates_bps[ 0 ] = min_mid_rate_bps;
        mid_side_rates_bps[ 1 ] = total_rate_bps - mid_side_rates_bps[ 0 ];
        
        width_Q14 = silk_DIV32_varQ( silk_LSHIFT( mid_side_rates_bps[ 1 ], 1 ) - min_mid_rate_bps,
            silk_SMULWB( SILK_FIX_CONST( 1, 16 ) + frac_3_Q16, min_mid_rate_bps ), 14+2 );
        width_Q14 = silk_LIMIT( width_Q14, 0, SILK_FIX_CONST( 1, 14 ) );
    } else {
        mid_side_rates_bps[ 1 ] = total_rate_bps - mid_side_rates_bps[ 0 ];
        width_Q14 = SILK_FIX_CONST( 1, 14 );
    }

    
    state->smth_width_Q14 = (opus_int16)silk_SMLAWB( state->smth_width_Q14, width_Q14 - state->smth_width_Q14, smooth_coef_Q16 );

    
    *mid_only_flag = 0;
    if( toMono ) {
        
        width_Q14 = 0;
        pred_Q13[ 0 ] = 0;
        pred_Q13[ 1 ] = 0;
        silk_stereo_quant_pred( pred_Q13, ix );
    } else if( state->width_prev_Q14 == 0 &&
        ( 8 * total_rate_bps < 13 * min_mid_rate_bps || silk_SMULWB( frac_Q16, state->smth_width_Q14 ) < SILK_FIX_CONST( 0.05, 14 ) ) )
    {
        
        
        pred_Q13[ 0 ] = silk_RSHIFT( silk_SMULBB( state->smth_width_Q14, pred_Q13[ 0 ] ), 14 );
        pred_Q13[ 1 ] = silk_RSHIFT( silk_SMULBB( state->smth_width_Q14, pred_Q13[ 1 ] ), 14 );
        silk_stereo_quant_pred( pred_Q13, ix );
        
        width_Q14 = 0;
        pred_Q13[ 0 ] = 0;
        pred_Q13[ 1 ] = 0;
        mid_side_rates_bps[ 0 ] = total_rate_bps;
        mid_side_rates_bps[ 1 ] = 0;
        *mid_only_flag = 1;
    } else if( state->width_prev_Q14 != 0 &&
        ( 8 * total_rate_bps < 11 * min_mid_rate_bps || silk_SMULWB( frac_Q16, state->smth_width_Q14 ) < SILK_FIX_CONST( 0.02, 14 ) ) )
    {
        
        
        pred_Q13[ 0 ] = silk_RSHIFT( silk_SMULBB( state->smth_width_Q14, pred_Q13[ 0 ] ), 14 );
        pred_Q13[ 1 ] = silk_RSHIFT( silk_SMULBB( state->smth_width_Q14, pred_Q13[ 1 ] ), 14 );
        silk_stereo_quant_pred( pred_Q13, ix );
        
        width_Q14 = 0;
        pred_Q13[ 0 ] = 0;
        pred_Q13[ 1 ] = 0;
    } else if( state->smth_width_Q14 > SILK_FIX_CONST( 0.95, 14 ) ) {
        
        silk_stereo_quant_pred( pred_Q13, ix );
        width_Q14 = SILK_FIX_CONST( 1, 14 );
    } else {
        
        pred_Q13[ 0 ] = silk_RSHIFT( silk_SMULBB( state->smth_width_Q14, pred_Q13[ 0 ] ), 14 );
        pred_Q13[ 1 ] = silk_RSHIFT( silk_SMULBB( state->smth_width_Q14, pred_Q13[ 1 ] ), 14 );
        silk_stereo_quant_pred( pred_Q13, ix );
        width_Q14 = state->smth_width_Q14;
    }

    
    if( *mid_only_flag == 1 ) {
        state->silent_side_len += frame_length - STEREO_INTERP_LEN_MS * fs_kHz;
        if( state->silent_side_len < LA_SHAPE_MS * fs_kHz ) {
            *mid_only_flag = 0;
        } else {
            
            state->silent_side_len = 10000;
        }
    } else {
        state->silent_side_len = 0;
    }

    if( *mid_only_flag == 0 && mid_side_rates_bps[ 1 ] < 1 ) {
        mid_side_rates_bps[ 1 ] = 1;
        mid_side_rates_bps[ 0 ] = silk_max_int( 1, total_rate_bps - mid_side_rates_bps[ 1 ]);
    }

    
    pred0_Q13  = -state->pred_prev_Q13[ 0 ];
    pred1_Q13  = -state->pred_prev_Q13[ 1 ];
    w_Q24      =  silk_LSHIFT( state->width_prev_Q14, 10 );
    denom_Q16  = silk_DIV32_16( 1 << 16, STEREO_INTERP_LEN_MS * fs_kHz );
    delta0_Q13 = -silk_RSHIFT_ROUND( silk_SMULBB( pred_Q13[ 0 ] - state->pred_prev_Q13[ 0 ], denom_Q16 ), 16 );
    delta1_Q13 = -silk_RSHIFT_ROUND( silk_SMULBB( pred_Q13[ 1 ] - state->pred_prev_Q13[ 1 ], denom_Q16 ), 16 );
    deltaw_Q24 =  silk_LSHIFT( silk_SMULWB( width_Q14 - state->width_prev_Q14, denom_Q16 ), 10 );
    for( n = 0; n < STEREO_INTERP_LEN_MS * fs_kHz; n++ ) {
        pred0_Q13 += delta0_Q13;
        pred1_Q13 += delta1_Q13;
        w_Q24   += deltaw_Q24;
        sum = silk_LSHIFT( silk_ADD_LSHIFT( mid[ n ] + mid[ n + 2 ], mid[ n + 1 ], 1 ), 9 );    
        sum = silk_SMLAWB( silk_SMULWB( w_Q24, side[ n + 1 ] ), sum, pred0_Q13 );               
        sum = silk_SMLAWB( sum, silk_LSHIFT( (opus_int32)mid[ n + 1 ], 11 ), pred1_Q13 );       
        x2[ n - 1 ] = (opus_int16)silk_SAT16( silk_RSHIFT_ROUND( sum, 8 ) );
    }

    pred0_Q13 = -pred_Q13[ 0 ];
    pred1_Q13 = -pred_Q13[ 1 ];
    w_Q24     =  silk_LSHIFT( width_Q14, 10 );
    for( n = STEREO_INTERP_LEN_MS * fs_kHz; n < frame_length; n++ ) {
        sum = silk_LSHIFT( silk_ADD_LSHIFT( mid[ n ] + mid[ n + 2 ], mid[ n + 1 ], 1 ), 9 );    
        sum = silk_SMLAWB( silk_SMULWB( w_Q24, side[ n + 1 ] ), sum, pred0_Q13 );               
        sum = silk_SMLAWB( sum, silk_LSHIFT( (opus_int32)mid[ n + 1 ], 11 ), pred1_Q13 );       
        x2[ n - 1 ] = (opus_int16)silk_SAT16( silk_RSHIFT_ROUND( sum, 8 ) );
    }
    state->pred_prev_Q13[ 0 ] = (opus_int16)pred_Q13[ 0 ];
    state->pred_prev_Q13[ 1 ] = (opus_int16)pred_Q13[ 1 ];
    state->width_prev_Q14     = (opus_int16)width_Q14;
}
