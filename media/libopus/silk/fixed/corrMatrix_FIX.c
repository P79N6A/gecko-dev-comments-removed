






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif





#include "main_FIX.h"


void silk_corrVector_FIX(
    const opus_int16                *x,                                     
    const opus_int16                *t,                                     
    const opus_int                  L,                                      
    const opus_int                  order,                                  
    opus_int32                      *Xt,                                    
    const opus_int                  rshifts                                 
)
{
    opus_int         lag, i;
    const opus_int16 *ptr1, *ptr2;
    opus_int32       inner_prod;

    ptr1 = &x[ order - 1 ]; 
    ptr2 = t;
    
    if( rshifts > 0 ) {
        
        for( lag = 0; lag < order; lag++ ) {
            inner_prod = 0;
            for( i = 0; i < L; i++ ) {
                inner_prod += silk_RSHIFT32( silk_SMULBB( ptr1[ i ], ptr2[i] ), rshifts );
            }
            Xt[ lag ] = inner_prod; 
            ptr1--; 
        }
    } else {
        silk_assert( rshifts == 0 );
        for( lag = 0; lag < order; lag++ ) {
            Xt[ lag ] = silk_inner_prod_aligned( ptr1, ptr2, L ); 
            ptr1--; 
        }
    }
}


void silk_corrMatrix_FIX(
    const opus_int16                *x,                                     
    const opus_int                  L,                                      
    const opus_int                  order,                                  
    const opus_int                  head_room,                              
    opus_int32                      *XX,                                    
    opus_int                        *rshifts                                
)
{
    opus_int         i, j, lag, rshifts_local, head_room_rshifts;
    opus_int32       energy;
    const opus_int16 *ptr1, *ptr2;

    
    silk_sum_sqr_shift( &energy, &rshifts_local, x, L + order - 1 );
    
    head_room_rshifts = silk_max( head_room - silk_CLZ32( energy ), 0 );

    energy = silk_RSHIFT32( energy, head_room_rshifts );
    rshifts_local += head_room_rshifts;

    
    
    for( i = 0; i < order - 1; i++ ) {
        energy -= silk_RSHIFT32( silk_SMULBB( x[ i ], x[ i ] ), rshifts_local );
    }
    if( rshifts_local < *rshifts ) {
        
        energy = silk_RSHIFT32( energy, *rshifts - rshifts_local );
        rshifts_local = *rshifts;
    }

    
    
    matrix_ptr( XX, 0, 0, order ) = energy;
    ptr1 = &x[ order - 1 ]; 
    for( j = 1; j < order; j++ ) {
        energy = silk_SUB32( energy, silk_RSHIFT32( silk_SMULBB( ptr1[ L - j ], ptr1[ L - j ] ), rshifts_local ) );
        energy = silk_ADD32( energy, silk_RSHIFT32( silk_SMULBB( ptr1[ -j ], ptr1[ -j ] ), rshifts_local ) );
        matrix_ptr( XX, j, j, order ) = energy;
    }

    ptr2 = &x[ order - 2 ]; 
    
    if( rshifts_local > 0 ) {
        
        for( lag = 1; lag < order; lag++ ) {
            
            energy = 0;
            for( i = 0; i < L; i++ ) {
                energy += silk_RSHIFT32( silk_SMULBB( ptr1[ i ], ptr2[i] ), rshifts_local );
            }
            
            matrix_ptr( XX, lag, 0, order ) = energy;
            matrix_ptr( XX, 0, lag, order ) = energy;
            for( j = 1; j < ( order - lag ); j++ ) {
                energy = silk_SUB32( energy, silk_RSHIFT32( silk_SMULBB( ptr1[ L - j ], ptr2[ L - j ] ), rshifts_local ) );
                energy = silk_ADD32( energy, silk_RSHIFT32( silk_SMULBB( ptr1[ -j ], ptr2[ -j ] ), rshifts_local ) );
                matrix_ptr( XX, lag + j, j, order ) = energy;
                matrix_ptr( XX, j, lag + j, order ) = energy;
            }
            ptr2--; 
        }
    } else {
        for( lag = 1; lag < order; lag++ ) {
            
            energy = silk_inner_prod_aligned( ptr1, ptr2, L );
            matrix_ptr( XX, lag, 0, order ) = energy;
            matrix_ptr( XX, 0, lag, order ) = energy;
            
            for( j = 1; j < ( order - lag ); j++ ) {
                energy = silk_SUB32( energy, silk_SMULBB( ptr1[ L - j ], ptr2[ L - j ] ) );
                energy = silk_SMLABB( energy, ptr1[ -j ], ptr2[ -j ] );
                matrix_ptr( XX, lag + j, j, order ) = energy;
                matrix_ptr( XX, j, lag + j, order ) = energy;
            }
            ptr2--;
        }
    }
    *rshifts = rshifts_local;
}

