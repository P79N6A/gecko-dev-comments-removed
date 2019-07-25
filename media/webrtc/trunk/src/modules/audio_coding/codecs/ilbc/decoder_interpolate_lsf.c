

















#include "lsf_interpolate_to_poly_dec.h"
#include "bw_expand.h"
#include "defines.h"
#include "constants.h"





void WebRtcIlbcfix_DecoderInterpolateLsp(
    WebRtc_Word16 *syntdenum,  
    WebRtc_Word16 *weightdenum, 

    WebRtc_Word16 *lsfdeq,   
    WebRtc_Word16 length,   
    iLBC_Dec_Inst_t *iLBCdec_inst
    
                                          ){
  int  i, pos, lp_length;
  WebRtc_Word16  lp[LPC_FILTERORDER + 1], *lsfdeq2;

  lsfdeq2 = lsfdeq + length;
  lp_length = length + 1;

  if (iLBCdec_inst->mode==30) {
    

    WebRtcIlbcfix_LspInterpolate2PolyDec(lp, (*iLBCdec_inst).lsfdeqold, lsfdeq,
                                         WebRtcIlbcfix_kLsfWeight30ms[0], length);
    WEBRTC_SPL_MEMCPY_W16(syntdenum,lp,lp_length);
    WebRtcIlbcfix_BwExpand(weightdenum, lp, (WebRtc_Word16*)WebRtcIlbcfix_kLpcChirpSyntDenum, (WebRtc_Word16)lp_length);

    

    pos = lp_length;
    for (i = 1; i < 6; i++) {
      WebRtcIlbcfix_LspInterpolate2PolyDec(lp, lsfdeq, lsfdeq2,
                                           WebRtcIlbcfix_kLsfWeight30ms[i], length);
      WEBRTC_SPL_MEMCPY_W16(syntdenum + pos,lp,lp_length);
      WebRtcIlbcfix_BwExpand(weightdenum + pos, lp,
                             (WebRtc_Word16*)WebRtcIlbcfix_kLpcChirpSyntDenum, (WebRtc_Word16)lp_length);
      pos += lp_length;
    }
  } else { 
    
    pos = 0;
    for (i = 0; i < iLBCdec_inst->nsub; i++) {
      WebRtcIlbcfix_LspInterpolate2PolyDec(lp, iLBCdec_inst->lsfdeqold, lsfdeq,
                                           WebRtcIlbcfix_kLsfWeight20ms[i], length);
      WEBRTC_SPL_MEMCPY_W16(syntdenum+pos,lp,lp_length);
      WebRtcIlbcfix_BwExpand(weightdenum+pos, lp,
                             (WebRtc_Word16*)WebRtcIlbcfix_kLpcChirpSyntDenum, (WebRtc_Word16)lp_length);
      pos += lp_length;
    }
  }

  

  if (iLBCdec_inst->mode==30) {
    WEBRTC_SPL_MEMCPY_W16(iLBCdec_inst->lsfdeqold, lsfdeq2, length);
  } else {
    WEBRTC_SPL_MEMCPY_W16(iLBCdec_inst->lsfdeqold, lsfdeq, length);
  }
}
