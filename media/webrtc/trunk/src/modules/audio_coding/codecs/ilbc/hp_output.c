

















#include "defines.h"





void WebRtcIlbcfix_HpOutput(
    WebRtc_Word16 *signal,     
    WebRtc_Word16 *ba,      


    WebRtc_Word16 *y,      

    WebRtc_Word16 *x,      
    WebRtc_Word16 len)      
{
  int i;
  WebRtc_Word32 tmpW32;
  WebRtc_Word32 tmpW32b;

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

    
    tmpW32b = tmpW32 + 1024;

    
    tmpW32b = WEBRTC_SPL_SAT((WebRtc_Word32)67108863, tmpW32b, (WebRtc_Word32)-67108864);

    
    signal[i] = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(tmpW32b, 11);

    
    y[2] = y[0];
    y[3] = y[1];

    
    if (tmpW32>268435455) {
      tmpW32 = WEBRTC_SPL_WORD32_MAX;
    } else if (tmpW32<-268435456) {
      tmpW32 = WEBRTC_SPL_WORD32_MIN;
    } else {
      tmpW32 = WEBRTC_SPL_LSHIFT_W32(tmpW32, 3);
    }

    y[0] = (WebRtc_Word16)(tmpW32 >> 16);
    y[1] = (WebRtc_Word16)((tmpW32 - WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)y[0], 16))>>1);

  }

  return;
}
