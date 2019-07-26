






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main_FIX.h"
#include "tuning_parameters.h"





typedef struct {
    opus_int32 Q36_part;
    opus_int32 Q48_part;
} inv_D_t;


static inline void silk_LDL_factorize_FIX(
    opus_int32          *A,         
    opus_int            M,          
    opus_int32          *L_Q16,     
    inv_D_t             *inv_D      
);


static inline void silk_LS_SolveFirst_FIX(
    const opus_int32    *L_Q16,     
    opus_int            M,          
    const opus_int32    *b,         
    opus_int32          *x_Q16      
);


static inline void silk_LS_SolveLast_FIX(
    const opus_int32    *L_Q16,     
    const opus_int      M,          
    const opus_int32    *b,         
    opus_int32          *x_Q16      
);

static inline void silk_LS_divide_Q16_FIX(
    opus_int32          T[],        
    inv_D_t             *inv_D,     
    opus_int            M           
);


void silk_solve_LDL_FIX(
    opus_int32                      *A,                                     
    opus_int                        M,                                      
    const opus_int32                *b,                                     
    opus_int32                      *x_Q16                                  
)
{
    opus_int32 L_Q16[  MAX_MATRIX_SIZE * MAX_MATRIX_SIZE ];
    opus_int32 Y[      MAX_MATRIX_SIZE ];
    inv_D_t   inv_D[  MAX_MATRIX_SIZE ];

    silk_assert( M <= MAX_MATRIX_SIZE );

    



    silk_LDL_factorize_FIX( A, M, L_Q16, inv_D );

    



    silk_LS_SolveFirst_FIX( L_Q16, M, b, Y );

    



    silk_LS_divide_Q16_FIX( Y, inv_D, M );

    


    silk_LS_SolveLast_FIX( L_Q16, M, Y, x_Q16 );
}

static inline void silk_LDL_factorize_FIX(
    opus_int32          *A,         
    opus_int            M,          
    opus_int32          *L_Q16,     
    inv_D_t             *inv_D      
)
{
    opus_int   i, j, k, status, loop_count;
    const opus_int32 *ptr1, *ptr2;
    opus_int32 diag_min_value, tmp_32, err;
    opus_int32 v_Q0[ MAX_MATRIX_SIZE ], D_Q0[ MAX_MATRIX_SIZE ];
    opus_int32 one_div_diag_Q36, one_div_diag_Q40, one_div_diag_Q48;

    silk_assert( M <= MAX_MATRIX_SIZE );

    status = 1;
    diag_min_value = silk_max_32( silk_SMMUL( silk_ADD_SAT32( A[ 0 ], A[ silk_SMULBB( M, M ) - 1 ] ), SILK_FIX_CONST( FIND_LTP_COND_FAC, 31 ) ), 1 << 9 );
    for( loop_count = 0; loop_count < M && status == 1; loop_count++ ) {
        status = 0;
        for( j = 0; j < M; j++ ) {
            ptr1 = matrix_adr( L_Q16, j, 0, M );
            tmp_32 = 0;
            for( i = 0; i < j; i++ ) {
                v_Q0[ i ] = silk_SMULWW(         D_Q0[ i ], ptr1[ i ] ); 
                tmp_32    = silk_SMLAWW( tmp_32, v_Q0[ i ], ptr1[ i ] ); 
            }
            tmp_32 = silk_SUB32( matrix_ptr( A, j, j, M ), tmp_32 );

            if( tmp_32 < diag_min_value ) {
                tmp_32 = silk_SUB32( silk_SMULBB( loop_count + 1, diag_min_value ), tmp_32 );
                
                for( i = 0; i < M; i++ ) {
                    matrix_ptr( A, i, i, M ) = silk_ADD32( matrix_ptr( A, i, i, M ), tmp_32 );
                }
                status = 1;
                break;
            }
            D_Q0[ j ] = tmp_32;                         

            
            one_div_diag_Q36 = silk_INVERSE32_varQ( tmp_32, 36 );                    
            one_div_diag_Q40 = silk_LSHIFT( one_div_diag_Q36, 4 );                   
            err = silk_SUB32( 1 << 24, silk_SMULWW( tmp_32, one_div_diag_Q40 ) );     
            one_div_diag_Q48 = silk_SMULWW( err, one_div_diag_Q40 );                 

            
            inv_D[ j ].Q36_part = one_div_diag_Q36;
            inv_D[ j ].Q48_part = one_div_diag_Q48;

            matrix_ptr( L_Q16, j, j, M ) = 65536; 
            ptr1 = matrix_adr( A, j, 0, M );
            ptr2 = matrix_adr( L_Q16, j + 1, 0, M );
            for( i = j + 1; i < M; i++ ) {
                tmp_32 = 0;
                for( k = 0; k < j; k++ ) {
                    tmp_32 = silk_SMLAWW( tmp_32, v_Q0[ k ], ptr2[ k ] ); 
                }
                tmp_32 = silk_SUB32( ptr1[ i ], tmp_32 ); 

                
                matrix_ptr( L_Q16, i, j, M ) = silk_ADD32( silk_SMMUL( tmp_32, one_div_diag_Q48 ),
                    silk_RSHIFT( silk_SMULWW( tmp_32, one_div_diag_Q36 ), 4 ) );

                
                ptr2 += M;
            }
        }
    }

    silk_assert( status == 0 );
}

static inline void silk_LS_divide_Q16_FIX(
    opus_int32          T[],        
    inv_D_t             *inv_D,     
    opus_int            M           
)
{
    opus_int   i;
    opus_int32 tmp_32;
    opus_int32 one_div_diag_Q36, one_div_diag_Q48;

    for( i = 0; i < M; i++ ) {
        one_div_diag_Q36 = inv_D[ i ].Q36_part;
        one_div_diag_Q48 = inv_D[ i ].Q48_part;

        tmp_32 = T[ i ];
        T[ i ] = silk_ADD32( silk_SMMUL( tmp_32, one_div_diag_Q48 ), silk_RSHIFT( silk_SMULWW( tmp_32, one_div_diag_Q36 ), 4 ) );
    }
}


static inline void silk_LS_SolveFirst_FIX(
    const opus_int32    *L_Q16,     
    opus_int            M,          
    const opus_int32    *b,         
    opus_int32          *x_Q16      
)
{
    opus_int i, j;
    const opus_int32 *ptr32;
    opus_int32 tmp_32;

    for( i = 0; i < M; i++ ) {
        ptr32 = matrix_adr( L_Q16, i, 0, M );
        tmp_32 = 0;
        for( j = 0; j < i; j++ ) {
            tmp_32 = silk_SMLAWW( tmp_32, ptr32[ j ], x_Q16[ j ] );
        }
        x_Q16[ i ] = silk_SUB32( b[ i ], tmp_32 );
    }
}


static inline void silk_LS_SolveLast_FIX(
    const opus_int32    *L_Q16,     
    const opus_int      M,          
    const opus_int32    *b,         
    opus_int32          *x_Q16      
)
{
    opus_int i, j;
    const opus_int32 *ptr32;
    opus_int32 tmp_32;

    for( i = M - 1; i >= 0; i-- ) {
        ptr32 = matrix_adr( L_Q16, 0, i, M );
        tmp_32 = 0;
        for( j = M - 1; j > i; j-- ) {
            tmp_32 = silk_SMLAWW( tmp_32, ptr32[ silk_SMULBB( j, M ) ], x_Q16[ j ] );
        }
        x_Q16[ i ] = silk_SUB32( b[ i ], tmp_32 );
    }
}
