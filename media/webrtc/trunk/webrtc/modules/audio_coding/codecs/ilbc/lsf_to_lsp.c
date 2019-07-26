

















#include "defines.h"
#include "constants.h"





void WebRtcIlbcfix_Lsf2Lsp(
    int16_t *lsf, 
    int16_t *lsp, 
    int16_t m  
                           ) {
  int16_t i, k;
  int16_t diff; 

  int16_t freq; 
  int32_t tmpW32;

  for(i=0; i<m; i++)
  {
    freq = (int16_t)WEBRTC_SPL_MUL_16_16_RSFT(lsf[i], 20861, 15);
    
    




    k = WEBRTC_SPL_RSHIFT_W16(freq, 8);
    diff = (freq&0x00ff);

    

    if (k>63) {
      k = 63;
    }

    
    tmpW32 = WEBRTC_SPL_MUL_16_16(WebRtcIlbcfix_kCosDerivative[k], diff);
    lsp[i] = WebRtcIlbcfix_kCos[k]+(int16_t)(WEBRTC_SPL_RSHIFT_W32(tmpW32, 12));
  }

  return;
}
