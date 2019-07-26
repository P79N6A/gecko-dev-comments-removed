

















#include "defines.h"
#include "simple_lpc_analysis.h"
#include "simple_interpolate_lsf.h"
#include "simple_lsf_quant.h"
#include "lsf_check.h"
#include "constants.h"





void WebRtcIlbcfix_LpcEncode(
    int16_t *syntdenum,  

    int16_t *weightdenum, 

    int16_t *lsf_index,  
    int16_t *data,   
    iLBC_Enc_Inst_t *iLBCenc_inst
    
                              ) {
  
  int16_t lsf[LPC_FILTERORDER * LPC_N_MAX];
  int16_t lsfdeq[LPC_FILTERORDER * LPC_N_MAX];

  
  WebRtcIlbcfix_SimpleLpcAnalysis(lsf, data, iLBCenc_inst);

  
  WebRtcIlbcfix_SimpleLsfQ(lsfdeq, lsf_index, lsf, iLBCenc_inst->lpc_n);

  
  WebRtcIlbcfix_LsfCheck(lsfdeq, LPC_FILTERORDER, iLBCenc_inst->lpc_n);

  

  WebRtcIlbcfix_SimpleInterpolateLsf(syntdenum, weightdenum,
                                     lsf, lsfdeq, iLBCenc_inst->lsfold,
                                     iLBCenc_inst->lsfdeqold, LPC_FILTERORDER, iLBCenc_inst);

  return;
}
