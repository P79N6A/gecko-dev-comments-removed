






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "SigProc_FIX.h"
#include "SigProc_FLP.h"

#define RC_THRESHOLD        0.9999f




silk_float silk_LPC_inverse_pred_gain_FLP(  
    const silk_float    *A,                 
    opus_int32          order               
)
{
    opus_int   k, n;
    double     invGain, rc, rc_mult1, rc_mult2;
    silk_float Atmp[ 2 ][ SILK_MAX_ORDER_LPC ];
    silk_float *Aold, *Anew;

    Anew = Atmp[ order & 1 ];
    silk_memcpy( Anew, A, order * sizeof(silk_float) );

    invGain = 1.0;
    for( k = order - 1; k > 0; k-- ) {
        rc = -Anew[ k ];
        if( rc > RC_THRESHOLD || rc < -RC_THRESHOLD ) {
            return 0.0f;
        }
        rc_mult1 = 1.0f - rc * rc;
        rc_mult2 = 1.0f / rc_mult1;
        invGain *= rc_mult1;
        
        Aold = Anew;
        Anew = Atmp[ k & 1 ];
        for( n = 0; n < k; n++ ) {
            Anew[ n ] = (silk_float)( ( Aold[ n ] - Aold[ k - n - 1 ] * rc ) * rc_mult2 );
        }
    }
    rc = -Anew[ 0 ];
    if( rc > RC_THRESHOLD || rc < -RC_THRESHOLD ) {
        return 0.0f;
    }
    rc_mult1 = 1.0f - rc * rc;
    invGain *= rc_mult1;
    return (silk_float)invGain;
}
