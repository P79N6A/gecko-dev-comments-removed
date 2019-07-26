






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main.h"

#define OFFSET                  ( ( MIN_QGAIN_DB * 128 ) / 6 + 16 * 128 )
#define SCALE_Q16               ( ( 65536 * ( N_LEVELS_QGAIN - 1 ) ) / ( ( ( MAX_QGAIN_DB - MIN_QGAIN_DB ) * 128 ) / 6 ) )
#define INV_SCALE_Q16           ( ( 65536 * ( ( ( MAX_QGAIN_DB - MIN_QGAIN_DB ) * 128 ) / 6 ) ) / ( N_LEVELS_QGAIN - 1 ) )


void silk_gains_quant(
    opus_int8                   ind[ MAX_NB_SUBFR ],            
    opus_int32                  gain_Q16[ MAX_NB_SUBFR ],       
    opus_int8                   *prev_ind,                      
    const opus_int              conditional,                    
    const opus_int              nb_subfr                        
)
{
    opus_int k, double_step_size_threshold;

    for( k = 0; k < nb_subfr; k++ ) {
        
        ind[ k ] = silk_SMULWB( SCALE_Q16, silk_lin2log( gain_Q16[ k ] ) - OFFSET );

        
        if( ind[ k ] < *prev_ind ) {
            ind[ k ]++;
        }
        ind[ k ] = silk_LIMIT_int( ind[ k ], 0, N_LEVELS_QGAIN - 1 );

        
        if( k == 0 && conditional == 0 ) {
            
            ind[ k ] = silk_LIMIT_int( ind[ k ], *prev_ind + MIN_DELTA_GAIN_QUANT, N_LEVELS_QGAIN - 1 );
            *prev_ind = ind[ k ];
        } else {
            
            ind[ k ] = ind[ k ] - *prev_ind;

            
            double_step_size_threshold = 2 * MAX_DELTA_GAIN_QUANT - N_LEVELS_QGAIN + *prev_ind;
            if( ind[ k ] > double_step_size_threshold ) {
                ind[ k ] = double_step_size_threshold + silk_RSHIFT( ind[ k ] - double_step_size_threshold + 1, 1 );
            }

            ind[ k ] = silk_LIMIT_int( ind[ k ], MIN_DELTA_GAIN_QUANT, MAX_DELTA_GAIN_QUANT );

            
            if( ind[ k ] > double_step_size_threshold ) {
                *prev_ind += silk_LSHIFT( ind[ k ], 1 ) - double_step_size_threshold;
            } else {
                *prev_ind += ind[ k ];
            }

            
            ind[ k ] -= MIN_DELTA_GAIN_QUANT;
        }

        
        gain_Q16[ k ] = silk_log2lin( silk_min_32( silk_SMULWB( INV_SCALE_Q16, *prev_ind ) + OFFSET, 3967 ) ); 
    }
}


void silk_gains_dequant(
    opus_int32                  gain_Q16[ MAX_NB_SUBFR ],       
    const opus_int8             ind[ MAX_NB_SUBFR ],            
    opus_int8                   *prev_ind,                      
    const opus_int              conditional,                    
    const opus_int              nb_subfr                        
)
{
    opus_int   k, ind_tmp, double_step_size_threshold;

    for( k = 0; k < nb_subfr; k++ ) {
        if( k == 0 && conditional == 0 ) {
            
            *prev_ind = silk_max_int( ind[ k ], *prev_ind - 16 );
        } else {
            
            ind_tmp = ind[ k ] + MIN_DELTA_GAIN_QUANT;

            
            double_step_size_threshold = 2 * MAX_DELTA_GAIN_QUANT - N_LEVELS_QGAIN + *prev_ind;
            if( ind_tmp > double_step_size_threshold ) {
                *prev_ind += silk_LSHIFT( ind_tmp, 1 ) - double_step_size_threshold;
            } else {
                *prev_ind += ind_tmp;
            }
        }
        *prev_ind = silk_LIMIT_int( *prev_ind, 0, N_LEVELS_QGAIN - 1 );

        
        gain_Q16[ k ] = silk_log2lin( silk_min_32( silk_SMULWB( INV_SCALE_Q16, *prev_ind ) + OFFSET, 3967 ) ); 
    }
}


opus_int32 silk_gains_ID(                                       
    const opus_int8             ind[ MAX_NB_SUBFR ],            
    const opus_int              nb_subfr                        
)
{
    opus_int   k;
    opus_int32 gainsID;

    gainsID = 0;
    for( k = 0; k < nb_subfr; k++ ) {
        gainsID = silk_ADD_LSHIFT32( ind[ k ], gainsID, 8 );
    }

    return gainsID;
}
