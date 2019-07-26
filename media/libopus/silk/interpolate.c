






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main.h"


void silk_interpolate(
    opus_int16                  xi[ MAX_LPC_ORDER ],            
    const opus_int16            x0[ MAX_LPC_ORDER ],            
    const opus_int16            x1[ MAX_LPC_ORDER ],            
    const opus_int              ifact_Q2,                       
    const opus_int              d                               
)
{
    opus_int i;

    silk_assert( ifact_Q2 >= 0 );
    silk_assert( ifact_Q2 <= 4 );

    for( i = 0; i < d; i++ ) {
        xi[ i ] = (opus_int16)silk_ADD_RSHIFT( x0[ i ], silk_SMULBB( x1[ i ] - x0[ i ], ifact_Q2 ), 2 );
    }
}
