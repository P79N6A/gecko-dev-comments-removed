






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main_FIX.h"
#include "tuning_parameters.h"


#define LTP_CORRS_HEAD_ROOM                             2

void silk_fit_LTP(
    opus_int32 LTP_coefs_Q16[ LTP_ORDER ],
    opus_int16 LTP_coefs_Q14[ LTP_ORDER ]
);

void silk_find_LTP_FIX(
    opus_int16                      b_Q14[ MAX_NB_SUBFR * LTP_ORDER ],      
    opus_int32                      WLTP[ MAX_NB_SUBFR * LTP_ORDER * LTP_ORDER ], 
    opus_int                        *LTPredCodGain_Q7,                      
    const opus_int16                r_lpc[],                                
    const opus_int                  lag[ MAX_NB_SUBFR ],                    
    const opus_int32                Wght_Q15[ MAX_NB_SUBFR ],               
    const opus_int                  subfr_length,                           
    const opus_int                  nb_subfr,                               
    const opus_int                  mem_offset,                             
    opus_int                        corr_rshifts[ MAX_NB_SUBFR ]            
)
{
    opus_int   i, k, lshift;
    const opus_int16 *r_ptr, *lag_ptr;
    opus_int16 *b_Q14_ptr;

    opus_int32 regu;
    opus_int32 *WLTP_ptr;
    opus_int32 b_Q16[ LTP_ORDER ], delta_b_Q14[ LTP_ORDER ], d_Q14[ MAX_NB_SUBFR ], nrg[ MAX_NB_SUBFR ], g_Q26;
    opus_int32 w[ MAX_NB_SUBFR ], WLTP_max, max_abs_d_Q14, max_w_bits;

    opus_int32 temp32, denom32;
    opus_int   extra_shifts;
    opus_int   rr_shifts, maxRshifts, maxRshifts_wxtra, LZs;
    opus_int32 LPC_res_nrg, LPC_LTP_res_nrg, div_Q16;
    opus_int32 Rr[ LTP_ORDER ], rr[ MAX_NB_SUBFR ];
    opus_int32 wd, m_Q12;

    b_Q14_ptr = b_Q14;
    WLTP_ptr  = WLTP;
    r_ptr     = &r_lpc[ mem_offset ];
    for( k = 0; k < nb_subfr; k++ ) {
        lag_ptr = r_ptr - ( lag[ k ] + LTP_ORDER / 2 );

        silk_sum_sqr_shift( &rr[ k ], &rr_shifts, r_ptr, subfr_length ); 

        
        LZs = silk_CLZ32( rr[k] );
        if( LZs < LTP_CORRS_HEAD_ROOM ) {
            rr[ k ] = silk_RSHIFT_ROUND( rr[ k ], LTP_CORRS_HEAD_ROOM - LZs );
            rr_shifts += ( LTP_CORRS_HEAD_ROOM - LZs );
        }
        corr_rshifts[ k ] = rr_shifts;
        silk_corrMatrix_FIX( lag_ptr, subfr_length, LTP_ORDER, LTP_CORRS_HEAD_ROOM, WLTP_ptr, &corr_rshifts[ k ] );  

        
        silk_corrVector_FIX( lag_ptr, r_ptr, subfr_length, LTP_ORDER, Rr, corr_rshifts[ k ] );  
        if( corr_rshifts[ k ] > rr_shifts ) {
            rr[ k ] = silk_RSHIFT( rr[ k ], corr_rshifts[ k ] - rr_shifts ); 
        }
        silk_assert( rr[ k ] >= 0 );

        regu = 1;
        regu = silk_SMLAWB( regu, rr[ k ], SILK_FIX_CONST( LTP_DAMPING/3, 16 ) );
        regu = silk_SMLAWB( regu, matrix_ptr( WLTP_ptr, 0, 0, LTP_ORDER ), SILK_FIX_CONST( LTP_DAMPING/3, 16 ) );
        regu = silk_SMLAWB( regu, matrix_ptr( WLTP_ptr, LTP_ORDER-1, LTP_ORDER-1, LTP_ORDER ), SILK_FIX_CONST( LTP_DAMPING/3, 16 ) );
        silk_regularize_correlations_FIX( WLTP_ptr, &rr[k], regu, LTP_ORDER );

        silk_solve_LDL_FIX( WLTP_ptr, LTP_ORDER, Rr, b_Q16 ); 

        
        silk_fit_LTP( b_Q16, b_Q14_ptr );

        
        nrg[ k ] = silk_residual_energy16_covar_FIX( b_Q14_ptr, WLTP_ptr, Rr, rr[ k ], LTP_ORDER, 14 ); 

        
        extra_shifts = silk_min_int( corr_rshifts[ k ], LTP_CORRS_HEAD_ROOM );
        denom32 = silk_LSHIFT_SAT32( silk_SMULWB( nrg[ k ], Wght_Q15[ k ] ), 1 + extra_shifts ) + 
            silk_RSHIFT( silk_SMULWB( subfr_length, 655 ), corr_rshifts[ k ] - extra_shifts );    
        denom32 = silk_max( denom32, 1 );
        silk_assert( ((opus_int64)Wght_Q15[ k ] << 16 ) < silk_int32_MAX );                       
        temp32 = silk_DIV32( silk_LSHIFT( (opus_int32)Wght_Q15[ k ], 16 ), denom32 );             
        temp32 = silk_RSHIFT( temp32, 31 + corr_rshifts[ k ] - extra_shifts - 26 );               

        
        WLTP_max = 0;
        for( i = 0; i < LTP_ORDER * LTP_ORDER; i++ ) {
            WLTP_max = silk_max( WLTP_ptr[ i ], WLTP_max );
        }
        lshift = silk_CLZ32( WLTP_max ) - 1 - 3; 
        silk_assert( 26 - 18 + lshift >= 0 );
        if( 26 - 18 + lshift < 31 ) {
            temp32 = silk_min_32( temp32, silk_LSHIFT( (opus_int32)1, 26 - 18 + lshift ) );
        }

        silk_scale_vector32_Q26_lshift_18( WLTP_ptr, temp32, LTP_ORDER * LTP_ORDER ); 

        w[ k ] = matrix_ptr( WLTP_ptr, LTP_ORDER/2, LTP_ORDER/2, LTP_ORDER ); 
        silk_assert( w[k] >= 0 );

        r_ptr     += subfr_length;
        b_Q14_ptr += LTP_ORDER;
        WLTP_ptr  += LTP_ORDER * LTP_ORDER;
    }

    maxRshifts = 0;
    for( k = 0; k < nb_subfr; k++ ) {
        maxRshifts = silk_max_int( corr_rshifts[ k ], maxRshifts );
    }

    
    if( LTPredCodGain_Q7 != NULL ) {
        LPC_LTP_res_nrg = 0;
        LPC_res_nrg     = 0;
        silk_assert( LTP_CORRS_HEAD_ROOM >= 2 ); 
        for( k = 0; k < nb_subfr; k++ ) {
            LPC_res_nrg     = silk_ADD32( LPC_res_nrg,     silk_RSHIFT( silk_ADD32( silk_SMULWB(  rr[ k ], Wght_Q15[ k ] ), 1 ), 1 + ( maxRshifts - corr_rshifts[ k ] ) ) ); 
            LPC_LTP_res_nrg = silk_ADD32( LPC_LTP_res_nrg, silk_RSHIFT( silk_ADD32( silk_SMULWB( nrg[ k ], Wght_Q15[ k ] ), 1 ), 1 + ( maxRshifts - corr_rshifts[ k ] ) ) ); 
        }
        LPC_LTP_res_nrg = silk_max( LPC_LTP_res_nrg, 1 ); 

        div_Q16 = silk_DIV32_varQ( LPC_res_nrg, LPC_LTP_res_nrg, 16 );
        *LTPredCodGain_Q7 = ( opus_int )silk_SMULBB( 3, silk_lin2log( div_Q16 ) - ( 16 << 7 ) );

        silk_assert( *LTPredCodGain_Q7 == ( opus_int )silk_SAT16( silk_MUL( 3, silk_lin2log( div_Q16 ) - ( 16 << 7 ) ) ) );
    }

    
    
    b_Q14_ptr = b_Q14;
    for( k = 0; k < nb_subfr; k++ ) {
        d_Q14[ k ] = 0;
        for( i = 0; i < LTP_ORDER; i++ ) {
            d_Q14[ k ] += b_Q14_ptr[ i ];
        }
        b_Q14_ptr += LTP_ORDER;
    }

    

    
    max_abs_d_Q14 = 0;
    max_w_bits    = 0;
    for( k = 0; k < nb_subfr; k++ ) {
        max_abs_d_Q14 = silk_max_32( max_abs_d_Q14, silk_abs( d_Q14[ k ] ) );
        
        
        max_w_bits = silk_max_32( max_w_bits, 32 - silk_CLZ32( w[ k ] ) + corr_rshifts[ k ] - maxRshifts );
    }

    
    silk_assert( max_abs_d_Q14 <= ( 5 << 15 ) );

    
    extra_shifts = max_w_bits + 32 - silk_CLZ32( max_abs_d_Q14 ) - 14;

    
    extra_shifts -= ( 32 - 1 - 2 + maxRshifts ); 
    extra_shifts = silk_max_int( extra_shifts, 0 );

    maxRshifts_wxtra = maxRshifts + extra_shifts;

    temp32 = silk_RSHIFT( 262, maxRshifts + extra_shifts ) + 1; 
    wd = 0;
    for( k = 0; k < nb_subfr; k++ ) {
        
        temp32 = silk_ADD32( temp32,                     silk_RSHIFT( w[ k ], maxRshifts_wxtra - corr_rshifts[ k ] ) );                      
        wd     = silk_ADD32( wd, silk_LSHIFT( silk_SMULWW( silk_RSHIFT( w[ k ], maxRshifts_wxtra - corr_rshifts[ k ] ), d_Q14[ k ] ), 2 ) ); 
    }
    m_Q12 = silk_DIV32_varQ( wd, temp32, 12 );

    b_Q14_ptr = b_Q14;
    for( k = 0; k < nb_subfr; k++ ) {
        
        if( 2 - corr_rshifts[k] > 0 ) {
            temp32 = silk_RSHIFT( w[ k ], 2 - corr_rshifts[ k ] );
        } else {
            temp32 = silk_LSHIFT_SAT32( w[ k ], corr_rshifts[ k ] - 2 );
        }

        g_Q26 = silk_MUL(
            silk_DIV32(
                SILK_FIX_CONST( LTP_SMOOTHING, 26 ),
                silk_RSHIFT( SILK_FIX_CONST( LTP_SMOOTHING, 26 ), 10 ) + temp32 ),                          
            silk_LSHIFT_SAT32( silk_SUB_SAT32( (opus_int32)m_Q12, silk_RSHIFT( d_Q14[ k ], 2 ) ), 4 ) );    

        temp32 = 0;
        for( i = 0; i < LTP_ORDER; i++ ) {
            delta_b_Q14[ i ] = silk_max_16( b_Q14_ptr[ i ], 1638 );     
            temp32 += delta_b_Q14[ i ];                                 
        }
        temp32 = silk_DIV32( g_Q26, temp32 );                           
        for( i = 0; i < LTP_ORDER; i++ ) {
            b_Q14_ptr[ i ] = silk_LIMIT_32( (opus_int32)b_Q14_ptr[ i ] + silk_SMULWB( silk_LSHIFT_SAT32( temp32, 4 ), delta_b_Q14[ i ] ), -16000, 28000 );
        }
        b_Q14_ptr += LTP_ORDER;
    }
}

void silk_fit_LTP(
    opus_int32 LTP_coefs_Q16[ LTP_ORDER ],
    opus_int16 LTP_coefs_Q14[ LTP_ORDER ]
)
{
    opus_int i;

    for( i = 0; i < LTP_ORDER; i++ ) {
        LTP_coefs_Q14[ i ] = (opus_int16)silk_SAT16( silk_RSHIFT_ROUND( LTP_coefs_Q16[ i ], 2 ) );
    }
}
