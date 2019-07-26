






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif







#include "SigProc_FIX.h"

void silk_insertion_sort_increasing(
    opus_int32           *a,             
    opus_int             *idx,           
    const opus_int       L,              
    const opus_int       K               
)
{
    opus_int32    value;
    opus_int        i, j;

    
    silk_assert( K >  0 );
    silk_assert( L >  0 );
    silk_assert( L >= K );

    
    for( i = 0; i < K; i++ ) {
        idx[ i ] = i;
    }

    
    for( i = 1; i < K; i++ ) {
        value = a[ i ];
        for( j = i - 1; ( j >= 0 ) && ( value < a[ j ] ); j-- ) {
            a[ j + 1 ]   = a[ j ];       
            idx[ j + 1 ] = idx[ j ];     
        }
        a[ j + 1 ]   = value;   
        idx[ j + 1 ] = i;       
    }

    
    
    for( i = K; i < L; i++ ) {
        value = a[ i ];
        if( value < a[ K - 1 ] ) {
            for( j = K - 2; ( j >= 0 ) && ( value < a[ j ] ); j-- ) {
                a[ j + 1 ]   = a[ j ];       
                idx[ j + 1 ] = idx[ j ];     
            }
            a[ j + 1 ]   = value;   
            idx[ j + 1 ] = i;       
        }
    }
}

#ifdef FIXED_POINT

void silk_insertion_sort_decreasing_int16(
    opus_int16                  *a,                 
    opus_int                    *idx,               
    const opus_int              L,                  
    const opus_int              K                   
)
{
    opus_int i, j;
    opus_int value;

    
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
#endif

void silk_insertion_sort_increasing_all_values_int16(
     opus_int16                 *a,                 
     const opus_int             L                   
)
{
    opus_int    value;
    opus_int    i, j;

    
    silk_assert( L >  0 );

    
    for( i = 1; i < L; i++ ) {
        value = a[ i ];
        for( j = i - 1; ( j >= 0 ) && ( value < a[ j ] ); j-- ) {
            a[ j + 1 ] = a[ j ]; 
        }
        a[ j + 1 ] = value; 
    }
}
