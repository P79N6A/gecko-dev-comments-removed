

















#include "defines.h"
#include "constants.h"





void WebRtcIlbcfix_Lsf2Lsp(
    WebRtc_Word16 *lsf, 
    WebRtc_Word16 *lsp, 
    WebRtc_Word16 m  
                           ) {
  WebRtc_Word16 i, k;
  WebRtc_Word16 diff; 

  WebRtc_Word16 freq; 
  WebRtc_Word32 tmpW32;

  for(i=0; i<m; i++)
  {
    freq = (WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT(lsf[i], 20861, 15);
    
    




    k = WEBRTC_SPL_RSHIFT_W16(freq, 8);
    diff = (freq&0x00ff);

    

    if (k>63) {
      k = 63;
    }

    
    tmpW32 = WEBRTC_SPL_MUL_16_16(WebRtcIlbcfix_kCosDerivative[k], diff);
    lsp[i] = WebRtcIlbcfix_kCos[k]+(WebRtc_Word16)(WEBRTC_SPL_RSHIFT_W32(tmpW32, 12));
  }

  return;
}
