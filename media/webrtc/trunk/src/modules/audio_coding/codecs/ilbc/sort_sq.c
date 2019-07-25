

















#include "defines.h"





void WebRtcIlbcfix_SortSq(
    WebRtc_Word16 *xq,   
    WebRtc_Word16 *index,  
    WebRtc_Word16 x,   
    const WebRtc_Word16 *cb, 
    WebRtc_Word16 cb_size  
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

    if (x > WEBRTC_SPL_RSHIFT_W32(( (WebRtc_Word32)cb[i] + cb[i - 1] + 1),1)) {
      *index = i;
      *xq = cb[i];
    } else {
      *index = i - 1;
      *xq = cb[i - 1];
    }
  }
}
