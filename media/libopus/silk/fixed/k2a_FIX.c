






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "SigProc_FIX.h"


void silk_k2a(
    opus_int32                  *A_Q24,             
    const opus_int16            *rc_Q15,            
    const opus_int32            order               
)
{
    opus_int   k, n;
    opus_int32 Atmp[ SILK_MAX_ORDER_LPC ];

    for( k = 0; k < order; k++ ) {
        for( n = 0; n < k; n++ ) {
            Atmp[ n ] = A_Q24[ n ];
        }
        for( n = 0; n < k; n++ ) {
            A_Q24[ n ] = silk_SMLAWB( A_Q24[ n ], silk_LSHIFT( Atmp[ k - n - 1 ], 1 ), rc_Q15[ k ] );
        }
        A_Q24[ k ] = -silk_LSHIFT( (opus_int32)rc_Q15[ k ], 9 );
    }
}
