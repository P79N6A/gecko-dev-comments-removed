






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "SigProc_FIX.h"


void silk_autocorr(
    opus_int32                  *results,           
    opus_int                    *scale,             
    const opus_int16            *inputData,         
    const opus_int              inputDataSize,      
    const opus_int              correlationCount    
)
{
    opus_int   i, lz, nRightShifts, corrCount;
    opus_int64 corr64;

    corrCount = silk_min_int( inputDataSize, correlationCount );

    
    corr64 = silk_inner_prod16_aligned_64( inputData, inputData, inputDataSize );

    
    corr64 += 1;

    
    lz = silk_CLZ64( corr64 );

    
    nRightShifts = 35 - lz;
    *scale = nRightShifts;

    if( nRightShifts <= 0 ) {
        results[ 0 ] = silk_LSHIFT( (opus_int32)silk_CHECK_FIT32( corr64 ), -nRightShifts );

        
          for( i = 1; i < corrCount; i++ ) {
            results[ i ] = silk_LSHIFT( silk_inner_prod_aligned( inputData, inputData + i, inputDataSize - i ), -nRightShifts );
        }
    } else {
        results[ 0 ] = (opus_int32)silk_CHECK_FIT32( silk_RSHIFT64( corr64, nRightShifts ) );

        
          for( i = 1; i < corrCount; i++ ) {
            results[ i ] =  (opus_int32)silk_CHECK_FIT32( silk_RSHIFT64( silk_inner_prod16_aligned_64( inputData, inputData + i, inputDataSize - i ), nRightShifts ) );
        }
    }
}
