






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main_FLP.h"

void silk_LTP_analysis_filter_FLP(
    silk_float                      *LTP_res,                           
    const silk_float                *x,                                 
    const silk_float                B[ LTP_ORDER * MAX_NB_SUBFR ],      
    const opus_int                  pitchL[   MAX_NB_SUBFR ],           
    const silk_float                invGains[ MAX_NB_SUBFR ],           
    const opus_int                  subfr_length,                       
    const opus_int                  nb_subfr,                           
    const opus_int                  pre_length                          
)
{
    const silk_float *x_ptr, *x_lag_ptr;
    silk_float   Btmp[ LTP_ORDER ];
    silk_float   *LTP_res_ptr;
    silk_float   inv_gain;
    opus_int     k, i, j;

    x_ptr = x;
    LTP_res_ptr = LTP_res;
    for( k = 0; k < nb_subfr; k++ ) {
        x_lag_ptr = x_ptr - pitchL[ k ];
        inv_gain = invGains[ k ];
        for( i = 0; i < LTP_ORDER; i++ ) {
            Btmp[ i ] = B[ k * LTP_ORDER + i ];
        }

        
        for( i = 0; i < subfr_length + pre_length; i++ ) {
            LTP_res_ptr[ i ] = x_ptr[ i ];
            
            for( j = 0; j < LTP_ORDER; j++ ) {
                LTP_res_ptr[ i ] -= Btmp[ j ] * x_lag_ptr[ LTP_ORDER / 2 - j ];
            }
            LTP_res_ptr[ i ] *= inv_gain;
            x_lag_ptr++;
        }

        
        LTP_res_ptr += subfr_length + pre_length;
        x_ptr       += subfr_length;
    }
}
