






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "SigProc_FIX.h"


void silk_bwexpander(
    opus_int16                  *ar,                
    const opus_int              d,                  
    opus_int32                  chirp_Q16           
)
{
    opus_int   i;
    opus_int32 chirp_minus_one_Q16 = chirp_Q16 - 65536;

    
    
    for( i = 0; i < d - 1; i++ ) {
        ar[ i ]    = (opus_int16)silk_RSHIFT_ROUND( silk_MUL( chirp_Q16, ar[ i ]             ), 16 );
        chirp_Q16 +=            silk_RSHIFT_ROUND( silk_MUL( chirp_Q16, chirp_minus_one_Q16 ), 16 );
    }
    ar[ d - 1 ] = (opus_int16)silk_RSHIFT_ROUND( silk_MUL( chirp_Q16, ar[ d - 1 ] ), 16 );
}
