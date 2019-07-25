

















#include "defines.h"
#include "constants.h"





WebRtc_Word16 WebRtcIlbcfix_FrameClassify(
    
    iLBC_Enc_Inst_t *iLBCenc_inst,
    
    WebRtc_Word16 *residualFIX 
                                                ){
  WebRtc_Word16 max, scale;
  WebRtc_Word32 ssqEn[NSUB_MAX-1];
  WebRtc_Word16 *ssqPtr;
  WebRtc_Word32 *seqEnPtr;
  WebRtc_Word32 maxW32;
  WebRtc_Word16 scale1;
  WebRtc_Word16 pos;
  int n;

  






  max = WebRtcSpl_MaxAbsValueW16(residualFIX, iLBCenc_inst->blockl);
  scale=WebRtcSpl_GetSizeInBits(WEBRTC_SPL_MUL_16_16(max,max));

  
  scale = scale-24;
  scale1 = WEBRTC_SPL_MAX(0, scale);

  
  ssqPtr=residualFIX + 2;
  seqEnPtr=ssqEn;
  for (n=(iLBCenc_inst->nsub-1); n>0; n--) {
    (*seqEnPtr) = WebRtcSpl_DotProductWithScale(ssqPtr, ssqPtr, 76, scale1);
    ssqPtr += 40;
    seqEnPtr++;
  }

  
  maxW32 = WebRtcSpl_MaxValueW32(ssqEn, (WebRtc_Word16)(iLBCenc_inst->nsub-1));
  scale = WebRtcSpl_GetSizeInBits(maxW32) - 20;
  scale1 = WEBRTC_SPL_MAX(0, scale);

  


  seqEnPtr=ssqEn;
  if (iLBCenc_inst->mode==20) {
    ssqPtr=(WebRtc_Word16*)WebRtcIlbcfix_kStartSequenceEnrgWin+1;
  } else {
    ssqPtr=(WebRtc_Word16*)WebRtcIlbcfix_kStartSequenceEnrgWin;
  }
  for (n=(iLBCenc_inst->nsub-1); n>0; n--) {
    (*seqEnPtr)=WEBRTC_SPL_MUL(((*seqEnPtr)>>scale1), (*ssqPtr));
    seqEnPtr++;
    ssqPtr++;
  }

  
  pos = WebRtcSpl_MaxIndexW32(ssqEn, (WebRtc_Word16)(iLBCenc_inst->nsub-1)) + 1;

  return(pos);
}
