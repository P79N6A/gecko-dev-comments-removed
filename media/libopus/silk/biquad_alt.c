





































#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "SigProc_FIX.h"


void silk_biquad_alt(
    const opus_int16            *in,                
    const opus_int32            *B_Q28,             
    const opus_int32            *A_Q28,             
    opus_int32                  *S,                 
    opus_int16                  *out,               
    const opus_int32            len,                
    opus_int                    stride              
)
{
    
    opus_int   k;
    opus_int32 inval, A0_U_Q28, A0_L_Q28, A1_U_Q28, A1_L_Q28, out32_Q14;

    
    A0_L_Q28 = ( -A_Q28[ 0 ] ) & 0x00003FFF;        
    A0_U_Q28 = silk_RSHIFT( -A_Q28[ 0 ], 14 );      
    A1_L_Q28 = ( -A_Q28[ 1 ] ) & 0x00003FFF;        
    A1_U_Q28 = silk_RSHIFT( -A_Q28[ 1 ], 14 );      

    for( k = 0; k < len; k++ ) {
        
        inval = in[ k * stride ];
        out32_Q14 = silk_LSHIFT( silk_SMLAWB( S[ 0 ], B_Q28[ 0 ], inval ), 2 );

        S[ 0 ] = S[1] + silk_RSHIFT_ROUND( silk_SMULWB( out32_Q14, A0_L_Q28 ), 14 );
        S[ 0 ] = silk_SMLAWB( S[ 0 ], out32_Q14, A0_U_Q28 );
        S[ 0 ] = silk_SMLAWB( S[ 0 ], B_Q28[ 1 ], inval);

        S[ 1 ] = silk_RSHIFT_ROUND( silk_SMULWB( out32_Q14, A1_L_Q28 ), 14 );
        S[ 1 ] = silk_SMLAWB( S[ 1 ], out32_Q14, A1_U_Q28 );
        S[ 1 ] = silk_SMLAWB( S[ 1 ], B_Q28[ 2 ], inval );

        
        out[ k * stride ] = (opus_int16)silk_SAT16( silk_RSHIFT( out32_Q14 + (1<<14) - 1, 14 ) );
    }
}
