






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main_FIX.h"


opus_int32 silk_residual_energy16_covar_FIX(
    const opus_int16                *c,                                     
    const opus_int32                *wXX,                                   
    const opus_int32                *wXx,                                   
    opus_int32                      wxx,                                    
    opus_int                        D,                                      
    opus_int                        cQ                                      
)
{
    opus_int   i, j, lshifts, Qxtra;
    opus_int32 c_max, w_max, tmp, tmp2, nrg;
    opus_int   cn[ MAX_MATRIX_SIZE ];
    const opus_int32 *pRow;

    
    silk_assert( D >=  0 );
    silk_assert( D <= 16 );
    silk_assert( cQ >  0 );
    silk_assert( cQ < 16 );

    lshifts = 16 - cQ;
    Qxtra = lshifts;

    c_max = 0;
    for( i = 0; i < D; i++ ) {
        c_max = silk_max_32( c_max, silk_abs( (opus_int32)c[ i ] ) );
    }
    Qxtra = silk_min_int( Qxtra, silk_CLZ32( c_max ) - 17 );

    w_max = silk_max_32( wXX[ 0 ], wXX[ D * D - 1 ] );
    Qxtra = silk_min_int( Qxtra, silk_CLZ32( silk_MUL( D, silk_RSHIFT( silk_SMULWB( w_max, c_max ), 4 ) ) ) - 5 );
    Qxtra = silk_max_int( Qxtra, 0 );
    for( i = 0; i < D; i++ ) {
        cn[ i ] = silk_LSHIFT( ( opus_int )c[ i ], Qxtra );
        silk_assert( silk_abs(cn[i]) <= ( silk_int16_MAX + 1 ) ); 
    }
    lshifts -= Qxtra;

    
    tmp = 0;
    for( i = 0; i < D; i++ ) {
        tmp = silk_SMLAWB( tmp, wXx[ i ], cn[ i ] );
    }
    nrg = silk_RSHIFT( wxx, 1 + lshifts ) - tmp;                         

    
    tmp2 = 0;
    for( i = 0; i < D; i++ ) {
        tmp = 0;
        pRow = &wXX[ i * D ];
        for( j = i + 1; j < D; j++ ) {
            tmp = silk_SMLAWB( tmp, pRow[ j ], cn[ j ] );
        }
        tmp  = silk_SMLAWB( tmp,  silk_RSHIFT( pRow[ i ], 1 ), cn[ i ] );
        tmp2 = silk_SMLAWB( tmp2, tmp,                        cn[ i ] );
    }
    nrg = silk_ADD_LSHIFT32( nrg, tmp2, lshifts );                       

    
    if( nrg < 1 ) {
        nrg = 1;
    } else if( nrg > silk_RSHIFT( silk_int32_MAX, lshifts + 2 ) ) {
        nrg = silk_int32_MAX >> 1;
    } else {
        nrg = silk_LSHIFT( nrg, lshifts + 1 );                           
    }
    return nrg;

}
