






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main.h"


void silk_NLSF_VQ(
    opus_int32                  err_Q26[],                      
    const opus_int16            in_Q15[],                       
    const opus_uint8            pCB_Q8[],                       
    const opus_int              K,                              
    const opus_int              LPC_order                       
)
{
    opus_int        i, m;
    opus_int32      diff_Q15, sum_error_Q30, sum_error_Q26;

    silk_assert( LPC_order <= 16 );
    silk_assert( ( LPC_order & 1 ) == 0 );

    
    for( i = 0; i < K; i++ ) {
        sum_error_Q26 = 0;
        for( m = 0; m < LPC_order; m += 2 ) {
            
            diff_Q15 = silk_SUB_LSHIFT32( in_Q15[ m ], (opus_int32)*pCB_Q8++, 7 ); 
            sum_error_Q30 = silk_SMULBB( diff_Q15, diff_Q15 );

            
            diff_Q15 = silk_SUB_LSHIFT32( in_Q15[m + 1], (opus_int32)*pCB_Q8++, 7 ); 
            sum_error_Q30 = silk_SMLABB( sum_error_Q30, diff_Q15, diff_Q15 );

            sum_error_Q26 = silk_ADD_RSHIFT32( sum_error_Q26, sum_error_Q30, 4 );

            silk_assert( sum_error_Q26 >= 0 );
            silk_assert( sum_error_Q30 >= 0 );
        }
        err_Q26[ i ] = sum_error_Q26;
    }
}
