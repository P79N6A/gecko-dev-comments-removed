

















#include "defines.h"
#include "constants.h"
#include "abs_quant_loop.h"







void WebRtcIlbcfix_AbsQuant(
    iLBC_Enc_Inst_t *iLBCenc_inst,
    
    iLBC_bits *iLBC_encbits, 


    int16_t *in,     
    int16_t *weightDenum   
                            ) {
  int16_t *syntOut;
  int16_t quantLen[2];

  
  int16_t syntOutBuf[LPC_FILTERORDER+STATE_SHORT_LEN_30MS];
  int16_t in_weightedVec[STATE_SHORT_LEN_30MS+LPC_FILTERORDER];
  int16_t *in_weighted = &in_weightedVec[LPC_FILTERORDER];

  
  WebRtcSpl_MemSetW16(syntOutBuf, 0, LPC_FILTERORDER+STATE_SHORT_LEN_30MS);
  syntOut = &syntOutBuf[LPC_FILTERORDER];
  
  WebRtcSpl_MemSetW16(in_weightedVec, 0, LPC_FILTERORDER);

  



  if (iLBC_encbits->state_first) {
    quantLen[0]=SUBL;
    quantLen[1]=iLBCenc_inst->state_short_len-SUBL;
  } else {
    quantLen[0]=iLBCenc_inst->state_short_len-SUBL;
    quantLen[1]=SUBL;
  }

  

  WebRtcSpl_FilterARFastQ12(
      in, in_weighted,
      weightDenum, LPC_FILTERORDER+1, quantLen[0]);
  WebRtcSpl_FilterARFastQ12(
      &in[quantLen[0]], &in_weighted[quantLen[0]],
      &weightDenum[LPC_FILTERORDER+1], LPC_FILTERORDER+1, quantLen[1]);

  WebRtcIlbcfix_AbsQuantLoop(
      syntOut,
      in_weighted,
      weightDenum,
      quantLen,
      iLBC_encbits->idxVec);

}
