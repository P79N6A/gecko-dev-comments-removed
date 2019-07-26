






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "SigProc_FIX.h"
#include "resampler_rom.h"


void silk_resampler_down2(
    opus_int32                  *S,                 
    opus_int16                  *out,               
    const opus_int16            *in,                
    opus_int32                  inLen               
)
{
    opus_int32 k, len2 = silk_RSHIFT32( inLen, 1 );
    opus_int32 in32, out32, Y, X;

    silk_assert( silk_resampler_down2_0 > 0 );
    silk_assert( silk_resampler_down2_1 < 0 );

    
    for( k = 0; k < len2; k++ ) {
        
        in32 = silk_LSHIFT( (opus_int32)in[ 2 * k ], 10 );

        
        Y      = silk_SUB32( in32, S[ 0 ] );
        X      = silk_SMLAWB( Y, Y, silk_resampler_down2_1 );
        out32  = silk_ADD32( S[ 0 ], X );
        S[ 0 ] = silk_ADD32( in32, X );

        
        in32 = silk_LSHIFT( (opus_int32)in[ 2 * k + 1 ], 10 );

        
        Y      = silk_SUB32( in32, S[ 1 ] );
        X      = silk_SMULWB( Y, silk_resampler_down2_0 );
        out32  = silk_ADD32( out32, S[ 1 ] );
        out32  = silk_ADD32( out32, X );
        S[ 1 ] = silk_ADD32( in32, X );

        
        out[ k ] = (opus_int16)silk_SAT16( silk_RSHIFT_ROUND( out32, 11 ) );
    }
}

