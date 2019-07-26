






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "SigProc_FLP.h"


void silk_k2a_FLP(
    silk_float          *A,                 
    const silk_float    *rc,                
    opus_int32          order               
)
{
    opus_int   k, n;
    silk_float Atmp[ SILK_MAX_ORDER_LPC ];

    for( k = 0; k < order; k++ ) {
        for( n = 0; n < k; n++ ) {
            Atmp[ n ] = A[ n ];
        }
        for( n = 0; n < k; n++ ) {
            A[ n ] += Atmp[ k - n - 1 ] * rc[ k ];
        }
        A[ k ] = -rc[ k ];
    }
}
