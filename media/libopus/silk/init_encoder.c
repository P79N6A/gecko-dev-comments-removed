






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef FIXED_POINT
#include "main_FIX.h"
#else
#include "main_FLP.h"
#endif
#include "tuning_parameters.h"




opus_int silk_init_encoder(
    silk_encoder_state_Fxx          *psEnc                                  
)
{
    opus_int ret = 0;

    
    silk_memset( psEnc, 0, sizeof( silk_encoder_state_Fxx ) );

    psEnc->sCmn.variable_HP_smth1_Q15 = silk_LSHIFT( silk_lin2log( SILK_FIX_CONST( VARIABLE_HP_MIN_CUTOFF_HZ, 16 ) ) - ( 16 << 7 ), 8 );
    psEnc->sCmn.variable_HP_smth2_Q15 = psEnc->sCmn.variable_HP_smth1_Q15;

    
    psEnc->sCmn.first_frame_after_reset = 1;

    
    ret += silk_VAD_Init( &psEnc->sCmn.sVAD );

    return  ret;
}
