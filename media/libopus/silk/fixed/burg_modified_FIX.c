






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "SigProc_FIX.h"
#include "define.h"
#include "tuning_parameters.h"

#define MAX_FRAME_SIZE              384             /* subfr_length * nb_subfr = ( 0.005 * 16000 + 16 ) * 4 = 384 */

#define QA                          25
#define N_BITS_HEAD_ROOM            2
#define MIN_RSHIFTS                 -16
#define MAX_RSHIFTS                 (32 - QA)


void silk_burg_modified(
    opus_int32                  *res_nrg,           
    opus_int                    *res_nrg_Q,         
    opus_int32                  A_Q16[],            
    const opus_int16            x[],                
    const opus_int32            minInvGain_Q30,     
    const opus_int              subfr_length,       
    const opus_int              nb_subfr,           
    const opus_int              D                   
)
{
    opus_int         k, n, s, lz, rshifts, rshifts_extra, reached_max_gain;
    opus_int32       C0, num, nrg, rc_Q31, invGain_Q30, Atmp_QA, Atmp1, tmp1, tmp2, x1, x2;
    const opus_int16 *x_ptr;
    opus_int32       C_first_row[ SILK_MAX_ORDER_LPC ];
    opus_int32       C_last_row[  SILK_MAX_ORDER_LPC ];
    opus_int32       Af_QA[       SILK_MAX_ORDER_LPC ];
    opus_int32       CAf[ SILK_MAX_ORDER_LPC + 1 ];
    opus_int32       CAb[ SILK_MAX_ORDER_LPC + 1 ];

    silk_assert( subfr_length * nb_subfr <= MAX_FRAME_SIZE );

    
    silk_sum_sqr_shift( &C0, &rshifts, x, nb_subfr * subfr_length );
    if( rshifts > MAX_RSHIFTS ) {
        C0 = silk_LSHIFT32( C0, rshifts - MAX_RSHIFTS );
        silk_assert( C0 > 0 );
        rshifts = MAX_RSHIFTS;
    } else {
        lz = silk_CLZ32( C0 ) - 1;
        rshifts_extra = N_BITS_HEAD_ROOM - lz;
        if( rshifts_extra > 0 ) {
            rshifts_extra = silk_min( rshifts_extra, MAX_RSHIFTS - rshifts );
            C0 = silk_RSHIFT32( C0, rshifts_extra );
        } else {
            rshifts_extra = silk_max( rshifts_extra, MIN_RSHIFTS - rshifts );
            C0 = silk_LSHIFT32( C0, -rshifts_extra );
        }
        rshifts += rshifts_extra;
    }
    CAb[ 0 ] = CAf[ 0 ] = C0 + silk_SMMUL( SILK_FIX_CONST( FIND_LPC_COND_FAC, 32 ), C0 ) + 1;                                
    silk_memset( C_first_row, 0, SILK_MAX_ORDER_LPC * sizeof( opus_int32 ) );
    if( rshifts > 0 ) {
        for( s = 0; s < nb_subfr; s++ ) {
            x_ptr = x + s * subfr_length;
            for( n = 1; n < D + 1; n++ ) {
                C_first_row[ n - 1 ] += (opus_int32)silk_RSHIFT64(
                    silk_inner_prod16_aligned_64( x_ptr, x_ptr + n, subfr_length - n ), rshifts );
            }
        }
    } else {
        for( s = 0; s < nb_subfr; s++ ) {
            x_ptr = x + s * subfr_length;
            for( n = 1; n < D + 1; n++ ) {
                C_first_row[ n - 1 ] += silk_LSHIFT32(
                    silk_inner_prod_aligned( x_ptr, x_ptr + n, subfr_length - n ), -rshifts );
            }
        }
    }
    silk_memcpy( C_last_row, C_first_row, SILK_MAX_ORDER_LPC * sizeof( opus_int32 ) );

    
    CAb[ 0 ] = CAf[ 0 ] = C0 + silk_SMMUL( SILK_FIX_CONST( FIND_LPC_COND_FAC, 32 ), C0 ) + 1;                                

    invGain_Q30 = 1 << 30;
    reached_max_gain = 0;
    for( n = 0; n < D; n++ ) {
        
        
        
        
        if( rshifts > -2 ) {
            for( s = 0; s < nb_subfr; s++ ) {
                x_ptr = x + s * subfr_length;
                x1  = -silk_LSHIFT32( (opus_int32)x_ptr[ n ],                    16 - rshifts );        
                x2  = -silk_LSHIFT32( (opus_int32)x_ptr[ subfr_length - n - 1 ], 16 - rshifts );        
                tmp1 = silk_LSHIFT32( (opus_int32)x_ptr[ n ],                    QA - 16 );             
                tmp2 = silk_LSHIFT32( (opus_int32)x_ptr[ subfr_length - n - 1 ], QA - 16 );             
                for( k = 0; k < n; k++ ) {
                    C_first_row[ k ] = silk_SMLAWB( C_first_row[ k ], x1, x_ptr[ n - k - 1 ]            ); 
                    C_last_row[ k ]  = silk_SMLAWB( C_last_row[ k ],  x2, x_ptr[ subfr_length - n + k ] ); 
                    Atmp_QA = Af_QA[ k ];
                    tmp1 = silk_SMLAWB( tmp1, Atmp_QA, x_ptr[ n - k - 1 ]            );                 
                    tmp2 = silk_SMLAWB( tmp2, Atmp_QA, x_ptr[ subfr_length - n + k ] );                 
                }
                tmp1 = silk_LSHIFT32( -tmp1, 32 - QA - rshifts );                                       
                tmp2 = silk_LSHIFT32( -tmp2, 32 - QA - rshifts );                                       
                for( k = 0; k <= n; k++ ) {
                    CAf[ k ] = silk_SMLAWB( CAf[ k ], tmp1, x_ptr[ n - k ]                    );        
                    CAb[ k ] = silk_SMLAWB( CAb[ k ], tmp2, x_ptr[ subfr_length - n + k - 1 ] );        
                }
            }
        } else {
            for( s = 0; s < nb_subfr; s++ ) {
                x_ptr = x + s * subfr_length;
                x1  = -silk_LSHIFT32( (opus_int32)x_ptr[ n ],                    -rshifts );            
                x2  = -silk_LSHIFT32( (opus_int32)x_ptr[ subfr_length - n - 1 ], -rshifts );            
                tmp1 = silk_LSHIFT32( (opus_int32)x_ptr[ n ],                    17 );                  
                tmp2 = silk_LSHIFT32( (opus_int32)x_ptr[ subfr_length - n - 1 ], 17 );                  
                for( k = 0; k < n; k++ ) {
                    C_first_row[ k ] = silk_MLA( C_first_row[ k ], x1, x_ptr[ n - k - 1 ]            ); 
                    C_last_row[ k ]  = silk_MLA( C_last_row[ k ],  x2, x_ptr[ subfr_length - n + k ] ); 
                    Atmp1 = silk_RSHIFT_ROUND( Af_QA[ k ], QA - 17 );                                   
                    tmp1 = silk_MLA( tmp1, x_ptr[ n - k - 1 ],            Atmp1 );                      
                    tmp2 = silk_MLA( tmp2, x_ptr[ subfr_length - n + k ], Atmp1 );                      
                }
                tmp1 = -tmp1;                                                                           
                tmp2 = -tmp2;                                                                           
                for( k = 0; k <= n; k++ ) {
                    CAf[ k ] = silk_SMLAWW( CAf[ k ], tmp1,
                        silk_LSHIFT32( (opus_int32)x_ptr[ n - k ], -rshifts - 1 ) );                    
                    CAb[ k ] = silk_SMLAWW( CAb[ k ], tmp2,
                        silk_LSHIFT32( (opus_int32)x_ptr[ subfr_length - n + k - 1 ], -rshifts - 1 ) ); 
                }
            }
        }

        
        tmp1 = C_first_row[ n ];                                                                        
        tmp2 = C_last_row[ n ];                                                                         
        num  = 0;                                                                                       
        nrg  = silk_ADD32( CAb[ 0 ], CAf[ 0 ] );                                                        
        for( k = 0; k < n; k++ ) {
            Atmp_QA = Af_QA[ k ];
            lz = silk_CLZ32( silk_abs( Atmp_QA ) ) - 1;
            lz = silk_min( 32 - QA, lz );
            Atmp1 = silk_LSHIFT32( Atmp_QA, lz );                                                       

            tmp1 = silk_ADD_LSHIFT32( tmp1, silk_SMMUL( C_last_row[  n - k - 1 ], Atmp1 ), 32 - QA - lz );  
            tmp2 = silk_ADD_LSHIFT32( tmp2, silk_SMMUL( C_first_row[ n - k - 1 ], Atmp1 ), 32 - QA - lz );  
            num  = silk_ADD_LSHIFT32( num,  silk_SMMUL( CAb[ n - k ],             Atmp1 ), 32 - QA - lz );  
            nrg  = silk_ADD_LSHIFT32( nrg,  silk_SMMUL( silk_ADD32( CAb[ k + 1 ], CAf[ k + 1 ] ),
                                                                                Atmp1 ), 32 - QA - lz );    
        }
        CAf[ n + 1 ] = tmp1;                                                                            
        CAb[ n + 1 ] = tmp2;                                                                            
        num = silk_ADD32( num, tmp2 );                                                                  
        num = silk_LSHIFT32( -num, 1 );                                                                 

        
        if( silk_abs( num ) < nrg ) {
            rc_Q31 = silk_DIV32_varQ( num, nrg, 31 );
        } else {
            rc_Q31 = ( num > 0 ) ? silk_int32_MAX : silk_int32_MIN;
        }

        
        tmp1 = ( (opus_int32)1 << 30 ) - silk_SMMUL( rc_Q31, rc_Q31 );
        tmp1 = silk_LSHIFT( silk_SMMUL( invGain_Q30, tmp1 ), 2 );
        if( tmp1 <= minInvGain_Q30 ) {
            
            tmp2 = ( 1 << 30 ) - silk_DIV32_varQ( minInvGain_Q30, invGain_Q30, 30 );            
            rc_Q31 = silk_SQRT_APPROX( tmp2 );                                                  
            
            rc_Q31 = silk_RSHIFT32( rc_Q31 + silk_DIV32( tmp2, rc_Q31 ), 1 );                   
            rc_Q31 = silk_LSHIFT32( rc_Q31, 16 );                                               
            if( num < 0 ) {
                
                rc_Q31 = -rc_Q31;
            }
            invGain_Q30 = minInvGain_Q30;
            reached_max_gain = 1;
        } else {
            invGain_Q30 = tmp1;
        }

        
        for( k = 0; k < (n + 1) >> 1; k++ ) {
            tmp1 = Af_QA[ k ];                                                                  
            tmp2 = Af_QA[ n - k - 1 ];                                                          
            Af_QA[ k ]         = silk_ADD_LSHIFT32( tmp1, silk_SMMUL( tmp2, rc_Q31 ), 1 );      
            Af_QA[ n - k - 1 ] = silk_ADD_LSHIFT32( tmp2, silk_SMMUL( tmp1, rc_Q31 ), 1 );      
        }
        Af_QA[ n ] = silk_RSHIFT32( rc_Q31, 31 - QA );                                          

        if( reached_max_gain ) {
            
            for( k = n + 1; k < D; k++ ) {
                Af_QA[ k ] = 0;
            }
            break;
        }

        
        for( k = 0; k <= n + 1; k++ ) {
            tmp1 = CAf[ k ];                                                                    
            tmp2 = CAb[ n - k + 1 ];                                                            
            CAf[ k ]         = silk_ADD_LSHIFT32( tmp1, silk_SMMUL( tmp2, rc_Q31 ), 1 );        
            CAb[ n - k + 1 ] = silk_ADD_LSHIFT32( tmp2, silk_SMMUL( tmp1, rc_Q31 ), 1 );        
        }
    }

    if( reached_max_gain ) {
        for( k = 0; k < D; k++ ) {
            
            A_Q16[ k ] = -silk_RSHIFT_ROUND( Af_QA[ k ], QA - 16 );
        }
        
        if( rshifts > 0 ) {
            for( s = 0; s < nb_subfr; s++ ) {
                x_ptr = x + s * subfr_length;
                C0 -= (opus_int32)silk_RSHIFT64( silk_inner_prod16_aligned_64( x_ptr, x_ptr, D ), rshifts );
            }
        } else {
            for( s = 0; s < nb_subfr; s++ ) {
                x_ptr = x + s * subfr_length;
                C0 -= silk_LSHIFT32( silk_inner_prod_aligned( x_ptr, x_ptr, D ), -rshifts );
            }
        }
        
        *res_nrg = silk_LSHIFT( silk_SMMUL( invGain_Q30, C0 ), 2 );
        *res_nrg_Q = -rshifts;
    } else {
        
        nrg  = CAf[ 0 ];                                                                            
        tmp1 = 1 << 16;                                                                             
        for( k = 0; k < D; k++ ) {
            Atmp1 = silk_RSHIFT_ROUND( Af_QA[ k ], QA - 16 );                                       
            nrg  = silk_SMLAWW( nrg, CAf[ k + 1 ], Atmp1 );                                         
            tmp1 = silk_SMLAWW( tmp1, Atmp1, Atmp1 );                                               
            A_Q16[ k ] = -Atmp1;
        }
        *res_nrg = silk_SMLAWW( nrg, silk_SMMUL( FIND_LPC_COND_FAC, C0 ), -tmp1 );                  
        *res_nrg_Q = -rshifts;
    }
}
