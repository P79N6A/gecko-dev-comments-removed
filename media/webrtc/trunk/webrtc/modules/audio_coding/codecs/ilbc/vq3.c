

















#include "vq3.h"
#include "constants.h"





void WebRtcIlbcfix_Vq3(
    int16_t *Xq, 
    int16_t *index,
    int16_t *CB, 
    int16_t *X,  
    int16_t n_cb
                       ){
  int16_t i, j;
  int16_t pos, minindex=0;
  int16_t tmp;
  int32_t dist, mindist;

  pos = 0;
  mindist = WEBRTC_SPL_WORD32_MAX; 

  
  for (j = 0; j < n_cb; j++) {
    tmp = X[0] - CB[pos];
    dist = WEBRTC_SPL_MUL_16_16(tmp, tmp);
    for (i = 1; i < 3; i++) {
      tmp = X[i] - CB[pos + i];
      dist += WEBRTC_SPL_MUL_16_16(tmp, tmp);
    }

    if (dist < mindist) {
      mindist = dist;
      minindex = j;
    }
    pos += 3;
  }

  
  for (i = 0; i < 3; i++) {
    Xq[i] = CB[minindex*3 + i];
  }
  *index = minindex;

}
