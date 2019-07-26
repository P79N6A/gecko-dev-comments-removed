






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main.h"


void silk_decode_parameters(
    silk_decoder_state          *psDec,                         
    silk_decoder_control        *psDecCtrl,                     
    opus_int                    condCoding                      
)
{
    opus_int   i, k, Ix;
    opus_int16 pNLSF_Q15[ MAX_LPC_ORDER ], pNLSF0_Q15[ MAX_LPC_ORDER ];
    const opus_int8 *cbk_ptr_Q7;

    
    silk_gains_dequant( psDecCtrl->Gains_Q16, psDec->indices.GainsIndices,
        &psDec->LastGainIndex, condCoding == CODE_CONDITIONALLY, psDec->nb_subfr );

    
    
    
    silk_NLSF_decode( pNLSF_Q15, psDec->indices.NLSFIndices, psDec->psNLSF_CB );

    
    silk_NLSF2A( psDecCtrl->PredCoef_Q12[ 1 ], pNLSF_Q15, psDec->LPC_order );

    
    
    if( psDec->first_frame_after_reset == 1 ) {
        psDec->indices.NLSFInterpCoef_Q2 = 4;
    }

    if( psDec->indices.NLSFInterpCoef_Q2 < 4 ) {
        
        
        for( i = 0; i < psDec->LPC_order; i++ ) {
            pNLSF0_Q15[ i ] = psDec->prevNLSF_Q15[ i ] + silk_RSHIFT( silk_MUL( psDec->indices.NLSFInterpCoef_Q2,
                pNLSF_Q15[ i ] - psDec->prevNLSF_Q15[ i ] ), 2 );
        }

        
        silk_NLSF2A( psDecCtrl->PredCoef_Q12[ 0 ], pNLSF0_Q15, psDec->LPC_order );
    } else {
        
        silk_memcpy( psDecCtrl->PredCoef_Q12[ 0 ], psDecCtrl->PredCoef_Q12[ 1 ], psDec->LPC_order * sizeof( opus_int16 ) );
    }

    silk_memcpy( psDec->prevNLSF_Q15, pNLSF_Q15, psDec->LPC_order * sizeof( opus_int16 ) );

    
    if( psDec->lossCnt ) {
        silk_bwexpander( psDecCtrl->PredCoef_Q12[ 0 ], psDec->LPC_order, BWE_AFTER_LOSS_Q16 );
        silk_bwexpander( psDecCtrl->PredCoef_Q12[ 1 ], psDec->LPC_order, BWE_AFTER_LOSS_Q16 );
    }

    if( psDec->indices.signalType == TYPE_VOICED ) {
        
        
        

        
        silk_decode_pitch( psDec->indices.lagIndex, psDec->indices.contourIndex, psDecCtrl->pitchL, psDec->fs_kHz, psDec->nb_subfr );

        
        cbk_ptr_Q7 = silk_LTP_vq_ptrs_Q7[ psDec->indices.PERIndex ]; 

        for( k = 0; k < psDec->nb_subfr; k++ ) {
            Ix = psDec->indices.LTPIndex[ k ];
            for( i = 0; i < LTP_ORDER; i++ ) {
                psDecCtrl->LTPCoef_Q14[ k * LTP_ORDER + i ] = silk_LSHIFT( cbk_ptr_Q7[ Ix * LTP_ORDER + i ], 7 );
            }
        }

        
        
        
        Ix = psDec->indices.LTP_scaleIndex;
        psDecCtrl->LTP_scale_Q14 = silk_LTPScales_table_Q14[ Ix ];
    } else {
        silk_memset( psDecCtrl->pitchL,      0,             psDec->nb_subfr * sizeof( opus_int   ) );
        silk_memset( psDecCtrl->LTPCoef_Q14, 0, LTP_ORDER * psDec->nb_subfr * sizeof( opus_int16 ) );
        psDec->indices.PERIndex  = 0;
        psDecCtrl->LTP_scale_Q14 = 0;
    }
}
