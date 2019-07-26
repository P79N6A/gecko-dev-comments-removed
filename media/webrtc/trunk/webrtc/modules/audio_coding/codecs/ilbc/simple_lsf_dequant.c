

















#include "defines.h"
#include "constants.h"





void WebRtcIlbcfix_SimpleLsfDeQ(
    WebRtc_Word16 *lsfdeq,  
    WebRtc_Word16 *index,  
    WebRtc_Word16 lpc_n  
                                ){
  int i, j, pos, cb_pos;

  

  pos = 0;
  cb_pos = 0;
  for (i = 0; i < LSF_NSPLIT; i++) {
    for (j = 0; j < WebRtcIlbcfix_kLsfDimCb[i]; j++) {
      lsfdeq[pos + j] = WebRtcIlbcfix_kLsfCb[cb_pos +
                                             WEBRTC_SPL_MUL_16_16(index[i], WebRtcIlbcfix_kLsfDimCb[i]) + j];
    }
    pos += WebRtcIlbcfix_kLsfDimCb[i];
    cb_pos += WEBRTC_SPL_MUL_16_16(WebRtcIlbcfix_kLsfSizeCb[i], WebRtcIlbcfix_kLsfDimCb[i]);
  }

  if (lpc_n>1) {
    
    pos = 0;
    cb_pos = 0;
    for (i = 0; i < LSF_NSPLIT; i++) {
      for (j = 0; j < WebRtcIlbcfix_kLsfDimCb[i]; j++) {
        lsfdeq[LPC_FILTERORDER + pos + j] = WebRtcIlbcfix_kLsfCb[cb_pos +
                                                                 WEBRTC_SPL_MUL_16_16(index[LSF_NSPLIT + i], WebRtcIlbcfix_kLsfDimCb[i]) + j];
      }
      pos += WebRtcIlbcfix_kLsfDimCb[i];
      cb_pos += WEBRTC_SPL_MUL_16_16(WebRtcIlbcfix_kLsfSizeCb[i], WebRtcIlbcfix_kLsfDimCb[i]);
    }
  }
  return;
}
