






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "SigProc_FIX.h"



void silk_sum_sqr_shift(
    opus_int32                  *energy,            
    opus_int                    *shift,             
    const opus_int16            *x,                 
    opus_int                    len                 
)
{
    opus_int   i, shft;
    opus_int32 nrg_tmp, nrg;

    nrg  = 0;
    shft = 0;
    len--;
    for( i = 0; i < len; i += 2 ) {
        nrg = silk_SMLABB_ovflw( nrg, x[ i ], x[ i ] );
        nrg = silk_SMLABB_ovflw( nrg, x[ i + 1 ], x[ i + 1 ] );
        if( nrg < 0 ) {
            
            nrg = (opus_int32)silk_RSHIFT_uint( (opus_uint32)nrg, 2 );
            shft = 2;
            break;
        }
    }
    for( ; i < len; i += 2 ) {
        nrg_tmp = silk_SMULBB( x[ i ], x[ i ] );
        nrg_tmp = silk_SMLABB_ovflw( nrg_tmp, x[ i + 1 ], x[ i + 1 ] );
        nrg = (opus_int32)silk_ADD_RSHIFT_uint( nrg, (opus_uint32)nrg_tmp, shft );
        if( nrg < 0 ) {
            
            nrg = (opus_int32)silk_RSHIFT_uint( (opus_uint32)nrg, 2 );
            shft += 2;
        }
    }
    if( i == len ) {
        
        nrg_tmp = silk_SMULBB( x[ i ], x[ i ] );
        nrg = (opus_int32)silk_ADD_RSHIFT_uint( nrg, nrg_tmp, shft );
    }

    
    if( nrg & 0xC0000000 ) {
        nrg = silk_RSHIFT_uint( (opus_uint32)nrg, 2 );
        shft += 2;
    }

    
    *shift  = shft;
    *energy = nrg;
}

