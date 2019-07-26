

















#include "defines.h"
#include "simple_lpc_analysis.h"
#include "simple_interpolate_lsf.h"
#include "simple_lsf_quant.h"
#include "lsf_check.h"
#include "constants.h"





void WebRtcIlbcfix_LpcEncode(
    WebRtc_Word16 *syntdenum,  

    WebRtc_Word16 *weightdenum, 

    WebRtc_Word16 *lsf_index,  
    WebRtc_Word16 *data,   
    iLBC_Enc_Inst_t *iLBCenc_inst
    
                              ) {
  
  WebRtc_Word16 lsf[LPC_FILTERORDER * LPC_N_MAX];
  WebRtc_Word16 lsfdeq[LPC_FILTERORDER * LPC_N_MAX];

  
  WebRtcIlbcfix_SimpleLpcAnalysis(lsf, data, iLBCenc_inst);

  
  WebRtcIlbcfix_SimpleLsfQ(lsfdeq, lsf_index, lsf, iLBCenc_inst->lpc_n);

  
  WebRtcIlbcfix_LsfCheck(lsfdeq, LPC_FILTERORDER, iLBCenc_inst->lpc_n);

  

  WebRtcIlbcfix_SimpleInterpolateLsf(syntdenum, weightdenum,
                                     lsf, lsfdeq, iLBCenc_inst->lsfold,
                                     iLBCenc_inst->lsfdeqold, LPC_FILTERORDER, iLBCenc_inst);

  return;
}
