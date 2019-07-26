






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "SigProc_FIX.h"


static opus_int16 A_fb1_20 = 5394 << 1;
static opus_int16 A_fb1_21 = -24290; 


void silk_ana_filt_bank_1(
    const opus_int16            *in,                
    opus_int32                  *S,                 
    opus_int16                  *outL,              
    opus_int16                  *outH,              
    const opus_int32            N                   
)
{
    opus_int      k, N2 = silk_RSHIFT( N, 1 );
    opus_int32    in32, X, Y, out_1, out_2;

    
    for( k = 0; k < N2; k++ ) {
        
        in32 = silk_LSHIFT( (opus_int32)in[ 2 * k ], 10 );

        
        Y      = silk_SUB32( in32, S[ 0 ] );
        X      = silk_SMLAWB( Y, Y, A_fb1_21 );
        out_1  = silk_ADD32( S[ 0 ], X );
        S[ 0 ] = silk_ADD32( in32, X );

        
        in32 = silk_LSHIFT( (opus_int32)in[ 2 * k + 1 ], 10 );

        
        Y      = silk_SUB32( in32, S[ 1 ] );
        X      = silk_SMULWB( Y, A_fb1_20 );
        out_2  = silk_ADD32( S[ 1 ], X );
        S[ 1 ] = silk_ADD32( in32, X );

        
        outL[ k ] = (opus_int16)silk_SAT16( silk_RSHIFT_ROUND( silk_ADD32( out_2, out_1 ), 11 ) );
        outH[ k ] = (opus_int16)silk_SAT16( silk_RSHIFT_ROUND( silk_SUB32( out_2, out_1 ), 11 ) );
    }
}
