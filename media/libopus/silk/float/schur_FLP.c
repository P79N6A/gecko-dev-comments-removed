






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "SigProc_FLP.h"

silk_float silk_schur_FLP(                  
    silk_float          refl_coef[],        
    const silk_float    auto_corr[],        
    opus_int            order               
)
{
    opus_int   k, n;
    silk_float C[ SILK_MAX_ORDER_LPC + 1 ][ 2 ];
    silk_float Ctmp1, Ctmp2, rc_tmp;

    silk_assert( order==6||order==8||order==10||order==12||order==14||order==16 );

    
    for( k = 0; k < order+1; k++ ) {
        C[ k ][ 0 ] = C[ k ][ 1 ] = auto_corr[ k ];
    }

    for( k = 0; k < order; k++ ) {
        
        rc_tmp = -C[ k + 1 ][ 0 ] / silk_max_float( C[ 0 ][ 1 ], 1e-9f );

        
        refl_coef[ k ] = rc_tmp;

        
        for( n = 0; n < order - k; n++ ) {
            Ctmp1 = C[ n + k + 1 ][ 0 ];
            Ctmp2 = C[ n ][ 1 ];
            C[ n + k + 1 ][ 0 ] = Ctmp1 + Ctmp2 * rc_tmp;
            C[ n ][ 1 ]         = Ctmp2 + Ctmp1 * rc_tmp;
        }
    }

    
    return C[ 0 ][ 1 ];
}

