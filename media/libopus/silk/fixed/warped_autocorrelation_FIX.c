






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main_FIX.h"

#define QC  10
#define QS  14


void silk_warped_autocorrelation_FIX(
          opus_int32                *corr,                                  
          opus_int                  *scale,                                 
    const opus_int16                *input,                                 
    const opus_int                  warping_Q16,                            
    const opus_int                  length,                                 
    const opus_int                  order                                   
)
{
    opus_int   n, i, lsh;
    opus_int32 tmp1_QS, tmp2_QS;
    opus_int32 state_QS[ MAX_SHAPE_LPC_ORDER + 1 ] = { 0 };
    opus_int64 corr_QC[  MAX_SHAPE_LPC_ORDER + 1 ] = { 0 };

    
    silk_assert( ( order & 1 ) == 0 );
    silk_assert( 2 * QS - QC >= 0 );

    
    for( n = 0; n < length; n++ ) {
        tmp1_QS = silk_LSHIFT32( (opus_int32)input[ n ], QS );
        
        for( i = 0; i < order; i += 2 ) {
            
            tmp2_QS = silk_SMLAWB( state_QS[ i ], state_QS[ i + 1 ] - tmp1_QS, warping_Q16 );
            state_QS[ i ]  = tmp1_QS;
            corr_QC[  i ] += silk_RSHIFT64( silk_SMULL( tmp1_QS, state_QS[ 0 ] ), 2 * QS - QC );
            
            tmp1_QS = silk_SMLAWB( state_QS[ i + 1 ], state_QS[ i + 2 ] - tmp2_QS, warping_Q16 );
            state_QS[ i + 1 ]  = tmp2_QS;
            corr_QC[  i + 1 ] += silk_RSHIFT64( silk_SMULL( tmp2_QS, state_QS[ 0 ] ), 2 * QS - QC );
        }
        state_QS[ order ] = tmp1_QS;
        corr_QC[  order ] += silk_RSHIFT64( silk_SMULL( tmp1_QS, state_QS[ 0 ] ), 2 * QS - QC );
    }

    lsh = silk_CLZ64( corr_QC[ 0 ] ) - 35;
    lsh = silk_LIMIT( lsh, -12 - QC, 30 - QC );
    *scale = -( QC + lsh );
    silk_assert( *scale >= -30 && *scale <= 12 );
    if( lsh >= 0 ) {
        for( i = 0; i < order + 1; i++ ) {
            corr[ i ] = (opus_int32)silk_CHECK_FIT32( silk_LSHIFT64( corr_QC[ i ], lsh ) );
        }
    } else {
        for( i = 0; i < order + 1; i++ ) {
            corr[ i ] = (opus_int32)silk_CHECK_FIT32( silk_RSHIFT64( corr_QC[ i ], -lsh ) );
        }
    }
    silk_assert( corr_QC[ 0 ] >= 0 ); 
}
