






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main.h"




opus_int32 silk_NLSF_encode(                                    
          opus_int8             *NLSFIndices,                   
          opus_int16            *pNLSF_Q15,                     
    const silk_NLSF_CB_struct   *psNLSF_CB,                     
    const opus_int16            *pW_QW,                         
    const opus_int              NLSF_mu_Q20,                    
    const opus_int              nSurvivors,                     
    const opus_int              signalType                      
)
{
    opus_int         i, s, ind1, bestIndex, prob_Q8, bits_q7;
    opus_int32       W_tmp_Q9;
    opus_int32       err_Q26[      NLSF_VQ_MAX_VECTORS ];
    opus_int32       RD_Q25[       NLSF_VQ_MAX_SURVIVORS ];
    opus_int         tempIndices1[ NLSF_VQ_MAX_SURVIVORS ];
    opus_int8        tempIndices2[ NLSF_VQ_MAX_SURVIVORS * MAX_LPC_ORDER ];
    opus_int16       res_Q15[      MAX_LPC_ORDER ];
    opus_int16       res_Q10[      MAX_LPC_ORDER ];
    opus_int16       NLSF_tmp_Q15[ MAX_LPC_ORDER ];
    opus_int16       W_tmp_QW[     MAX_LPC_ORDER ];
    opus_int16       W_adj_Q5[     MAX_LPC_ORDER ];
    opus_uint8       pred_Q8[      MAX_LPC_ORDER ];
    opus_int16       ec_ix[        MAX_LPC_ORDER ];
    const opus_uint8 *pCB_element, *iCDF_ptr;

    silk_assert( nSurvivors <= NLSF_VQ_MAX_SURVIVORS );
    silk_assert( signalType >= 0 && signalType <= 2 );
    silk_assert( NLSF_mu_Q20 <= 32767 && NLSF_mu_Q20 >= 0 );

    
    silk_NLSF_stabilize( pNLSF_Q15, psNLSF_CB->deltaMin_Q15, psNLSF_CB->order );

    
    silk_NLSF_VQ( err_Q26, pNLSF_Q15, psNLSF_CB->CB1_NLSF_Q8, psNLSF_CB->nVectors, psNLSF_CB->order );

    
    silk_insertion_sort_increasing( err_Q26, tempIndices1, psNLSF_CB->nVectors, nSurvivors );

    
    for( s = 0; s < nSurvivors; s++ ) {
        ind1 = tempIndices1[ s ];

        
        pCB_element = &psNLSF_CB->CB1_NLSF_Q8[ ind1 * psNLSF_CB->order ];
        for( i = 0; i < psNLSF_CB->order; i++ ) {
            NLSF_tmp_Q15[ i ] = silk_LSHIFT16( (opus_int16)pCB_element[ i ], 7 );
            res_Q15[ i ] = pNLSF_Q15[ i ] - NLSF_tmp_Q15[ i ];
        }

        
        silk_NLSF_VQ_weights_laroia( W_tmp_QW, NLSF_tmp_Q15, psNLSF_CB->order );

        
        for( i = 0; i < psNLSF_CB->order; i++ ) {
            W_tmp_Q9 = silk_SQRT_APPROX( silk_LSHIFT( (opus_int32)W_tmp_QW[ i ], 18 - NLSF_W_Q ) );
            res_Q10[ i ] = (opus_int16)silk_RSHIFT( silk_SMULBB( res_Q15[ i ], W_tmp_Q9 ), 14 );
        }

        
        for( i = 0; i < psNLSF_CB->order; i++ ) {
            W_adj_Q5[ i ] = silk_DIV32_16( silk_LSHIFT( (opus_int32)pW_QW[ i ], 5 ), W_tmp_QW[ i ] );
        }

        
        silk_NLSF_unpack( ec_ix, pred_Q8, psNLSF_CB, ind1 );

        
        RD_Q25[ s ] = silk_NLSF_del_dec_quant( &tempIndices2[ s * MAX_LPC_ORDER ], res_Q10, W_adj_Q5, pred_Q8, ec_ix,
            psNLSF_CB->ec_Rates_Q5, psNLSF_CB->quantStepSize_Q16, psNLSF_CB->invQuantStepSize_Q6, NLSF_mu_Q20, psNLSF_CB->order );

        
        iCDF_ptr = &psNLSF_CB->CB1_iCDF[ ( signalType >> 1 ) * psNLSF_CB->nVectors ];
        if( ind1 == 0 ) {
            prob_Q8 = 256 - iCDF_ptr[ ind1 ];
        } else {
            prob_Q8 = iCDF_ptr[ ind1 - 1 ] - iCDF_ptr[ ind1 ];
        }
        bits_q7 = ( 8 << 7 ) - silk_lin2log( prob_Q8 );
        RD_Q25[ s ] = silk_SMLABB( RD_Q25[ s ], bits_q7, silk_RSHIFT( NLSF_mu_Q20, 2 ) );
    }

    
    silk_insertion_sort_increasing( RD_Q25, &bestIndex, nSurvivors, 1 );

    NLSFIndices[ 0 ] = (opus_int8)tempIndices1[ bestIndex ];
    silk_memcpy( &NLSFIndices[ 1 ], &tempIndices2[ bestIndex * MAX_LPC_ORDER ], psNLSF_CB->order * sizeof( opus_int8 ) );

    
    silk_NLSF_decode( pNLSF_Q15, NLSFIndices, psNLSF_CB );

    return RD_Q25[ 0 ];
}
