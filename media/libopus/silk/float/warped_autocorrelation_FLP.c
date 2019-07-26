






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main_FLP.h"


void silk_warped_autocorrelation_FLP(
    silk_float                      *corr,                              
    const silk_float                *input,                             
    const silk_float                warping,                            
    const opus_int                  length,                             
    const opus_int                  order                               
)
{
    opus_int    n, i;
    double      tmp1, tmp2;
    double      state[ MAX_SHAPE_LPC_ORDER + 1 ] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
    double      C[     MAX_SHAPE_LPC_ORDER + 1 ] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

    
    silk_assert( ( order & 1 ) == 0 );

    
    for( n = 0; n < length; n++ ) {
        tmp1 = input[ n ];
        
        for( i = 0; i < order; i += 2 ) {
            
            tmp2 = state[ i ] + warping * ( state[ i + 1 ] - tmp1 );
            state[ i ] = tmp1;
            C[ i ] += state[ 0 ] * tmp1;
            
            tmp1 = state[ i + 1 ] + warping * ( state[ i + 2 ] - tmp2 );
            state[ i + 1 ] = tmp2;
            C[ i + 1 ] += state[ 0 ] * tmp2;
        }
        state[ order ] = tmp1;
        C[ order ] += state[ 0 ] * tmp1;
    }

    
    for( i = 0; i < order + 1; i++ ) {
        corr[ i ] = ( silk_float )C[ i ];
    }
}
