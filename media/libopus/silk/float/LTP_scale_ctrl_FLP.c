






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main_FLP.h"

void silk_LTP_scale_ctrl_FLP(
    silk_encoder_state_FLP          *psEnc,                             
    silk_encoder_control_FLP        *psEncCtrl,                         
    opus_int                        condCoding                          
)
{
    opus_int   round_loss;

    if( condCoding == CODE_INDEPENDENTLY ) {
        
        round_loss = psEnc->sCmn.PacketLoss_perc + psEnc->sCmn.nFramesPerPacket;
        psEnc->sCmn.indices.LTP_scaleIndex = (opus_int8)silk_LIMIT( round_loss * psEncCtrl->LTPredCodGain * 0.1f, 0.0f, 2.0f );
    } else {
        
        psEnc->sCmn.indices.LTP_scaleIndex = 0;
    }

    psEncCtrl->LTP_scale = (silk_float)silk_LTPScales_table_Q14[ psEnc->sCmn.indices.LTP_scaleIndex ] / 16384.0f;
}
