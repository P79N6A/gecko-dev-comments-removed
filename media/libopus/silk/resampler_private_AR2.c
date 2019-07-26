






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "SigProc_FIX.h"
#include "resampler_private.h"


void silk_resampler_private_AR2(
    opus_int32                      S[],            
    opus_int32                      out_Q8[],       
    const opus_int16                in[],           
    const opus_int16                A_Q14[],        
    opus_int32                      len             
)
{
    opus_int32    k;
    opus_int32    out32;

    for( k = 0; k < len; k++ ) {
        out32       = silk_ADD_LSHIFT32( S[ 0 ], (opus_int32)in[ k ], 8 );
        out_Q8[ k ] = out32;
        out32       = silk_LSHIFT( out32, 2 );
        S[ 0 ]      = silk_SMLAWB( S[ 1 ], out32, A_Q14[ 0 ] );
        S[ 1 ]      = silk_SMULWB( out32, A_Q14[ 1 ] );
    }
}

