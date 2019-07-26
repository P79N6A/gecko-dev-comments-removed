






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main_FIX.h"



void silk_residual_energy_FIX(
          opus_int32                nrgs[ MAX_NB_SUBFR ],                   
          opus_int                  nrgsQ[ MAX_NB_SUBFR ],                  
    const opus_int16                x[],                                    
          opus_int16                a_Q12[ 2 ][ MAX_LPC_ORDER ],            
    const opus_int32                gains[ MAX_NB_SUBFR ],                  
    const opus_int                  subfr_length,                           
    const opus_int                  nb_subfr,                               
    const opus_int                  LPC_order                               
)
{
    opus_int         offset, i, j, rshift, lz1, lz2;
    opus_int16       *LPC_res_ptr, LPC_res[ ( MAX_FRAME_LENGTH + MAX_NB_SUBFR * MAX_LPC_ORDER ) / 2 ];
    const opus_int16 *x_ptr;
    opus_int32       tmp32;

    x_ptr  = x;
    offset = LPC_order + subfr_length;

    
    for( i = 0; i < nb_subfr >> 1; i++ ) {
        
        silk_LPC_analysis_filter( LPC_res, x_ptr, a_Q12[ i ], ( MAX_NB_SUBFR >> 1 ) * offset, LPC_order );

        
        LPC_res_ptr = LPC_res + LPC_order;
        for( j = 0; j < ( MAX_NB_SUBFR >> 1 ); j++ ) {
            
            silk_sum_sqr_shift( &nrgs[ i * ( MAX_NB_SUBFR >> 1 ) + j ], &rshift, LPC_res_ptr, subfr_length );

            
            nrgsQ[ i * ( MAX_NB_SUBFR >> 1 ) + j ] = -rshift;

            
            LPC_res_ptr += offset;
        }
        
        x_ptr += ( MAX_NB_SUBFR >> 1 ) * offset;
    }

    
    for( i = 0; i < nb_subfr; i++ ) {
        
        lz1 = silk_CLZ32( nrgs[  i ] ) - 1;
        lz2 = silk_CLZ32( gains[ i ] ) - 1;

        tmp32 = silk_LSHIFT32( gains[ i ], lz2 );

        
        tmp32 = silk_SMMUL( tmp32, tmp32 ); 

        
        nrgs[ i ] = silk_SMMUL( tmp32, silk_LSHIFT32( nrgs[ i ], lz1 ) ); 
        nrgsQ[ i ] += lz1 + 2 * lz2 - 32 - 32;
    }
}
