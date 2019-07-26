






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main_FLP.h"

#define MAX_ITERATIONS_RESIDUAL_NRG         10
#define REGULARIZATION_FACTOR               1e-8f


silk_float silk_residual_energy_covar_FLP(                              
    const silk_float                *c,                                 
    silk_float                      *wXX,                               
    const silk_float                *wXx,                               
    const silk_float                wxx,                                
    const opus_int                  D                                   
)
{
    opus_int   i, j, k;
    silk_float tmp, nrg = 0.0f, regularization;

    
    silk_assert( D >= 0 );

    regularization = REGULARIZATION_FACTOR * ( wXX[ 0 ] + wXX[ D * D - 1 ] );
    for( k = 0; k < MAX_ITERATIONS_RESIDUAL_NRG; k++ ) {
        nrg = wxx;

        tmp = 0.0f;
        for( i = 0; i < D; i++ ) {
            tmp += wXx[ i ] * c[ i ];
        }
        nrg -= 2.0f * tmp;

        
        for( i = 0; i < D; i++ ) {
            tmp = 0.0f;
            for( j = i + 1; j < D; j++ ) {
                tmp += matrix_c_ptr( wXX, i, j, D ) * c[ j ];
            }
            nrg += c[ i ] * ( 2.0f * tmp + matrix_c_ptr( wXX, i, i, D ) * c[ i ] );
        }
        if( nrg > 0 ) {
            break;
        } else {
            
            for( i = 0; i < D; i++ ) {
                matrix_c_ptr( wXX, i, i, D ) +=  regularization;
            }
            
            regularization *= 2.0f;
        }
    }
    if( k == MAX_ITERATIONS_RESIDUAL_NRG ) {
        silk_assert( nrg == 0 );
        nrg = 1.0f;
    }

    return nrg;
}



void silk_residual_energy_FLP(
    silk_float                      nrgs[ MAX_NB_SUBFR ],               
    const silk_float                x[],                                
    silk_float                      a[ 2 ][ MAX_LPC_ORDER ],            
    const silk_float                gains[],                            
    const opus_int                  subfr_length,                       
    const opus_int                  nb_subfr,                           
    const opus_int                  LPC_order                           
)
{
    opus_int     shift;
    silk_float   *LPC_res_ptr, LPC_res[ ( MAX_FRAME_LENGTH + MAX_NB_SUBFR * MAX_LPC_ORDER ) / 2 ];

    LPC_res_ptr = LPC_res + LPC_order;
    shift = LPC_order + subfr_length;

    
    silk_LPC_analysis_filter_FLP( LPC_res, a[ 0 ], x + 0 * shift, 2 * shift, LPC_order );
    nrgs[ 0 ] = ( silk_float )( gains[ 0 ] * gains[ 0 ] * silk_energy_FLP( LPC_res_ptr + 0 * shift, subfr_length ) );
    nrgs[ 1 ] = ( silk_float )( gains[ 1 ] * gains[ 1 ] * silk_energy_FLP( LPC_res_ptr + 1 * shift, subfr_length ) );

    if( nb_subfr == MAX_NB_SUBFR ) {
        silk_LPC_analysis_filter_FLP( LPC_res, a[ 1 ], x + 2 * shift, 2 * shift, LPC_order );
        nrgs[ 2 ] = ( silk_float )( gains[ 2 ] * gains[ 2 ] * silk_energy_FLP( LPC_res_ptr + 0 * shift, subfr_length ) );
        nrgs[ 3 ] = ( silk_float )( gains[ 3 ] * gains[ 3 ] * silk_energy_FLP( LPC_res_ptr + 1 * shift, subfr_length ) );
    }
}
