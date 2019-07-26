






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif





#include "typedef.h"
#include "SigProc_FLP.h"

void silk_insertion_sort_decreasing_FLP(
    silk_float          *a,                 
    opus_int            *idx,               
    const opus_int      L,                  
    const opus_int      K                   
)
{
    silk_float value;
    opus_int   i, j;

    
    silk_assert( K >  0 );
    silk_assert( L >  0 );
    silk_assert( L >= K );

    
    for( i = 0; i < K; i++ ) {
        idx[ i ] = i;
    }

    
    for( i = 1; i < K; i++ ) {
        value = a[ i ];
        for( j = i - 1; ( j >= 0 ) && ( value > a[ j ] ); j-- ) {
            a[ j + 1 ]   = a[ j ];      
            idx[ j + 1 ] = idx[ j ];    
        }
        a[ j + 1 ]   = value;   
        idx[ j + 1 ] = i;       
    }

    
    
    for( i = K; i < L; i++ ) {
        value = a[ i ];
        if( value > a[ K - 1 ] ) {
            for( j = K - 2; ( j >= 0 ) && ( value > a[ j ] ); j-- ) {
                a[ j + 1 ]   = a[ j ];      
                idx[ j + 1 ] = idx[ j ];    
            }
            a[ j + 1 ]   = value;   
            idx[ j + 1 ] = i;       
        }
    }
}
