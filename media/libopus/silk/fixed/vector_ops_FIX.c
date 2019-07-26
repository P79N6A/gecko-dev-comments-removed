






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "SigProc_FIX.h"


void silk_scale_copy_vector16(
    opus_int16                  *data_out,
    const opus_int16            *data_in,
    opus_int32                  gain_Q16,           
    const opus_int              dataSize            
)
{
    opus_int  i;
    opus_int32 tmp32;

    for( i = 0; i < dataSize; i++ ) {
        tmp32 = silk_SMULWB( gain_Q16, data_in[ i ] );
        data_out[ i ] = (opus_int16)silk_CHECK_FIT16( tmp32 );
    }
}


void silk_scale_vector32_Q26_lshift_18(
    opus_int32                  *data1,             
    opus_int32                  gain_Q26,           
    opus_int                    dataSize            
)
{
    opus_int  i;

    for( i = 0; i < dataSize; i++ ) {
        data1[ i ] = (opus_int32)silk_CHECK_FIT32( silk_RSHIFT64( silk_SMULL( data1[ i ], gain_Q26 ), 8 ) );    
    }
}






opus_int32 silk_inner_prod_aligned(
    const opus_int16 *const     inVec1,             
    const opus_int16 *const     inVec2,             
    const opus_int              len                 
)
{
    opus_int   i;
    opus_int32 sum = 0;
    for( i = 0; i < len; i++ ) {
        sum = silk_SMLABB( sum, inVec1[ i ], inVec2[ i ] );
    }
    return sum;
}

opus_int64 silk_inner_prod16_aligned_64(
    const opus_int16            *inVec1,            
    const opus_int16            *inVec2,            
    const opus_int              len                 
)
{
    opus_int   i;
    opus_int64 sum = 0;
    for( i = 0; i < len; i++ ) {
        sum = silk_SMLALBB( sum, inVec1[ i ], inVec2[ i ] );
    }
    return sum;
}


opus_int16 silk_int16_array_maxabs(                 
    const opus_int16            *vec,               
    const opus_int32            len                 
)
{
    opus_int32 max = 0, i, lvl = 0, ind;
    if( len == 0 ) return 0;

    ind = len - 1;
    max = silk_SMULBB( vec[ ind ], vec[ ind ] );
    for( i = len - 2; i >= 0; i-- ) {
        lvl = silk_SMULBB( vec[ i ], vec[ i ] );
        if( lvl > max ) {
            max = lvl;
            ind = i;
        }
    }

    
    if( max >= 1073676289 ) {           
        return( silk_int16_MAX );
    } else {
        if( vec[ ind ] < 0 ) {
            return( -vec[ ind ] );
        } else {
            return(  vec[ ind ] );
        }
    }
}
