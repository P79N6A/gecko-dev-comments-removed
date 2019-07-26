






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "SigProc_FIX.h"
#include "resampler_private.h"




void silk_resampler_private_up2_HQ(
    opus_int32                      *S,             
    opus_int16                      *out,           
    const opus_int16                *in,            
    opus_int32                      len             
)
{
    opus_int32 k;
    opus_int32 in32, out32_1, out32_2, Y, X;

    silk_assert( silk_resampler_up2_hq_0[ 0 ] > 0 );
    silk_assert( silk_resampler_up2_hq_0[ 1 ] > 0 );
    silk_assert( silk_resampler_up2_hq_0[ 2 ] < 0 );
    silk_assert( silk_resampler_up2_hq_1[ 0 ] > 0 );
    silk_assert( silk_resampler_up2_hq_1[ 1 ] > 0 );
    silk_assert( silk_resampler_up2_hq_1[ 2 ] < 0 );

    
    for( k = 0; k < len; k++ ) {
        
        in32 = silk_LSHIFT( (opus_int32)in[ k ], 10 );

        
        Y       = silk_SUB32( in32, S[ 0 ] );
        X       = silk_SMULWB( Y, silk_resampler_up2_hq_0[ 0 ] );
        out32_1 = silk_ADD32( S[ 0 ], X );
        S[ 0 ]  = silk_ADD32( in32, X );

        
        Y       = silk_SUB32( out32_1, S[ 1 ] );
        X       = silk_SMULWB( Y, silk_resampler_up2_hq_0[ 1 ] );
        out32_2 = silk_ADD32( S[ 1 ], X );
        S[ 1 ]  = silk_ADD32( out32_1, X );

        
        Y       = silk_SUB32( out32_2, S[ 2 ] );
        X       = silk_SMLAWB( Y, Y, silk_resampler_up2_hq_0[ 2 ] );
        out32_1 = silk_ADD32( S[ 2 ], X );
        S[ 2 ]  = silk_ADD32( out32_2, X );

        
        out[ 2 * k ] = (opus_int16)silk_SAT16( silk_RSHIFT_ROUND( out32_1, 10 ) );

        
        Y       = silk_SUB32( in32, S[ 3 ] );
        X       = silk_SMULWB( Y, silk_resampler_up2_hq_1[ 0 ] );
        out32_1 = silk_ADD32( S[ 3 ], X );
        S[ 3 ]  = silk_ADD32( in32, X );

        
        Y       = silk_SUB32( out32_1, S[ 4 ] );
        X       = silk_SMULWB( Y, silk_resampler_up2_hq_1[ 1 ] );
        out32_2 = silk_ADD32( S[ 4 ], X );
        S[ 4 ]  = silk_ADD32( out32_1, X );

        
        Y       = silk_SUB32( out32_2, S[ 5 ] );
        X       = silk_SMLAWB( Y, Y, silk_resampler_up2_hq_1[ 2 ] );
        out32_1 = silk_ADD32( S[ 5 ], X );
        S[ 5 ]  = silk_ADD32( out32_2, X );

        
        out[ 2 * k + 1 ] = (opus_int16)silk_SAT16( silk_RSHIFT_ROUND( out32_1, 10 ) );
    }
}

void silk_resampler_private_up2_HQ_wrapper(
    void                            *SS,            
    opus_int16                      *out,           
    const opus_int16                *in,            
    opus_int32                      len             
)
{
    silk_resampler_state_struct *S = (silk_resampler_state_struct *)SS;
    silk_resampler_private_up2_HQ( S->sIIR, out, in, len );
}
