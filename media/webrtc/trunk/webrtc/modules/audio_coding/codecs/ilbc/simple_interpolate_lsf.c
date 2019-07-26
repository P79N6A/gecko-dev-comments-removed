

















#include "defines.h"
#include "lsf_interpolate_to_poly_enc.h"
#include "bw_expand.h"
#include "constants.h"





void WebRtcIlbcfix_SimpleInterpolateLsf(
    WebRtc_Word16 *syntdenum, 


    WebRtc_Word16 *weightdenum, 


    WebRtc_Word16 *lsf,  
    WebRtc_Word16 *lsfdeq,  
    WebRtc_Word16 *lsfold,  

    WebRtc_Word16 *lsfdeqold, 

    WebRtc_Word16 length,  
    iLBC_Enc_Inst_t *iLBCenc_inst
    
                                        ) {
  int i, pos, lp_length;

  WebRtc_Word16 *lsf2, *lsfdeq2;
  
  WebRtc_Word16 lp[LPC_FILTERORDER + 1];

  lsf2 = lsf + length;
  lsfdeq2 = lsfdeq + length;
  lp_length = length + 1;

  if (iLBCenc_inst->mode==30) {
    


    
    WebRtcIlbcfix_LsfInterpolate2PloyEnc(lp, lsfdeqold, lsfdeq,
                                         WebRtcIlbcfix_kLsfWeight30ms[0],
                                         length);
    WEBRTC_SPL_MEMCPY_W16(syntdenum, lp, lp_length);

    
    WebRtcIlbcfix_LsfInterpolate2PloyEnc(lp, lsfold, lsf,
                                         WebRtcIlbcfix_kLsfWeight30ms[0],
                                         length);
    WebRtcIlbcfix_BwExpand(weightdenum, lp,
                           (WebRtc_Word16*)WebRtcIlbcfix_kLpcChirpWeightDenum,
                           (WebRtc_Word16)lp_length);

    


    pos = lp_length;
    for (i = 1; i < iLBCenc_inst->nsub; i++) {

      
      WebRtcIlbcfix_LsfInterpolate2PloyEnc(lp, lsfdeq, lsfdeq2,
                                           WebRtcIlbcfix_kLsfWeight30ms[i],
                                           length);
      WEBRTC_SPL_MEMCPY_W16(syntdenum + pos, lp, lp_length);

      
      WebRtcIlbcfix_LsfInterpolate2PloyEnc(lp, lsf, lsf2,
                                           WebRtcIlbcfix_kLsfWeight30ms[i],
                                           length);
      WebRtcIlbcfix_BwExpand(weightdenum + pos, lp,
                             (WebRtc_Word16*)WebRtcIlbcfix_kLpcChirpWeightDenum,
                             (WebRtc_Word16)lp_length);

      pos += lp_length;
    }

    

    WEBRTC_SPL_MEMCPY_W16(lsfold, lsf2, length);
    WEBRTC_SPL_MEMCPY_W16(lsfdeqold, lsfdeq2, length);

  } else { 
    pos = 0;
    for (i = 0; i < iLBCenc_inst->nsub; i++) {

      
      WebRtcIlbcfix_LsfInterpolate2PloyEnc(lp, lsfdeqold, lsfdeq,
                                           WebRtcIlbcfix_kLsfWeight20ms[i],
                                           length);
      WEBRTC_SPL_MEMCPY_W16(syntdenum + pos, lp, lp_length);

      
      WebRtcIlbcfix_LsfInterpolate2PloyEnc(lp, lsfold, lsf,
                                           WebRtcIlbcfix_kLsfWeight20ms[i],
                                           length);
      WebRtcIlbcfix_BwExpand(weightdenum+pos, lp,
                             (WebRtc_Word16*)WebRtcIlbcfix_kLpcChirpWeightDenum,
                             (WebRtc_Word16)lp_length);

      pos += lp_length;
    }

    

    WEBRTC_SPL_MEMCPY_W16(lsfold, lsf, length);
    WEBRTC_SPL_MEMCPY_W16(lsfdeqold, lsfdeq, length);

  }

  return;
}
