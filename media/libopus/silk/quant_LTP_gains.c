






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main.h"

void silk_quant_LTP_gains(
    opus_int16                  B_Q14[ MAX_NB_SUBFR * LTP_ORDER ],          
    opus_int8                   cbk_index[ MAX_NB_SUBFR ],                  
    opus_int8                   *periodicity_index,                         
    const opus_int32            W_Q18[ MAX_NB_SUBFR*LTP_ORDER*LTP_ORDER ],  
    opus_int                    mu_Q9,                                      
    opus_int                    lowComplexity,                              
    const opus_int              nb_subfr                                    
)
{
    opus_int             j, k, cbk_size;
    opus_int8            temp_idx[ MAX_NB_SUBFR ];
    const opus_uint8     *cl_ptr_Q5;
    const opus_int8      *cbk_ptr_Q7;
    const opus_int16     *b_Q14_ptr;
    const opus_int32     *W_Q18_ptr;
    opus_int32           rate_dist_Q14_subfr, rate_dist_Q14, min_rate_dist_Q14;

    
    
    
    
    min_rate_dist_Q14 = silk_int32_MAX;
    for( k = 0; k < 3; k++ ) {
        cl_ptr_Q5  = silk_LTP_gain_BITS_Q5_ptrs[ k ];
        cbk_ptr_Q7 = silk_LTP_vq_ptrs_Q7[        k ];
        cbk_size   = silk_LTP_vq_sizes[          k ];

        
        W_Q18_ptr = W_Q18;
        b_Q14_ptr = B_Q14;

        rate_dist_Q14 = 0;
        for( j = 0; j < nb_subfr; j++ ) {
            silk_VQ_WMat_EC(
                &temp_idx[ j ],         
                &rate_dist_Q14_subfr,   
                b_Q14_ptr,              
                W_Q18_ptr,              
                cbk_ptr_Q7,             
                cl_ptr_Q5,              
                mu_Q9,                  
                cbk_size                
            );

            rate_dist_Q14 = silk_ADD_POS_SAT32( rate_dist_Q14, rate_dist_Q14_subfr );

            b_Q14_ptr += LTP_ORDER;
            W_Q18_ptr += LTP_ORDER * LTP_ORDER;
        }

        
        rate_dist_Q14 = silk_min( silk_int32_MAX - 1, rate_dist_Q14 );

        if( rate_dist_Q14 < min_rate_dist_Q14 ) {
            min_rate_dist_Q14 = rate_dist_Q14;
            *periodicity_index = (opus_int8)k;
            silk_memcpy( cbk_index, temp_idx, nb_subfr * sizeof( opus_int8 ) );
        }

        
        if( lowComplexity && ( rate_dist_Q14 < silk_LTP_gain_middle_avg_RD_Q14 ) ) {
            break;
        }
    }

    cbk_ptr_Q7 = silk_LTP_vq_ptrs_Q7[ *periodicity_index ];
    for( j = 0; j < nb_subfr; j++ ) {
        for( k = 0; k < LTP_ORDER; k++ ) {
            B_Q14[ j * LTP_ORDER + k ] = silk_LSHIFT( cbk_ptr_Q7[ cbk_index[ j ] * LTP_ORDER + k ], 7 );
        }
    }
}

