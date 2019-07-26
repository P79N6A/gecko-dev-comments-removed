






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main_FLP.h"
#include "tuning_parameters.h"

void silk_find_LTP_FLP(
    silk_float                      b[ MAX_NB_SUBFR * LTP_ORDER ],      
    silk_float                      WLTP[ MAX_NB_SUBFR * LTP_ORDER * LTP_ORDER ], 
    silk_float                      *LTPredCodGain,                     
    const silk_float                r_lpc[],                            
    const opus_int                  lag[  MAX_NB_SUBFR ],               
    const silk_float                Wght[ MAX_NB_SUBFR ],               
    const opus_int                  subfr_length,                       
    const opus_int                  nb_subfr,                           
    const opus_int                  mem_offset                          
)
{
    opus_int   i, k;
    silk_float *b_ptr, temp, *WLTP_ptr;
    silk_float LPC_res_nrg, LPC_LTP_res_nrg;
    silk_float d[ MAX_NB_SUBFR ], m, g, delta_b[ LTP_ORDER ];
    silk_float w[ MAX_NB_SUBFR ], nrg[ MAX_NB_SUBFR ], regu;
    silk_float Rr[ LTP_ORDER ], rr[ MAX_NB_SUBFR ];
    const silk_float *r_ptr, *lag_ptr;

    b_ptr    = b;
    WLTP_ptr = WLTP;
    r_ptr    = &r_lpc[ mem_offset ];
    for( k = 0; k < nb_subfr; k++ ) {
        lag_ptr = r_ptr - ( lag[ k ] + LTP_ORDER / 2 );

        silk_corrMatrix_FLP( lag_ptr, subfr_length, LTP_ORDER, WLTP_ptr );
        silk_corrVector_FLP( lag_ptr, r_ptr, subfr_length, LTP_ORDER, Rr );

        rr[ k ] = ( silk_float )silk_energy_FLP( r_ptr, subfr_length );
        regu = 1.0f + rr[ k ] +
            matrix_ptr( WLTP_ptr, 0, 0, LTP_ORDER ) +
            matrix_ptr( WLTP_ptr, LTP_ORDER-1, LTP_ORDER-1, LTP_ORDER );
        regu *= LTP_DAMPING / 3;
        silk_regularize_correlations_FLP( WLTP_ptr, &rr[ k ], regu, LTP_ORDER );
        silk_solve_LDL_FLP( WLTP_ptr, LTP_ORDER, Rr, b_ptr );

        
        nrg[ k ] = silk_residual_energy_covar_FLP( b_ptr, WLTP_ptr, Rr, rr[ k ], LTP_ORDER );

        temp = Wght[ k ] / ( nrg[ k ] * Wght[ k ] + 0.01f * subfr_length );
        silk_scale_vector_FLP( WLTP_ptr, temp, LTP_ORDER * LTP_ORDER );
        w[ k ] = matrix_ptr( WLTP_ptr, LTP_ORDER / 2, LTP_ORDER / 2, LTP_ORDER );

        r_ptr    += subfr_length;
        b_ptr    += LTP_ORDER;
        WLTP_ptr += LTP_ORDER * LTP_ORDER;
    }

    
    if( LTPredCodGain != NULL ) {
        LPC_LTP_res_nrg = 1e-6f;
        LPC_res_nrg     = 0.0f;
        for( k = 0; k < nb_subfr; k++ ) {
            LPC_res_nrg     += rr[  k ] * Wght[ k ];
            LPC_LTP_res_nrg += nrg[ k ] * Wght[ k ];
        }

        silk_assert( LPC_LTP_res_nrg > 0 );
        *LTPredCodGain = 3.0f * silk_log2( LPC_res_nrg / LPC_LTP_res_nrg );
    }

    
    
    b_ptr = b;
    for( k = 0; k < nb_subfr; k++ ) {
        d[ k ] = 0;
        for( i = 0; i < LTP_ORDER; i++ ) {
            d[ k ] += b_ptr[ i ];
        }
        b_ptr += LTP_ORDER;
    }
    
    temp = 1e-3f;
    for( k = 0; k < nb_subfr; k++ ) {
        temp += w[ k ];
    }
    m = 0;
    for( k = 0; k < nb_subfr; k++ ) {
        m += d[ k ] * w[ k ];
    }
    m = m / temp;

    b_ptr = b;
    for( k = 0; k < nb_subfr; k++ ) {
        g = LTP_SMOOTHING / ( LTP_SMOOTHING + w[ k ] ) * ( m - d[ k ] );
        temp = 0;
        for( i = 0; i < LTP_ORDER; i++ ) {
            delta_b[ i ] = silk_max_float( b_ptr[ i ], 0.1f );
            temp += delta_b[ i ];
        }
        temp = g / temp;
        for( i = 0; i < LTP_ORDER; i++ ) {
            b_ptr[ i ] = b_ptr[ i ] + delta_b[ i ] * temp;
        }
        b_ptr += LTP_ORDER;
    }
}
