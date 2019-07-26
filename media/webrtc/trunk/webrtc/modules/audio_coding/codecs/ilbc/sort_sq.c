

















#include "defines.h"





void WebRtcIlbcfix_SortSq(
    int16_t *xq,   
    int16_t *index,  
    int16_t x,   
    const int16_t *cb, 
    int16_t cb_size  
                          ){
  int i;

  if (x <= cb[0]) {
    *index = 0;
    *xq = cb[0];
  } else {
    i = 0;
    while ((x > cb[i]) && (i < (cb_size-1))) {
      i++;
    }

    if (x > WEBRTC_SPL_RSHIFT_W32(( (int32_t)cb[i] + cb[i - 1] + 1),1)) {
      *index = i;
      *xq = cb[i];
    } else {
      *index = i - 1;
      *xq = cb[i - 1];
    }
  }
}
