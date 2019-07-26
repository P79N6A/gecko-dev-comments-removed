

















#include "defines.h"
#include "constants.h"
#include "abs_quant.h"





void WebRtcIlbcfix_StateSearch(
    iLBC_Enc_Inst_t *iLBCenc_inst,
    
    iLBC_bits *iLBC_encbits,

    WebRtc_Word16 *residual,   
    WebRtc_Word16 *syntDenum,  
    WebRtc_Word16 *weightDenum  
                               ) {
  WebRtc_Word16 k, index;
  WebRtc_Word16 maxVal;
  WebRtc_Word16 scale, shift;
  WebRtc_Word32 maxValsq;
  WebRtc_Word16 scaleRes;
  WebRtc_Word16 max;
  int i;
  
  WebRtc_Word16 numerator[1+LPC_FILTERORDER];
  WebRtc_Word16 residualLongVec[2*STATE_SHORT_LEN_30MS+LPC_FILTERORDER];
  WebRtc_Word16 sampleMa[2*STATE_SHORT_LEN_30MS];
  WebRtc_Word16 *residualLong = &residualLongVec[LPC_FILTERORDER];
  WebRtc_Word16 *sampleAr = residualLong;

  
  max = WebRtcSpl_MaxAbsValueW16(residual, iLBCenc_inst->state_short_len);
  scaleRes = WebRtcSpl_GetSizeInBits(max)-12;
  scaleRes = WEBRTC_SPL_MAX(0, scaleRes);
  
  for (i=0; i<LPC_FILTERORDER+1; i++) {
    numerator[i] = (syntDenum[LPC_FILTERORDER-i]>>scaleRes);
  }

  


  WEBRTC_SPL_MEMCPY_W16(residualLong, residual, iLBCenc_inst->state_short_len);
  WebRtcSpl_MemSetW16(residualLong + iLBCenc_inst->state_short_len, 0, iLBCenc_inst->state_short_len);

  
  WebRtcSpl_MemSetW16(residualLongVec, 0, LPC_FILTERORDER);
  WebRtcSpl_FilterMAFastQ12(
      residualLong, sampleMa,
      numerator, LPC_FILTERORDER+1, (WebRtc_Word16)(iLBCenc_inst->state_short_len + LPC_FILTERORDER));
  WebRtcSpl_MemSetW16(&sampleMa[iLBCenc_inst->state_short_len + LPC_FILTERORDER], 0, iLBCenc_inst->state_short_len - LPC_FILTERORDER);

  WebRtcSpl_FilterARFastQ12(
      sampleMa, sampleAr,
      syntDenum, LPC_FILTERORDER+1, (WebRtc_Word16)(2*iLBCenc_inst->state_short_len));

  for(k=0;k<iLBCenc_inst->state_short_len;k++){
    sampleAr[k] += sampleAr[k+iLBCenc_inst->state_short_len];
  }

  
  maxVal=WebRtcSpl_MaxAbsValueW16(sampleAr, iLBCenc_inst->state_short_len);

  

  if ((((WebRtc_Word32)maxVal)<<scaleRes)<23170) {
    maxValsq=((WebRtc_Word32)maxVal*maxVal)<<(2+2*scaleRes);
  } else {
    maxValsq=(WebRtc_Word32)WEBRTC_SPL_WORD32_MAX;
  }

  index=0;
  for (i=0;i<63;i++) {

    if (maxValsq>=WebRtcIlbcfix_kChooseFrgQuant[i]) {
      index=i+1;
    } else {
      i=63;
    }
  }
  iLBC_encbits->idxForMax=index;

  
  scale=WebRtcIlbcfix_kScale[index];

  if (index<27) { 
    shift=4;
  } else { 
    shift=9;
  }

  
  WebRtcSpl_ScaleVectorWithSat(sampleAr, sampleAr, scale,
                              iLBCenc_inst->state_short_len, (WebRtc_Word16)(shift-scaleRes));

  
  WebRtcIlbcfix_AbsQuant(iLBCenc_inst, iLBC_encbits, sampleAr, weightDenum);

  return;
}
