






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "SigProc_FIX.h"



opus_int32 silk_schur(                              
    opus_int16                  *rc_Q15,            
    const opus_int32            *c,                 
    const opus_int32            order               
)
{
    opus_int        k, n, lz;
    opus_int32    C[ SILK_MAX_ORDER_LPC + 1 ][ 2 ];
    opus_int32    Ctmp1, Ctmp2, rc_tmp_Q15;

    silk_assert( order==6||order==8||order==10||order==12||order==14||order==16 );

    
    lz = silk_CLZ32( c[ 0 ] );

    
    if( lz < 2 ) {
        
        for( k = 0; k < order + 1; k++ ) {
            C[ k ][ 0 ] = C[ k ][ 1 ] = silk_RSHIFT( c[ k ], 1 );
        }
    } else if( lz > 2 ) {
        
        lz -= 2;
        for( k = 0; k < order + 1; k++ ) {
            C[ k ][ 0 ] = C[ k ][ 1 ] = silk_LSHIFT( c[ k ], lz );
        }
    } else {
        
        for( k = 0; k < order + 1; k++ ) {
            C[ k ][ 0 ] = C[ k ][ 1 ] = c[ k ];
        }
    }

    for( k = 0; k < order; k++ ) {

        
        rc_tmp_Q15 = -silk_DIV32_16( C[ k + 1 ][ 0 ], silk_max_32( silk_RSHIFT( C[ 0 ][ 1 ], 15 ), 1 ) );

        
        rc_tmp_Q15 = silk_SAT16( rc_tmp_Q15 );

        
        rc_Q15[ k ] = (opus_int16)rc_tmp_Q15;

        
        for( n = 0; n < order - k; n++ ) {
            Ctmp1 = C[ n + k + 1 ][ 0 ];
            Ctmp2 = C[ n ][ 1 ];
            C[ n + k + 1 ][ 0 ] = silk_SMLAWB( Ctmp1, silk_LSHIFT( Ctmp2, 1 ), rc_tmp_Q15 );
            C[ n ][ 1 ]         = silk_SMLAWB( Ctmp2, silk_LSHIFT( Ctmp1, 1 ), rc_tmp_Q15 );
        }
    }

    
    return C[ 0 ][ 1 ];
}
