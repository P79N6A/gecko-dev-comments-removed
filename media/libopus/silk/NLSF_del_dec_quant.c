






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main.h"


opus_int32 silk_NLSF_del_dec_quant(                             
    opus_int8                   indices[],                      
    const opus_int16            x_Q10[],                        
    const opus_int16            w_Q5[],                         
    const opus_uint8            pred_coef_Q8[],                 
    const opus_int16            ec_ix[],                        
    const opus_uint8            ec_rates_Q5[],                  
    const opus_int              quant_step_size_Q16,            
    const opus_int16            inv_quant_step_size_Q6,         
    const opus_int32            mu_Q20,                         
    const opus_int16            order                           
)
{
    opus_int         i, j, nStates, ind_tmp, ind_min_max, ind_max_min, in_Q10, res_Q10;
    opus_int         pred_Q10, diff_Q10, out0_Q10, out1_Q10, rate0_Q5, rate1_Q5;
    opus_int32       RD_tmp_Q25, min_Q25, min_max_Q25, max_min_Q25, pred_coef_Q16;
    opus_int         ind_sort[         NLSF_QUANT_DEL_DEC_STATES ];
    opus_int8        ind[              NLSF_QUANT_DEL_DEC_STATES ][ MAX_LPC_ORDER ];
    opus_int16       prev_out_Q10[ 2 * NLSF_QUANT_DEL_DEC_STATES ];
    opus_int32       RD_Q25[       2 * NLSF_QUANT_DEL_DEC_STATES ];
    opus_int32       RD_min_Q25[       NLSF_QUANT_DEL_DEC_STATES ];
    opus_int32       RD_max_Q25[       NLSF_QUANT_DEL_DEC_STATES ];
    const opus_uint8 *rates_Q5;

    silk_assert( (NLSF_QUANT_DEL_DEC_STATES & (NLSF_QUANT_DEL_DEC_STATES-1)) == 0 );     

    nStates = 1;
    RD_Q25[ 0 ] = 0;
    prev_out_Q10[ 0 ] = 0;
    for( i = order - 1; ; i-- ) {
        rates_Q5 = &ec_rates_Q5[ ec_ix[ i ] ];
        pred_coef_Q16 = silk_LSHIFT( (opus_int32)pred_coef_Q8[ i ], 8 );
        in_Q10 = x_Q10[ i ];
        for( j = 0; j < nStates; j++ ) {
            pred_Q10 = silk_SMULWB( pred_coef_Q16, prev_out_Q10[ j ] );
            res_Q10  = silk_SUB16( in_Q10, pred_Q10 );
            ind_tmp  = silk_SMULWB( inv_quant_step_size_Q6, res_Q10 );
            ind_tmp  = silk_LIMIT( ind_tmp, -NLSF_QUANT_MAX_AMPLITUDE_EXT, NLSF_QUANT_MAX_AMPLITUDE_EXT-1 );
            ind[ j ][ i ] = (opus_int8)ind_tmp;

            
            out0_Q10 = silk_LSHIFT( ind_tmp, 10 );
            out1_Q10 = silk_ADD16( out0_Q10, 1024 );
            if( ind_tmp > 0 ) {
                out0_Q10 = silk_SUB16( out0_Q10, SILK_FIX_CONST( NLSF_QUANT_LEVEL_ADJ, 10 ) );
                out1_Q10 = silk_SUB16( out1_Q10, SILK_FIX_CONST( NLSF_QUANT_LEVEL_ADJ, 10 ) );
            } else if( ind_tmp == 0 ) {
                out1_Q10 = silk_SUB16( out1_Q10, SILK_FIX_CONST( NLSF_QUANT_LEVEL_ADJ, 10 ) );
            } else if( ind_tmp == -1 ) {
                out0_Q10 = silk_ADD16( out0_Q10, SILK_FIX_CONST( NLSF_QUANT_LEVEL_ADJ, 10 ) );
            } else {
                out0_Q10 = silk_ADD16( out0_Q10, SILK_FIX_CONST( NLSF_QUANT_LEVEL_ADJ, 10 ) );
                out1_Q10 = silk_ADD16( out1_Q10, SILK_FIX_CONST( NLSF_QUANT_LEVEL_ADJ, 10 ) );
            }
            out0_Q10  = silk_SMULWB( out0_Q10, quant_step_size_Q16 );
            out1_Q10  = silk_SMULWB( out1_Q10, quant_step_size_Q16 );
            out0_Q10  = silk_ADD16( out0_Q10, pred_Q10 );
            out1_Q10  = silk_ADD16( out1_Q10, pred_Q10 );
            prev_out_Q10[ j           ] = out0_Q10;
            prev_out_Q10[ j + nStates ] = out1_Q10;

            
            if( ind_tmp + 1 >= NLSF_QUANT_MAX_AMPLITUDE ) {
                if( ind_tmp + 1 == NLSF_QUANT_MAX_AMPLITUDE ) {
                    rate0_Q5 = rates_Q5[ ind_tmp + NLSF_QUANT_MAX_AMPLITUDE ];
                    rate1_Q5 = 280;
                } else {
                    rate0_Q5 = silk_SMLABB( 280 - 43 * NLSF_QUANT_MAX_AMPLITUDE, 43, ind_tmp );
                    rate1_Q5 = silk_ADD16( rate0_Q5, 43 );
                }
            } else if( ind_tmp <= -NLSF_QUANT_MAX_AMPLITUDE ) {
                if( ind_tmp == -NLSF_QUANT_MAX_AMPLITUDE ) {
                    rate0_Q5 = 280;
                    rate1_Q5 = rates_Q5[ ind_tmp + 1 + NLSF_QUANT_MAX_AMPLITUDE ];
                } else {
                    rate0_Q5 = silk_SMLABB( 280 - 43 * NLSF_QUANT_MAX_AMPLITUDE, -43, ind_tmp );
                    rate1_Q5 = silk_SUB16( rate0_Q5, 43 );
                }
            } else {
                rate0_Q5 = rates_Q5[ ind_tmp +     NLSF_QUANT_MAX_AMPLITUDE ];
                rate1_Q5 = rates_Q5[ ind_tmp + 1 + NLSF_QUANT_MAX_AMPLITUDE ];
            }
            RD_tmp_Q25            = RD_Q25[ j ];
            diff_Q10              = silk_SUB16( in_Q10, out0_Q10 );
            RD_Q25[ j ]           = silk_SMLABB( silk_MLA( RD_tmp_Q25, silk_SMULBB( diff_Q10, diff_Q10 ), w_Q5[ i ] ), mu_Q20, rate0_Q5 );
            diff_Q10              = silk_SUB16( in_Q10, out1_Q10 );
            RD_Q25[ j + nStates ] = silk_SMLABB( silk_MLA( RD_tmp_Q25, silk_SMULBB( diff_Q10, diff_Q10 ), w_Q5[ i ] ), mu_Q20, rate1_Q5 );
        }

        if( nStates < NLSF_QUANT_DEL_DEC_STATES ) {
            
            for( j = 0; j < nStates; j++ ) {
                ind[ j + nStates ][ i ] = ind[ j ][ i ] + 1;
            }
            nStates = silk_LSHIFT( nStates, 1 );
            for( j = nStates; j < NLSF_QUANT_DEL_DEC_STATES; j++ ) {
                ind[ j ][ i ] = ind[ j - nStates ][ i ];
            }
        } else if( i > 0 ) {
            
            for( j = 0; j < NLSF_QUANT_DEL_DEC_STATES; j++ ) {
                if( RD_Q25[ j ] > RD_Q25[ j + NLSF_QUANT_DEL_DEC_STATES ] ) {
                    RD_max_Q25[ j ]                         = RD_Q25[ j ];
                    RD_min_Q25[ j ]                         = RD_Q25[ j + NLSF_QUANT_DEL_DEC_STATES ];
                    RD_Q25[ j ]                             = RD_min_Q25[ j ];
                    RD_Q25[ j + NLSF_QUANT_DEL_DEC_STATES ] = RD_max_Q25[ j ];
                    
                    out0_Q10 = prev_out_Q10[ j ];
                    prev_out_Q10[ j ] = prev_out_Q10[ j + NLSF_QUANT_DEL_DEC_STATES ];
                    prev_out_Q10[ j + NLSF_QUANT_DEL_DEC_STATES ] = out0_Q10;
                    ind_sort[ j ] = j + NLSF_QUANT_DEL_DEC_STATES;
                } else {
                    RD_min_Q25[ j ] = RD_Q25[ j ];
                    RD_max_Q25[ j ] = RD_Q25[ j + NLSF_QUANT_DEL_DEC_STATES ];
                    ind_sort[ j ] = j;
                }
            }
            
            
            while( 1 ) {
                min_max_Q25 = silk_int32_MAX;
                max_min_Q25 = 0;
                ind_min_max = 0;
                ind_max_min = 0;
                for( j = 0; j < NLSF_QUANT_DEL_DEC_STATES; j++ ) {
                    if( min_max_Q25 > RD_max_Q25[ j ] ) {
                        min_max_Q25 = RD_max_Q25[ j ];
                        ind_min_max = j;
                    }
                    if( max_min_Q25 < RD_min_Q25[ j ] ) {
                        max_min_Q25 = RD_min_Q25[ j ];
                        ind_max_min = j;
                    }
                }
                if( min_max_Q25 >= max_min_Q25 ) {
                    break;
                }
                
                ind_sort[     ind_max_min ] = ind_sort[     ind_min_max ] ^ NLSF_QUANT_DEL_DEC_STATES;
                RD_Q25[       ind_max_min ] = RD_Q25[       ind_min_max + NLSF_QUANT_DEL_DEC_STATES ];
                prev_out_Q10[ ind_max_min ] = prev_out_Q10[ ind_min_max + NLSF_QUANT_DEL_DEC_STATES ];
                RD_min_Q25[   ind_max_min ] = 0;
                RD_max_Q25[   ind_min_max ] = silk_int32_MAX;
                silk_memcpy( ind[ ind_max_min ], ind[ ind_min_max ], MAX_LPC_ORDER * sizeof( opus_int8 ) );
            }
            
            for( j = 0; j < NLSF_QUANT_DEL_DEC_STATES; j++ ) {
                ind[ j ][ i ] += silk_RSHIFT( ind_sort[ j ], NLSF_QUANT_DEL_DEC_STATES_LOG2 );
            }
        } else {  
            break;
        }
    }

    
    ind_tmp = 0;
    min_Q25 = silk_int32_MAX;
    for( j = 0; j < 2 * NLSF_QUANT_DEL_DEC_STATES; j++ ) {
        if( min_Q25 > RD_Q25[ j ] ) {
            min_Q25 = RD_Q25[ j ];
            ind_tmp = j;
        }
    }
    for( j = 0; j < order; j++ ) {
        indices[ j ] = ind[ ind_tmp & ( NLSF_QUANT_DEL_DEC_STATES - 1 ) ][ j ];
        silk_assert( indices[ j ] >= -NLSF_QUANT_MAX_AMPLITUDE_EXT );
        silk_assert( indices[ j ] <=  NLSF_QUANT_MAX_AMPLITUDE_EXT );
    }
    indices[ 0 ] += silk_RSHIFT( ind_tmp, NLSF_QUANT_DEL_DEC_STATES_LOG2 );
    silk_assert( indices[ 0 ] <= NLSF_QUANT_MAX_AMPLITUDE_EXT );
    silk_assert( min_Q25 >= 0 );
    return min_Q25;
}
