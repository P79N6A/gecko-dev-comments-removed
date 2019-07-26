






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "SigProc_FIX.h"

opus_int32 silk_inner_prod_aligned_scale(
    const opus_int16 *const     inVec1,             
    const opus_int16 *const     inVec2,             
    const opus_int              scale,              
    const opus_int              len                 
)
{
    opus_int   i;
    opus_int32 sum = 0;
    for( i = 0; i < len; i++ ) {
        sum = silk_ADD_RSHIFT32( sum, silk_SMULBB( inVec1[ i ], inVec2[ i ] ), scale );
    }
    return sum;
}
