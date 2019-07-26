

















#include "defines.h"
#include "lsf_interpolate_to_poly_enc.h"
#include "bw_expand.h"
#include "constants.h"





void WebRtcIlbcfix_SimpleInterpolateLsf(
    int16_t *syntdenum, 


    int16_t *weightdenum, 


    int16_t *lsf,  
    int16_t *lsfdeq,  
    int16_t *lsfold,  

    int16_t *lsfdeqold, 

    int16_t length,  
    iLBC_Enc_Inst_t *iLBCenc_inst
    
                                        ) {
  int i, pos, lp_length;

  int16_t *lsf2, *lsfdeq2;
  
  int16_t lp[LPC_FILTERORDER + 1];

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
                           (int16_t*)WebRtcIlbcfix_kLpcChirpWeightDenum,
                           (int16_t)lp_length);

    


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
                             (int16_t*)WebRtcIlbcfix_kLpcChirpWeightDenum,
                             (int16_t)lp_length);

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
                             (int16_t*)WebRtcIlbcfix_kLpcChirpWeightDenum,
                             (int16_t)lp_length);

      pos += lp_length;
    }

    

    WEBRTC_SPL_MEMCPY_W16(lsfold, lsf, length);
    WEBRTC_SPL_MEMCPY_W16(lsfdeqold, lsfdeq, length);

  }

  return;
}
