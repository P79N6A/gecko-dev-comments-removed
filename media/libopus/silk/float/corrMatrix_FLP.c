






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif





#include "main_FLP.h"


void silk_corrVector_FLP(
    const silk_float                *x,                                 
    const silk_float                *t,                                 
    const opus_int                  L,                                  
    const opus_int                  Order,                              
    silk_float                      *Xt                                 
)
{
    opus_int lag;
    const silk_float *ptr1;

    ptr1 = &x[ Order - 1 ];                     
    for( lag = 0; lag < Order; lag++ ) {
        
        Xt[ lag ] = (silk_float)silk_inner_product_FLP( ptr1, t, L );
        ptr1--;                                 
    }
}


void silk_corrMatrix_FLP(
    const silk_float                *x,                                 
    const opus_int                  L,                                  
    const opus_int                  Order,                              
    silk_float                      *XX                                 
)
{
    opus_int j, lag;
    double  energy;
    const silk_float *ptr1, *ptr2;

    ptr1 = &x[ Order - 1 ];                     
    energy = silk_energy_FLP( ptr1, L );  
    matrix_ptr( XX, 0, 0, Order ) = ( silk_float )energy;
    for( j = 1; j < Order; j++ ) {
        
        energy += ptr1[ -j ] * ptr1[ -j ] - ptr1[ L - j ] * ptr1[ L - j ];
        matrix_ptr( XX, j, j, Order ) = ( silk_float )energy;
    }

    ptr2 = &x[ Order - 2 ];                     
    for( lag = 1; lag < Order; lag++ ) {
        
        energy = silk_inner_product_FLP( ptr1, ptr2, L );
        matrix_ptr( XX, lag, 0, Order ) = ( silk_float )energy;
        matrix_ptr( XX, 0, lag, Order ) = ( silk_float )energy;
        
        for( j = 1; j < ( Order - lag ); j++ ) {
            energy += ptr1[ -j ] * ptr2[ -j ] - ptr1[ L - j ] * ptr2[ L - j ];
            matrix_ptr( XX, lag + j, j, Order ) = ( silk_float )energy;
            matrix_ptr( XX, j, lag + j, Order ) = ( silk_float )energy;
        }
        ptr2--;                                 
    }
}
