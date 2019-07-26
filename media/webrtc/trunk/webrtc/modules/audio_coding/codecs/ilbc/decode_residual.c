

















#include "defines.h"
#include "state_construct.h"
#include "cb_construct.h"
#include "index_conv_dec.h"
#include "do_plc.h"
#include "constants.h"
#include "enhancer_interface.h"
#include "xcorr_coef.h"
#include "lsf_check.h"






void WebRtcIlbcfix_DecodeResidual(
    iLBC_Dec_Inst_t *iLBCdec_inst,
    
    iLBC_bits *iLBC_encbits, 

    WebRtc_Word16 *decresidual,  
    WebRtc_Word16 *syntdenum   

                                  ) {
  WebRtc_Word16 meml_gotten, Nfor, Nback, diff, start_pos;
  WebRtc_Word16 subcount, subframe;
  WebRtc_Word16 *reverseDecresidual = iLBCdec_inst->enh_buf; 
  WebRtc_Word16 *memVec = iLBCdec_inst->prevResidual;  
  WebRtc_Word16 *mem = &memVec[CB_HALFFILTERLEN];   

  diff = STATE_LEN - iLBCdec_inst->state_short_len;

  if (iLBC_encbits->state_first == 1) {
    start_pos = (iLBC_encbits->startIdx-1)*SUBL;
  } else {
    start_pos = (iLBC_encbits->startIdx-1)*SUBL + diff;
  }

  

  WebRtcIlbcfix_StateConstruct(iLBC_encbits->idxForMax,
                               iLBC_encbits->idxVec, &syntdenum[(iLBC_encbits->startIdx-1)*(LPC_FILTERORDER+1)],
                               &decresidual[start_pos], iLBCdec_inst->state_short_len
                               );

  if (iLBC_encbits->state_first) { 

    

    WebRtcSpl_MemSetW16(mem, 0, (WebRtc_Word16)(CB_MEML-iLBCdec_inst->state_short_len));
    WEBRTC_SPL_MEMCPY_W16(mem+CB_MEML-iLBCdec_inst->state_short_len, decresidual+start_pos,
                          iLBCdec_inst->state_short_len);

    

    WebRtcIlbcfix_CbConstruct(
        &decresidual[start_pos+iLBCdec_inst->state_short_len],
        iLBC_encbits->cb_index, iLBC_encbits->gain_index,
        mem+CB_MEML-ST_MEM_L_TBL,
        ST_MEM_L_TBL, (WebRtc_Word16)diff
                              );

  }
  else {

    

    WebRtcSpl_MemCpyReversedOrder(reverseDecresidual+diff,
                                  &decresidual[(iLBC_encbits->startIdx+1)*SUBL-1-STATE_LEN], diff);

    

    meml_gotten = iLBCdec_inst->state_short_len;
    WebRtcSpl_MemCpyReversedOrder(mem+CB_MEML-1,
                                  decresidual+start_pos, meml_gotten);
    WebRtcSpl_MemSetW16(mem, 0, (WebRtc_Word16)(CB_MEML-meml_gotten));

    

    WebRtcIlbcfix_CbConstruct(
        reverseDecresidual,
        iLBC_encbits->cb_index, iLBC_encbits->gain_index,
        mem+CB_MEML-ST_MEM_L_TBL,
        ST_MEM_L_TBL, diff
                              );

    

    WebRtcSpl_MemCpyReversedOrder(&decresidual[start_pos-1],
                                  reverseDecresidual, diff);
  }

  

  subcount=1;

  

  Nfor = iLBCdec_inst->nsub-iLBC_encbits->startIdx-1;

  if( Nfor > 0 ) {

    
    WebRtcSpl_MemSetW16(mem, 0, CB_MEML-STATE_LEN);
    WEBRTC_SPL_MEMCPY_W16(mem+CB_MEML-STATE_LEN,
                          decresidual+(iLBC_encbits->startIdx-1)*SUBL, STATE_LEN);

    

    for (subframe=0; subframe<Nfor; subframe++) {

      
      WebRtcIlbcfix_CbConstruct(
          &decresidual[(iLBC_encbits->startIdx+1+subframe)*SUBL],
          iLBC_encbits->cb_index+subcount*CB_NSTAGES,
          iLBC_encbits->gain_index+subcount*CB_NSTAGES,
          mem, MEM_LF_TBL, SUBL
                                );

      
      WEBRTC_SPL_MEMMOVE_W16(mem, mem+SUBL, CB_MEML-SUBL);
      WEBRTC_SPL_MEMCPY_W16(mem+CB_MEML-SUBL,
                            &decresidual[(iLBC_encbits->startIdx+1+subframe)*SUBL], SUBL);

      subcount++;
    }

  }

  

  Nback = iLBC_encbits->startIdx-1;

  if( Nback > 0 ){

    

    meml_gotten = SUBL*(iLBCdec_inst->nsub+1-iLBC_encbits->startIdx);
    if( meml_gotten > CB_MEML ) {
      meml_gotten=CB_MEML;
    }

    WebRtcSpl_MemCpyReversedOrder(mem+CB_MEML-1,
                                  decresidual+(iLBC_encbits->startIdx-1)*SUBL, meml_gotten);
    WebRtcSpl_MemSetW16(mem, 0, (WebRtc_Word16)(CB_MEML-meml_gotten));

    

    for (subframe=0; subframe<Nback; subframe++) {

      
      WebRtcIlbcfix_CbConstruct(
          &reverseDecresidual[subframe*SUBL],
          iLBC_encbits->cb_index+subcount*CB_NSTAGES,
          iLBC_encbits->gain_index+subcount*CB_NSTAGES,
          mem, MEM_LF_TBL, SUBL
                                );

      
      WEBRTC_SPL_MEMMOVE_W16(mem, mem+SUBL, CB_MEML-SUBL);
      WEBRTC_SPL_MEMCPY_W16(mem+CB_MEML-SUBL,
                            &reverseDecresidual[subframe*SUBL], SUBL);

      subcount++;
    }

    
    WebRtcSpl_MemCpyReversedOrder(decresidual+SUBL*Nback-1,
                                  reverseDecresidual, SUBL*Nback);
  }
}
