

















#include "defines.h"





void WebRtcIlbcfix_HpInput(
    int16_t *signal,     
    int16_t *ba,      


    int16_t *y,      

    int16_t *x,      
    int16_t len)      
{
  int i;
  int32_t tmpW32;
  int32_t tmpW32b;

  for (i=0; i<len; i++) {

    




    tmpW32  = WEBRTC_SPL_MUL_16_16(y[1], ba[3]);     
    tmpW32 += WEBRTC_SPL_MUL_16_16(y[3], ba[4]);     
    tmpW32 = (tmpW32>>15);
    tmpW32 += WEBRTC_SPL_MUL_16_16(y[0], ba[3]);     
    tmpW32 += WEBRTC_SPL_MUL_16_16(y[2], ba[4]);     
    tmpW32 = (tmpW32<<1);

    tmpW32 += WEBRTC_SPL_MUL_16_16(signal[i], ba[0]);   
    tmpW32 += WEBRTC_SPL_MUL_16_16(x[0],      ba[1]);   
    tmpW32 += WEBRTC_SPL_MUL_16_16(x[1],      ba[2]);   

    
    x[1] = x[0];
    x[0] = signal[i];

    
    tmpW32b = tmpW32 + 4096;

    
    tmpW32b = WEBRTC_SPL_SAT((int32_t)268435455, tmpW32b, (int32_t)-268435456);

    
    signal[i] = (int16_t)WEBRTC_SPL_RSHIFT_W32(tmpW32b, 13);

    
    y[2] = y[0];
    y[3] = y[1];

    
    if (tmpW32>268435455) {
      tmpW32 = WEBRTC_SPL_WORD32_MAX;
    } else if (tmpW32<-268435456) {
      tmpW32 = WEBRTC_SPL_WORD32_MIN;
    } else {
      tmpW32 = WEBRTC_SPL_LSHIFT_W32(tmpW32, 3);
    }

    y[0] = (int16_t)(tmpW32 >> 16);
    y[1] = (int16_t)((tmpW32 - WEBRTC_SPL_LSHIFT_W32((int32_t)y[0], 16))>>1);
  }

  return;
}
