

















#include "vq4.h"
#include "constants.h"





void WebRtcIlbcfix_Vq4(
    WebRtc_Word16 *Xq, 
    WebRtc_Word16 *index,
    WebRtc_Word16 *CB, 
    WebRtc_Word16 *X,  
    WebRtc_Word16 n_cb
                       ){
  WebRtc_Word16 i, j;
  WebRtc_Word16 pos, minindex=0;
  WebRtc_Word16 tmp;
  WebRtc_Word32 dist, mindist;

  pos = 0;
  mindist = WEBRTC_SPL_WORD32_MAX; 

  
  for (j = 0; j < n_cb; j++) {
    tmp = X[0] - CB[pos];
    dist = WEBRTC_SPL_MUL_16_16(tmp, tmp);
    for (i = 1; i < 4; i++) {
      tmp = X[i] - CB[pos + i];
      dist += WEBRTC_SPL_MUL_16_16(tmp, tmp);
    }

    if (dist < mindist) {
      mindist = dist;
      minindex = j;
    }
    pos += 4;
  }

  
  for (i = 0; i < 4; i++) {
    Xq[i] = CB[minindex*4 + i];
  }
  *index = minindex;
}
