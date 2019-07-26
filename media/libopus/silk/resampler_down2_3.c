






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "SigProc_FIX.h"
#include "resampler_private.h"

#define ORDER_FIR                   4


void silk_resampler_down2_3(
    opus_int32                  *S,                 
    opus_int16                  *out,               
    const opus_int16            *in,                
    opus_int32                  inLen               
)
{
    opus_int32 nSamplesIn, counter, res_Q6;
    opus_int32 buf[ RESAMPLER_MAX_BATCH_SIZE_IN + ORDER_FIR ];
    opus_int32 *buf_ptr;

    
    silk_memcpy( buf, S, ORDER_FIR * sizeof( opus_int32 ) );

    
    while( 1 ) {
        nSamplesIn = silk_min( inLen, RESAMPLER_MAX_BATCH_SIZE_IN );

        
        silk_resampler_private_AR2( &S[ ORDER_FIR ], &buf[ ORDER_FIR ], in,
            silk_Resampler_2_3_COEFS_LQ, nSamplesIn );

        
        buf_ptr = buf;
        counter = nSamplesIn;
        while( counter > 2 ) {
            
            res_Q6 = silk_SMULWB(         buf_ptr[ 0 ], silk_Resampler_2_3_COEFS_LQ[ 2 ] );
            res_Q6 = silk_SMLAWB( res_Q6, buf_ptr[ 1 ], silk_Resampler_2_3_COEFS_LQ[ 3 ] );
            res_Q6 = silk_SMLAWB( res_Q6, buf_ptr[ 2 ], silk_Resampler_2_3_COEFS_LQ[ 5 ] );
            res_Q6 = silk_SMLAWB( res_Q6, buf_ptr[ 3 ], silk_Resampler_2_3_COEFS_LQ[ 4 ] );

            
            *out++ = (opus_int16)silk_SAT16( silk_RSHIFT_ROUND( res_Q6, 6 ) );

            res_Q6 = silk_SMULWB(         buf_ptr[ 1 ], silk_Resampler_2_3_COEFS_LQ[ 4 ] );
            res_Q6 = silk_SMLAWB( res_Q6, buf_ptr[ 2 ], silk_Resampler_2_3_COEFS_LQ[ 5 ] );
            res_Q6 = silk_SMLAWB( res_Q6, buf_ptr[ 3 ], silk_Resampler_2_3_COEFS_LQ[ 3 ] );
            res_Q6 = silk_SMLAWB( res_Q6, buf_ptr[ 4 ], silk_Resampler_2_3_COEFS_LQ[ 2 ] );

            
            *out++ = (opus_int16)silk_SAT16( silk_RSHIFT_ROUND( res_Q6, 6 ) );

            buf_ptr += 3;
            counter -= 3;
        }

        in += nSamplesIn;
        inLen -= nSamplesIn;

        if( inLen > 0 ) {
            
            silk_memcpy( buf, &buf[ nSamplesIn ], ORDER_FIR * sizeof( opus_int32 ) );
        } else {
            break;
        }
    }

    
    silk_memcpy( S, &buf[ nSamplesIn ], ORDER_FIR * sizeof( opus_int32 ) );
}
