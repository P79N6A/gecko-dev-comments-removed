






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main.h"


void silk_VQ_WMat_EC(
    opus_int8                   *ind,                           
    opus_int32                  *rate_dist_Q14,                 
    const opus_int16            *in_Q14,                        
    const opus_int32            *W_Q18,                         
    const opus_int8             *cb_Q7,                         
    const opus_uint8            *cl_Q5,                         
    const opus_int              mu_Q9,                          
    opus_int                    L                               
)
{
    opus_int   k;
    const opus_int8 *cb_row_Q7;
    opus_int16 diff_Q14[ 5 ];
    opus_int32 sum1_Q14, sum2_Q16;

    
    *rate_dist_Q14 = silk_int32_MAX;
    cb_row_Q7 = cb_Q7;
    for( k = 0; k < L; k++ ) {
        diff_Q14[ 0 ] = in_Q14[ 0 ] - silk_LSHIFT( cb_row_Q7[ 0 ], 7 );
        diff_Q14[ 1 ] = in_Q14[ 1 ] - silk_LSHIFT( cb_row_Q7[ 1 ], 7 );
        diff_Q14[ 2 ] = in_Q14[ 2 ] - silk_LSHIFT( cb_row_Q7[ 2 ], 7 );
        diff_Q14[ 3 ] = in_Q14[ 3 ] - silk_LSHIFT( cb_row_Q7[ 3 ], 7 );
        diff_Q14[ 4 ] = in_Q14[ 4 ] - silk_LSHIFT( cb_row_Q7[ 4 ], 7 );

        
        sum1_Q14 = silk_SMULBB( mu_Q9, cl_Q5[ k ] );

        silk_assert( sum1_Q14 >= 0 );

        
        sum2_Q16 = silk_SMULWB(           W_Q18[  1 ], diff_Q14[ 1 ] );
        sum2_Q16 = silk_SMLAWB( sum2_Q16, W_Q18[  2 ], diff_Q14[ 2 ] );
        sum2_Q16 = silk_SMLAWB( sum2_Q16, W_Q18[  3 ], diff_Q14[ 3 ] );
        sum2_Q16 = silk_SMLAWB( sum2_Q16, W_Q18[  4 ], diff_Q14[ 4 ] );
        sum2_Q16 = silk_LSHIFT( sum2_Q16, 1 );
        sum2_Q16 = silk_SMLAWB( sum2_Q16, W_Q18[  0 ], diff_Q14[ 0 ] );
        sum1_Q14 = silk_SMLAWB( sum1_Q14, sum2_Q16,    diff_Q14[ 0 ] );

        
        sum2_Q16 = silk_SMULWB(           W_Q18[  7 ], diff_Q14[ 2 ] );
        sum2_Q16 = silk_SMLAWB( sum2_Q16, W_Q18[  8 ], diff_Q14[ 3 ] );
        sum2_Q16 = silk_SMLAWB( sum2_Q16, W_Q18[  9 ], diff_Q14[ 4 ] );
        sum2_Q16 = silk_LSHIFT( sum2_Q16, 1 );
        sum2_Q16 = silk_SMLAWB( sum2_Q16, W_Q18[  6 ], diff_Q14[ 1 ] );
        sum1_Q14 = silk_SMLAWB( sum1_Q14, sum2_Q16,    diff_Q14[ 1 ] );

        
        sum2_Q16 = silk_SMULWB(           W_Q18[ 13 ], diff_Q14[ 3 ] );
        sum2_Q16 = silk_SMLAWB( sum2_Q16, W_Q18[ 14 ], diff_Q14[ 4 ] );
        sum2_Q16 = silk_LSHIFT( sum2_Q16, 1 );
        sum2_Q16 = silk_SMLAWB( sum2_Q16, W_Q18[ 12 ], diff_Q14[ 2 ] );
        sum1_Q14 = silk_SMLAWB( sum1_Q14, sum2_Q16,    diff_Q14[ 2 ] );

        
        sum2_Q16 = silk_SMULWB(           W_Q18[ 19 ], diff_Q14[ 4 ] );
        sum2_Q16 = silk_LSHIFT( sum2_Q16, 1 );
        sum2_Q16 = silk_SMLAWB( sum2_Q16, W_Q18[ 18 ], diff_Q14[ 3 ] );
        sum1_Q14 = silk_SMLAWB( sum1_Q14, sum2_Q16,    diff_Q14[ 3 ] );

        
        sum2_Q16 = silk_SMULWB(           W_Q18[ 24 ], diff_Q14[ 4 ] );
        sum1_Q14 = silk_SMLAWB( sum1_Q14, sum2_Q16,    diff_Q14[ 4 ] );

        silk_assert( sum1_Q14 >= 0 );

        
        if( sum1_Q14 < *rate_dist_Q14 ) {
            *rate_dist_Q14 = sum1_Q14;
            *ind = (opus_int8)k;
        }

        
        cb_row_Q7 += LTP_ORDER;
    }
}
