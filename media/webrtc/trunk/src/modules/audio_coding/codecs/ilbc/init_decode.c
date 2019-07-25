

















#include "defines.h"
#include "constants.h"





WebRtc_Word16 WebRtcIlbcfix_InitDecode(		
    iLBC_Dec_Inst_t *iLBCdec_inst,	
    WebRtc_Word16 mode,					
    int use_enhancer           

                                                ) {
  int i;

  iLBCdec_inst->mode = mode;

  
  if (mode==30) {
    iLBCdec_inst->blockl = BLOCKL_30MS;
    iLBCdec_inst->nsub = NSUB_30MS;
    iLBCdec_inst->nasub = NASUB_30MS;
    iLBCdec_inst->lpc_n = LPC_N_30MS;
    iLBCdec_inst->no_of_bytes = NO_OF_BYTES_30MS;
    iLBCdec_inst->no_of_words = NO_OF_WORDS_30MS;
    iLBCdec_inst->state_short_len=STATE_SHORT_LEN_30MS;
  }
  else if (mode==20) {
    iLBCdec_inst->blockl = BLOCKL_20MS;
    iLBCdec_inst->nsub = NSUB_20MS;
    iLBCdec_inst->nasub = NASUB_20MS;
    iLBCdec_inst->lpc_n = LPC_N_20MS;
    iLBCdec_inst->no_of_bytes = NO_OF_BYTES_20MS;
    iLBCdec_inst->no_of_words = NO_OF_WORDS_20MS;
    iLBCdec_inst->state_short_len=STATE_SHORT_LEN_20MS;
  }
  else {
    return(-1);
  }

  
  WEBRTC_SPL_MEMCPY_W16(iLBCdec_inst->lsfdeqold, WebRtcIlbcfix_kLsfMean, LPC_FILTERORDER);

  
  WebRtcSpl_MemSetW16(iLBCdec_inst->syntMem, 0, LPC_FILTERORDER);

  
  WebRtcSpl_MemSetW16(iLBCdec_inst->old_syntdenum, 0, ((LPC_FILTERORDER + 1)*NSUB_MAX));
  for (i=0; i<NSUB_MAX; i++) {
    iLBCdec_inst->old_syntdenum[i*(LPC_FILTERORDER+1)] = 4096;
  }

  
  iLBCdec_inst->last_lag = 20;
  iLBCdec_inst->consPLICount = 0;
  iLBCdec_inst->prevPLI = 0;
  iLBCdec_inst->perSquare = 0;
  iLBCdec_inst->prevLag = 120;
  iLBCdec_inst->prevLpc[0] = 4096;
  WebRtcSpl_MemSetW16(iLBCdec_inst->prevLpc+1, 0, LPC_FILTERORDER);
  WebRtcSpl_MemSetW16(iLBCdec_inst->prevResidual, 0, BLOCKL_MAX);

  
  iLBCdec_inst->seed = 777;

  
  WebRtcSpl_MemSetW16(iLBCdec_inst->hpimemx, 0, 2);
  WebRtcSpl_MemSetW16(iLBCdec_inst->hpimemy, 0, 4);

  
  iLBCdec_inst->use_enhancer = use_enhancer;
  WebRtcSpl_MemSetW16(iLBCdec_inst->enh_buf, 0, (ENH_BUFL+ENH_BUFL_FILTEROVERHEAD));
  for (i=0;i<ENH_NBLOCKS_TOT;i++) {
    iLBCdec_inst->enh_period[i]=160; 
  }

  iLBCdec_inst->prev_enh_pl = 0;

  return (iLBCdec_inst->blockl);
}
