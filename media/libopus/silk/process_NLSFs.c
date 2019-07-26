






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main.h"


void silk_process_NLSFs(
    silk_encoder_state          *psEncC,                            
    opus_int16                  PredCoef_Q12[ 2 ][ MAX_LPC_ORDER ], 
    opus_int16                  pNLSF_Q15[         MAX_LPC_ORDER ], 
    const opus_int16            prev_NLSFq_Q15[    MAX_LPC_ORDER ]  
)
{
    opus_int     i, doInterpolate;
    opus_int     NLSF_mu_Q20;
    opus_int32   i_sqr_Q15;
    opus_int16   pNLSF0_temp_Q15[ MAX_LPC_ORDER ];
    opus_int16   pNLSFW_QW[ MAX_LPC_ORDER ];
    opus_int16   pNLSFW0_temp_QW[ MAX_LPC_ORDER ];

    silk_assert( psEncC->speech_activity_Q8 >=   0 );
    silk_assert( psEncC->speech_activity_Q8 <= SILK_FIX_CONST( 1.0, 8 ) );
    silk_assert( psEncC->useInterpolatedNLSFs == 1 || psEncC->indices.NLSFInterpCoef_Q2 == ( 1 << 2 ) );

    
    
    
    
    NLSF_mu_Q20 = silk_SMLAWB( SILK_FIX_CONST( 0.003, 20 ), SILK_FIX_CONST( -0.001, 28 ), psEncC->speech_activity_Q8 );
    if( psEncC->nb_subfr == 2 ) {
        
        NLSF_mu_Q20 = silk_ADD_RSHIFT( NLSF_mu_Q20, NLSF_mu_Q20, 1 );
    }

    silk_assert( NLSF_mu_Q20 >  0 );
    silk_assert( NLSF_mu_Q20 <= SILK_FIX_CONST( 0.005, 20 ) );

    
    silk_NLSF_VQ_weights_laroia( pNLSFW_QW, pNLSF_Q15, psEncC->predictLPCOrder );

    
    doInterpolate = ( psEncC->useInterpolatedNLSFs == 1 ) && ( psEncC->indices.NLSFInterpCoef_Q2 < 4 );
    if( doInterpolate ) {
        
        silk_interpolate( pNLSF0_temp_Q15, prev_NLSFq_Q15, pNLSF_Q15,
            psEncC->indices.NLSFInterpCoef_Q2, psEncC->predictLPCOrder );

        
        silk_NLSF_VQ_weights_laroia( pNLSFW0_temp_QW, pNLSF0_temp_Q15, psEncC->predictLPCOrder );

        
        i_sqr_Q15 = silk_LSHIFT( silk_SMULBB( psEncC->indices.NLSFInterpCoef_Q2, psEncC->indices.NLSFInterpCoef_Q2 ), 11 );
        for( i = 0; i < psEncC->predictLPCOrder; i++ ) {
            pNLSFW_QW[ i ] = silk_SMLAWB( silk_RSHIFT( pNLSFW_QW[ i ], 1 ), pNLSFW0_temp_QW[ i ], i_sqr_Q15 );
            silk_assert( pNLSFW_QW[ i ] >= 1 );
        }
    }

    silk_NLSF_encode( psEncC->indices.NLSFIndices, pNLSF_Q15, psEncC->psNLSF_CB, pNLSFW_QW,
        NLSF_mu_Q20, psEncC->NLSF_MSVQ_Survivors, psEncC->indices.signalType );

    
    silk_NLSF2A( PredCoef_Q12[ 1 ], pNLSF_Q15, psEncC->predictLPCOrder );

    if( doInterpolate ) {
        
        silk_interpolate( pNLSF0_temp_Q15, prev_NLSFq_Q15, pNLSF_Q15,
            psEncC->indices.NLSFInterpCoef_Q2, psEncC->predictLPCOrder );

        
        silk_NLSF2A( PredCoef_Q12[ 0 ], pNLSF0_temp_Q15, psEncC->predictLPCOrder );

    } else {
        
        silk_memcpy( PredCoef_Q12[ 0 ], PredCoef_Q12[ 1 ], psEncC->predictLPCOrder * sizeof( opus_int16 ) );
    }
}
