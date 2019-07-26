






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "SigProc_FIX.h"


void silk_bwexpander_32(
    opus_int32                  *ar,                
    const opus_int              d,                  
    opus_int32                  chirp_Q16           
)
{
    opus_int   i;
    opus_int32 chirp_minus_one_Q16 = chirp_Q16 - 65536;

    for( i = 0; i < d - 1; i++ ) {
        ar[ i ]    = silk_SMULWW( chirp_Q16, ar[ i ] );
        chirp_Q16 += silk_RSHIFT_ROUND( silk_MUL( chirp_Q16, chirp_minus_one_Q16 ), 16 );
    }
    ar[ d - 1 ] = silk_SMULWW( chirp_Q16, ar[ d - 1 ] );
}

