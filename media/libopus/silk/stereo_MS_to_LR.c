






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main.h"


void silk_stereo_MS_to_LR(
    stereo_dec_state            *state,                         
    opus_int16                  x1[],                           
    opus_int16                  x2[],                           
    const opus_int32            pred_Q13[],                     
    opus_int                    fs_kHz,                         
    opus_int                    frame_length                    
)
{
    opus_int   n, denom_Q16, delta0_Q13, delta1_Q13;
    opus_int32 sum, diff, pred0_Q13, pred1_Q13;

    
    silk_memcpy( x1, state->sMid,  2 * sizeof( opus_int16 ) );
    silk_memcpy( x2, state->sSide, 2 * sizeof( opus_int16 ) );
    silk_memcpy( state->sMid,  &x1[ frame_length ], 2 * sizeof( opus_int16 ) );
    silk_memcpy( state->sSide, &x2[ frame_length ], 2 * sizeof( opus_int16 ) );

    
    pred0_Q13  = state->pred_prev_Q13[ 0 ];
    pred1_Q13  = state->pred_prev_Q13[ 1 ];
    denom_Q16  = silk_DIV32_16( 1 << 16, STEREO_INTERP_LEN_MS * fs_kHz );
    delta0_Q13 = silk_RSHIFT_ROUND( silk_SMULBB( pred_Q13[ 0 ] - state->pred_prev_Q13[ 0 ], denom_Q16 ), 16 );
    delta1_Q13 = silk_RSHIFT_ROUND( silk_SMULBB( pred_Q13[ 1 ] - state->pred_prev_Q13[ 1 ], denom_Q16 ), 16 );
    for( n = 0; n < STEREO_INTERP_LEN_MS * fs_kHz; n++ ) {
        pred0_Q13 += delta0_Q13;
        pred1_Q13 += delta1_Q13;
        sum = silk_LSHIFT( silk_ADD_LSHIFT( x1[ n ] + x1[ n + 2 ], x1[ n + 1 ], 1 ), 9 );       
        sum = silk_SMLAWB( silk_LSHIFT( (opus_int32)x2[ n + 1 ], 8 ), sum, pred0_Q13 );         
        sum = silk_SMLAWB( sum, silk_LSHIFT( (opus_int32)x1[ n + 1 ], 11 ), pred1_Q13 );        
        x2[ n + 1 ] = (opus_int16)silk_SAT16( silk_RSHIFT_ROUND( sum, 8 ) );
    }
    pred0_Q13 = pred_Q13[ 0 ];
    pred1_Q13 = pred_Q13[ 1 ];
    for( n = STEREO_INTERP_LEN_MS * fs_kHz; n < frame_length; n++ ) {
        sum = silk_LSHIFT( silk_ADD_LSHIFT( x1[ n ] + x1[ n + 2 ], x1[ n + 1 ], 1 ), 9 );       
        sum = silk_SMLAWB( silk_LSHIFT( (opus_int32)x2[ n + 1 ], 8 ), sum, pred0_Q13 );         
        sum = silk_SMLAWB( sum, silk_LSHIFT( (opus_int32)x1[ n + 1 ], 11 ), pred1_Q13 );        
        x2[ n + 1 ] = (opus_int16)silk_SAT16( silk_RSHIFT_ROUND( sum, 8 ) );
    }
    state->pred_prev_Q13[ 0 ] = pred_Q13[ 0 ];
    state->pred_prev_Q13[ 1 ] = pred_Q13[ 1 ];

    
    for( n = 0; n < frame_length; n++ ) {
        sum  = x1[ n + 1 ] + (opus_int32)x2[ n + 1 ];
        diff = x1[ n + 1 ] - (opus_int32)x2[ n + 1 ];
        x1[ n + 1 ] = (opus_int16)silk_SAT16( sum );
        x2[ n + 1 ] = (opus_int16)silk_SAT16( diff );
    }
}
