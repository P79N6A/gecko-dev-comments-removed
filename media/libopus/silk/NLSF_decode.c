






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main.h"


static inline void silk_NLSF_residual_dequant(               
          opus_int16         x_Q10[],                        
    const opus_int8          indices[],                      
    const opus_uint8         pred_coef_Q8[],                 
    const opus_int           quant_step_size_Q16,            
    const opus_int16         order                           
)
{
    opus_int     i, out_Q10, pred_Q10;

    out_Q10 = 0;
    for( i = order-1; i >= 0; i-- ) {
        pred_Q10 = silk_RSHIFT( silk_SMULBB( out_Q10, (opus_int16)pred_coef_Q8[ i ] ), 8 );
        out_Q10  = silk_LSHIFT( indices[ i ], 10 );
        if( out_Q10 > 0 ) {
            out_Q10 = silk_SUB16( out_Q10, SILK_FIX_CONST( NLSF_QUANT_LEVEL_ADJ, 10 ) );
        } else if( out_Q10 < 0 ) {
            out_Q10 = silk_ADD16( out_Q10, SILK_FIX_CONST( NLSF_QUANT_LEVEL_ADJ, 10 ) );
        }
        out_Q10  = silk_SMLAWB( pred_Q10, out_Q10, quant_step_size_Q16 );
        x_Q10[ i ] = out_Q10;
    }
}





void silk_NLSF_decode(
          opus_int16            *pNLSF_Q15,                     
          opus_int8             *NLSFIndices,                   
    const silk_NLSF_CB_struct   *psNLSF_CB                      
)
{
    opus_int         i;
    opus_uint8       pred_Q8[  MAX_LPC_ORDER ];
    opus_int16       ec_ix[    MAX_LPC_ORDER ];
    opus_int16       res_Q10[  MAX_LPC_ORDER ];
    opus_int16       W_tmp_QW[ MAX_LPC_ORDER ];
    opus_int32       W_tmp_Q9, NLSF_Q15_tmp;
    const opus_uint8 *pCB_element;

    
    pCB_element = &psNLSF_CB->CB1_NLSF_Q8[ NLSFIndices[ 0 ] * psNLSF_CB->order ];
    for( i = 0; i < psNLSF_CB->order; i++ ) {
        pNLSF_Q15[ i ] = silk_LSHIFT( (opus_int16)pCB_element[ i ], 7 );
    }

    
    silk_NLSF_unpack( ec_ix, pred_Q8, psNLSF_CB, NLSFIndices[ 0 ] );

    
    silk_NLSF_residual_dequant( res_Q10, &NLSFIndices[ 1 ], pred_Q8, psNLSF_CB->quantStepSize_Q16, psNLSF_CB->order );

    
    silk_NLSF_VQ_weights_laroia( W_tmp_QW, pNLSF_Q15, psNLSF_CB->order );

    
    for( i = 0; i < psNLSF_CB->order; i++ ) {
        W_tmp_Q9 = silk_SQRT_APPROX( silk_LSHIFT( (opus_int32)W_tmp_QW[ i ], 18 - NLSF_W_Q ) );
        NLSF_Q15_tmp = silk_ADD32( pNLSF_Q15[ i ], silk_DIV32_16( silk_LSHIFT( (opus_int32)res_Q10[ i ], 14 ), W_tmp_Q9 ) );
        pNLSF_Q15[ i ] = (opus_int16)silk_LIMIT( NLSF_Q15_tmp, 0, 32767 );
    }

    
    silk_NLSF_stabilize( pNLSF_Q15, psNLSF_CB->deltaMin_Q15, psNLSF_CB->order );
}
