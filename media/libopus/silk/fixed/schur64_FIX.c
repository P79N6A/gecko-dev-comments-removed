






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "SigProc_FIX.h"



opus_int32 silk_schur64(                            
    opus_int32                  rc_Q16[],           
    const opus_int32            c[],                
    opus_int32                  order               
)
{
    opus_int   k, n;
    opus_int32 C[ SILK_MAX_ORDER_LPC + 1 ][ 2 ];
    opus_int32 Ctmp1_Q30, Ctmp2_Q30, rc_tmp_Q31;

    silk_assert( order==6||order==8||order==10||order==12||order==14||order==16 );

    
    if( c[ 0 ] <= 0 ) {
        silk_memset( rc_Q16, 0, order * sizeof( opus_int32 ) );
        return 0;
    }

    for( k = 0; k < order + 1; k++ ) {
        C[ k ][ 0 ] = C[ k ][ 1 ] = c[ k ];
    }

    for( k = 0; k < order; k++ ) {
        
        rc_tmp_Q31 = silk_DIV32_varQ( -C[ k + 1 ][ 0 ], C[ 0 ][ 1 ], 31 );

        
        rc_Q16[ k ] = silk_RSHIFT_ROUND( rc_tmp_Q31, 15 );

        
        for( n = 0; n < order - k; n++ ) {
            Ctmp1_Q30 = C[ n + k + 1 ][ 0 ];
            Ctmp2_Q30 = C[ n ][ 1 ];

            
            C[ n + k + 1 ][ 0 ] = Ctmp1_Q30 + silk_SMMUL( silk_LSHIFT( Ctmp2_Q30, 1 ), rc_tmp_Q31 );
            C[ n ][ 1 ]         = Ctmp2_Q30 + silk_SMMUL( silk_LSHIFT( Ctmp1_Q30, 1 ), rc_tmp_Q31 );
        }
    }

    return( C[ 0 ][ 1 ] );
}
