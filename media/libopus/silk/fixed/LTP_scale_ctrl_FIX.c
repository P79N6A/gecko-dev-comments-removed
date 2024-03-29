


























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main_FIX.h"


void silk_LTP_scale_ctrl_FIX(
    silk_encoder_state_FIX          *psEnc,                                 
    silk_encoder_control_FIX        *psEncCtrl,                             
    opus_int                        condCoding                              
)
{
    opus_int round_loss;

    if( condCoding == CODE_INDEPENDENTLY ) {
        
        round_loss = psEnc->sCmn.PacketLoss_perc + psEnc->sCmn.nFramesPerPacket;
        psEnc->sCmn.indices.LTP_scaleIndex = (opus_int8)silk_LIMIT(
            silk_SMULWB( silk_SMULBB( round_loss, psEncCtrl->LTPredCodGain_Q7 ), SILK_FIX_CONST( 0.1, 9 ) ), 0, 2 );
    } else {
        
        psEnc->sCmn.indices.LTP_scaleIndex = 0;
    }
    psEncCtrl->LTP_scale_Q14 = silk_LTPScales_table_Q14[ psEnc->sCmn.indices.LTP_scaleIndex ];
}
