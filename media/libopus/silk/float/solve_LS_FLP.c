






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main_FLP.h"
#include "tuning_parameters.h"






static inline void silk_LDL_FLP(
    silk_float          *A,         
    opus_int            M,          
    silk_float          *L,         
    silk_float          *Dinv       
);





static inline void silk_SolveWithLowerTriangularWdiagOnes_FLP(
    const silk_float    *L,         
    opus_int            M,          
    const silk_float    *b,         
    silk_float          *x          
);





static inline void silk_SolveWithUpperTriangularFromLowerWdiagOnes_FLP(
    const silk_float    *L,         
    opus_int            M,          
    const silk_float    *b,         
    silk_float          *x          
);





void silk_solve_LDL_FLP(
    silk_float                      *A,                                 
    const opus_int                  M,                                  
    const silk_float                *b,                                 
    silk_float                      *x                                  
)
{
    opus_int   i;
    silk_float L[    MAX_MATRIX_SIZE ][ MAX_MATRIX_SIZE ];
    silk_float T[    MAX_MATRIX_SIZE ];
    silk_float Dinv[ MAX_MATRIX_SIZE ]; 

    silk_assert( M <= MAX_MATRIX_SIZE );

    



    silk_LDL_FLP( A, M, &L[ 0 ][ 0 ], Dinv );

    



    silk_SolveWithLowerTriangularWdiagOnes_FLP( &L[ 0 ][ 0 ], M, b, T );

    



    for( i = 0; i < M; i++ ) {
        T[ i ] = T[ i ] * Dinv[ i ];
    }
    


    silk_SolveWithUpperTriangularFromLowerWdiagOnes_FLP( &L[ 0 ][ 0 ], M, T, x );
}

static inline void silk_SolveWithUpperTriangularFromLowerWdiagOnes_FLP(
    const silk_float    *L,         
    opus_int            M,          
    const silk_float    *b,         
    silk_float          *x          
)
{
    opus_int   i, j;
    silk_float temp;
    const silk_float *ptr1;

    for( i = M - 1; i >= 0; i-- ) {
        ptr1 =  matrix_adr( L, 0, i, M );
        temp = 0;
        for( j = M - 1; j > i ; j-- ) {
            temp += ptr1[ j * M ] * x[ j ];
        }
        temp = b[ i ] - temp;
        x[ i ] = temp;
    }
}

static inline void silk_SolveWithLowerTriangularWdiagOnes_FLP(
    const silk_float    *L,         
    opus_int            M,          
    const silk_float    *b,         
    silk_float          *x          
)
{
    opus_int   i, j;
    silk_float temp;
    const silk_float *ptr1;

    for( i = 0; i < M; i++ ) {
        ptr1 =  matrix_adr( L, i, 0, M );
        temp = 0;
        for( j = 0; j < i; j++ ) {
            temp += ptr1[ j ] * x[ j ];
        }
        temp = b[ i ] - temp;
        x[ i ] = temp;
    }
}

static inline void silk_LDL_FLP(
    silk_float          *A,         
    opus_int            M,          
    silk_float          *L,         
    silk_float          *Dinv       
)
{
    opus_int i, j, k, loop_count, err = 1;
    silk_float *ptr1, *ptr2;
    double temp, diag_min_value;
    silk_float v[ MAX_MATRIX_SIZE ], D[ MAX_MATRIX_SIZE ]; 

    silk_assert( M <= MAX_MATRIX_SIZE );

    diag_min_value = FIND_LTP_COND_FAC * 0.5f * ( A[ 0 ] + A[ M * M - 1 ] );
    for( loop_count = 0; loop_count < M && err == 1; loop_count++ ) {
        err = 0;
        for( j = 0; j < M; j++ ) {
            ptr1 = matrix_adr( L, j, 0, M );
            temp = matrix_ptr( A, j, j, M ); 
            for( i = 0; i < j; i++ ) {
                v[ i ] = ptr1[ i ] * D[ i ];
                temp  -= ptr1[ i ] * v[ i ];
            }
            if( temp < diag_min_value ) {
                
                temp = ( loop_count + 1 ) * diag_min_value - temp;
                for( i = 0; i < M; i++ ) {
                    matrix_ptr( A, i, i, M ) += ( silk_float )temp;
                }
                err = 1;
                break;
            }
            D[ j ]    = ( silk_float )temp;
            Dinv[ j ] = ( silk_float )( 1.0f / temp );
            matrix_ptr( L, j, j, M ) = 1.0f;

            ptr1 = matrix_adr( A, j, 0, M );
            ptr2 = matrix_adr( L, j + 1, 0, M);
            for( i = j + 1; i < M; i++ ) {
                temp = 0.0;
                for( k = 0; k < j; k++ ) {
                    temp += ptr2[ k ] * v[ k ];
                }
                matrix_ptr( L, i, j, M ) = ( silk_float )( ( ptr1[ i ] - temp ) * Dinv[ j ] );
                ptr2 += M; 
            }
        }
    }
    silk_assert( err == 0 );
}

