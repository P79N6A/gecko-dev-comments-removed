

















#include "defines.h"
#include "simple_lsf_dequant.h"
#include "decoder_interpolate_lsf.h"
#include "index_conv_dec.h"
#include "do_plc.h"
#include "constants.h"
#include "enhancer_interface.h"
#include "xcorr_coef.h"
#include "lsf_check.h"
#include "decode_residual.h"
#include "unpack_bits.h"
#include "hp_output.h"
#ifndef WEBRTC_ARCH_BIG_ENDIAN
#include "swap_bytes.h"
#endif





void WebRtcIlbcfix_DecodeImpl(
    int16_t *decblock,    
    const uint16_t *bytes, 
    iLBC_Dec_Inst_t *iLBCdec_inst, 

    int16_t mode      

                           ) {
  int i;
  int16_t order_plus_one;

  int16_t last_bit;
  int16_t *data;
  
  int16_t decresidual[BLOCKL_MAX];
  int16_t PLCresidual[BLOCKL_MAX + LPC_FILTERORDER];
  int16_t syntdenum[NSUB_MAX*(LPC_FILTERORDER+1)];
  int16_t PLClpc[LPC_FILTERORDER + 1];
#ifndef WEBRTC_ARCH_BIG_ENDIAN
  uint16_t swapped[NO_OF_WORDS_30MS];
#endif
  iLBC_bits *iLBCbits_inst = (iLBC_bits*)PLCresidual;

  
  data = &PLCresidual[LPC_FILTERORDER];

  if (mode) { 

    

    

#ifndef WEBRTC_ARCH_BIG_ENDIAN
    WebRtcIlbcfix_SwapBytes(bytes, iLBCdec_inst->no_of_words, swapped);
    last_bit = WebRtcIlbcfix_UnpackBits(swapped, iLBCbits_inst, iLBCdec_inst->mode);
#else
    last_bit = WebRtcIlbcfix_UnpackBits(bytes, iLBCbits_inst, iLBCdec_inst->mode);
#endif

    
    if (iLBCbits_inst->startIdx<1)
      mode = 0;
    if ((iLBCdec_inst->mode==20) && (iLBCbits_inst->startIdx>3))
      mode = 0;
    if ((iLBCdec_inst->mode==30) && (iLBCbits_inst->startIdx>5))
      mode = 0;
    if (last_bit==1)
      mode = 0;

    if (mode) { 
      
      int16_t lsfdeq[LPC_FILTERORDER*LPC_N_MAX];
      int16_t weightdenum[(LPC_FILTERORDER + 1)*NSUB_MAX];

      
      WebRtcIlbcfix_IndexConvDec(iLBCbits_inst->cb_index);

      
      WebRtcIlbcfix_SimpleLsfDeQ(lsfdeq, (int16_t*)(iLBCbits_inst->lsf), iLBCdec_inst->lpc_n);
      WebRtcIlbcfix_LsfCheck(lsfdeq, LPC_FILTERORDER, iLBCdec_inst->lpc_n);
      WebRtcIlbcfix_DecoderInterpolateLsp(syntdenum, weightdenum,
                                          lsfdeq, LPC_FILTERORDER, iLBCdec_inst);

      
      WebRtcIlbcfix_DecodeResidual(iLBCdec_inst, iLBCbits_inst, decresidual, syntdenum);

      
      WebRtcIlbcfix_DoThePlc( PLCresidual, PLClpc, 0,
                              decresidual, syntdenum + (LPC_FILTERORDER + 1)*(iLBCdec_inst->nsub - 1),
                              (int16_t)(iLBCdec_inst->last_lag), iLBCdec_inst);

      
      WEBRTC_SPL_MEMCPY_W16(decresidual, PLCresidual, iLBCdec_inst->blockl);
    }

  }

  if (mode == 0) {
    



    

    WebRtcIlbcfix_DoThePlc( PLCresidual, PLClpc, 1,
                            decresidual, syntdenum, (int16_t)(iLBCdec_inst->last_lag), iLBCdec_inst);

    WEBRTC_SPL_MEMCPY_W16(decresidual, PLCresidual, iLBCdec_inst->blockl);

    order_plus_one = LPC_FILTERORDER + 1;

    for (i = 0; i < iLBCdec_inst->nsub; i++) {
      WEBRTC_SPL_MEMCPY_W16(syntdenum+(i*order_plus_one),
                            PLClpc, order_plus_one);
    }
  }

  if ((*iLBCdec_inst).use_enhancer == 1) { 

    
    if (iLBCdec_inst->prev_enh_pl==2) {
      for (i=0;i<iLBCdec_inst->nsub;i++) {
        WEBRTC_SPL_MEMCPY_W16(&(iLBCdec_inst->old_syntdenum[i*(LPC_FILTERORDER+1)]),
                              syntdenum, (LPC_FILTERORDER+1));
      }
    }

    
    (*iLBCdec_inst).last_lag =
        WebRtcIlbcfix_EnhancerInterface(data, decresidual, iLBCdec_inst);

    

    
    WEBRTC_SPL_MEMCPY_W16(&data[-LPC_FILTERORDER], iLBCdec_inst->syntMem, LPC_FILTERORDER);

    if (iLBCdec_inst->mode==20) {
      
      i=0;
      WebRtcSpl_FilterARFastQ12(
          data, data,
          iLBCdec_inst->old_syntdenum + (i+iLBCdec_inst->nsub-1)*(LPC_FILTERORDER+1),
          LPC_FILTERORDER+1, SUBL);

      for (i=1; i < iLBCdec_inst->nsub; i++) {
        WebRtcSpl_FilterARFastQ12(
            data+i*SUBL, data+i*SUBL,
            syntdenum+(i-1)*(LPC_FILTERORDER+1),
            LPC_FILTERORDER+1, SUBL);
      }

    } else if (iLBCdec_inst->mode==30) {
      
      for (i=0; i < 2; i++) {
        WebRtcSpl_FilterARFastQ12(
            data+i*SUBL, data+i*SUBL,
            iLBCdec_inst->old_syntdenum + (i+4)*(LPC_FILTERORDER+1),
            LPC_FILTERORDER+1, SUBL);
      }
      for (i=2; i < iLBCdec_inst->nsub; i++) {
        WebRtcSpl_FilterARFastQ12(
            data+i*SUBL, data+i*SUBL,
            syntdenum+(i-2)*(LPC_FILTERORDER+1),
            LPC_FILTERORDER+1, SUBL);
      }
    }

    
    WEBRTC_SPL_MEMCPY_W16(iLBCdec_inst->syntMem, &data[iLBCdec_inst->blockl-LPC_FILTERORDER], LPC_FILTERORDER);

  } else { 
    int16_t lag;

    
    lag = 20;
    if (iLBCdec_inst->mode==20) {
      lag = (int16_t)WebRtcIlbcfix_XcorrCoef(
          &decresidual[iLBCdec_inst->blockl-60],
          &decresidual[iLBCdec_inst->blockl-60-lag],
          60,
          80, lag, -1);
    } else {
      lag = (int16_t)WebRtcIlbcfix_XcorrCoef(
          &decresidual[iLBCdec_inst->blockl-ENH_BLOCKL],
          &decresidual[iLBCdec_inst->blockl-ENH_BLOCKL-lag],
          ENH_BLOCKL,
          100, lag, -1);
    }

    
    (*iLBCdec_inst).last_lag = (int)lag;

    
    WEBRTC_SPL_MEMCPY_W16(data, decresidual, iLBCdec_inst->blockl);

    
    WEBRTC_SPL_MEMCPY_W16(&data[-LPC_FILTERORDER], iLBCdec_inst->syntMem, LPC_FILTERORDER);

    for (i=0; i < iLBCdec_inst->nsub; i++) {
      WebRtcSpl_FilterARFastQ12(
          data+i*SUBL, data+i*SUBL,
          syntdenum + i*(LPC_FILTERORDER+1),
          LPC_FILTERORDER+1, SUBL);
    }

    
    WEBRTC_SPL_MEMCPY_W16(iLBCdec_inst->syntMem, &data[iLBCdec_inst->blockl-LPC_FILTERORDER], LPC_FILTERORDER);
  }

  WEBRTC_SPL_MEMCPY_W16(decblock,data,iLBCdec_inst->blockl);

  
  WebRtcIlbcfix_HpOutput(decblock, (int16_t*)WebRtcIlbcfix_kHpOutCoefs,
                         iLBCdec_inst->hpimemy, iLBCdec_inst->hpimemx,
                         iLBCdec_inst->blockl);

  WEBRTC_SPL_MEMCPY_W16(iLBCdec_inst->old_syntdenum,
                        syntdenum, iLBCdec_inst->nsub*(LPC_FILTERORDER+1));

  iLBCdec_inst->prev_enh_pl=0;

  if (mode==0) { 
    iLBCdec_inst->prev_enh_pl=1;
  }
}
