






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "define.h"
#include "SigProc_FIX.h"








void silk_NLSF_VQ_weights_laroia(
    opus_int16                  *pNLSFW_Q_OUT,      
    const opus_int16            *pNLSF_Q15,         
    const opus_int              D                   
)
{
    opus_int   k;
    opus_int32 tmp1_int, tmp2_int;

    silk_assert( D > 0 );
    silk_assert( ( D & 1 ) == 0 );

    
    tmp1_int = silk_max_int( pNLSF_Q15[ 0 ], 1 );
    tmp1_int = silk_DIV32_16( 1 << ( 15 + NLSF_W_Q ), tmp1_int );
    tmp2_int = silk_max_int( pNLSF_Q15[ 1 ] - pNLSF_Q15[ 0 ], 1 );
    tmp2_int = silk_DIV32_16( 1 << ( 15 + NLSF_W_Q ), tmp2_int );
    pNLSFW_Q_OUT[ 0 ] = (opus_int16)silk_min_int( tmp1_int + tmp2_int, silk_int16_MAX );
    silk_assert( pNLSFW_Q_OUT[ 0 ] > 0 );

    
    for( k = 1; k < D - 1; k += 2 ) {
        tmp1_int = silk_max_int( pNLSF_Q15[ k + 1 ] - pNLSF_Q15[ k ], 1 );
        tmp1_int = silk_DIV32_16( 1 << ( 15 + NLSF_W_Q ), tmp1_int );
        pNLSFW_Q_OUT[ k ] = (opus_int16)silk_min_int( tmp1_int + tmp2_int, silk_int16_MAX );
        silk_assert( pNLSFW_Q_OUT[ k ] > 0 );

        tmp2_int = silk_max_int( pNLSF_Q15[ k + 2 ] - pNLSF_Q15[ k + 1 ], 1 );
        tmp2_int = silk_DIV32_16( 1 << ( 15 + NLSF_W_Q ), tmp2_int );
        pNLSFW_Q_OUT[ k + 1 ] = (opus_int16)silk_min_int( tmp1_int + tmp2_int, silk_int16_MAX );
        silk_assert( pNLSFW_Q_OUT[ k + 1 ] > 0 );
    }

    
    tmp1_int = silk_max_int( ( 1 << 15 ) - pNLSF_Q15[ D - 1 ], 1 );
    tmp1_int = silk_DIV32_16( 1 << ( 15 + NLSF_W_Q ), tmp1_int );
    pNLSFW_Q_OUT[ D - 1 ] = (opus_int16)silk_min_int( tmp1_int + tmp2_int, silk_int16_MAX );
    silk_assert( pNLSFW_Q_OUT[ D - 1 ] > 0 );
}
