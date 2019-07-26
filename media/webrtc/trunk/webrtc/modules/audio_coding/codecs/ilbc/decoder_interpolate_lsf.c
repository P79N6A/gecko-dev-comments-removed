

















#include "lsf_interpolate_to_poly_dec.h"
#include "bw_expand.h"
#include "defines.h"
#include "constants.h"





void WebRtcIlbcfix_DecoderInterpolateLsp(
    int16_t *syntdenum,  
    int16_t *weightdenum, 

    int16_t *lsfdeq,   
    int16_t length,   
    iLBC_Dec_Inst_t *iLBCdec_inst
    
                                          ){
  int  i, pos, lp_length;
  int16_t  lp[LPC_FILTERORDER + 1], *lsfdeq2;

  lsfdeq2 = lsfdeq + length;
  lp_length = length + 1;

  if (iLBCdec_inst->mode==30) {
    

    WebRtcIlbcfix_LspInterpolate2PolyDec(lp, (*iLBCdec_inst).lsfdeqold, lsfdeq,
                                         WebRtcIlbcfix_kLsfWeight30ms[0], length);
    WEBRTC_SPL_MEMCPY_W16(syntdenum,lp,lp_length);
    WebRtcIlbcfix_BwExpand(weightdenum, lp, (int16_t*)WebRtcIlbcfix_kLpcChirpSyntDenum, (int16_t)lp_length);

    

    pos = lp_length;
    for (i = 1; i < 6; i++) {
      WebRtcIlbcfix_LspInterpolate2PolyDec(lp, lsfdeq, lsfdeq2,
                                           WebRtcIlbcfix_kLsfWeight30ms[i], length);
      WEBRTC_SPL_MEMCPY_W16(syntdenum + pos,lp,lp_length);
      WebRtcIlbcfix_BwExpand(weightdenum + pos, lp,
                             (int16_t*)WebRtcIlbcfix_kLpcChirpSyntDenum, (int16_t)lp_length);
      pos += lp_length;
    }
  } else { 
    
    pos = 0;
    for (i = 0; i < iLBCdec_inst->nsub; i++) {
      WebRtcIlbcfix_LspInterpolate2PolyDec(lp, iLBCdec_inst->lsfdeqold, lsfdeq,
                                           WebRtcIlbcfix_kLsfWeight20ms[i], length);
      WEBRTC_SPL_MEMCPY_W16(syntdenum+pos,lp,lp_length);
      WebRtcIlbcfix_BwExpand(weightdenum+pos, lp,
                             (int16_t*)WebRtcIlbcfix_kLpcChirpSyntDenum, (int16_t)lp_length);
      pos += lp_length;
    }
  }

  

  if (iLBCdec_inst->mode==30) {
    WEBRTC_SPL_MEMCPY_W16(iLBCdec_inst->lsfdeqold, lsfdeq2, length);
  } else {
    WEBRTC_SPL_MEMCPY_W16(iLBCdec_inst->lsfdeqold, lsfdeq, length);
  }
}
