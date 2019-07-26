

















#include "defines.h"




















void WebRtcIlbcfix_GetLspPoly(
    int16_t *lsp, 
    int32_t *f)  
{
  int32_t tmpW32;
  int i, j;
  int16_t high, low;
  int16_t *lspPtr;
  int32_t *fPtr;

  lspPtr = lsp;
  fPtr = f;
  
  (*fPtr) = (int32_t)16777216;
  fPtr++;

  (*fPtr) = WEBRTC_SPL_MUL((*lspPtr), -1024);
  fPtr++;
  lspPtr+=2;

  for(i=2; i<=5; i++)
  {
    (*fPtr) = fPtr[-2];

    for(j=i; j>1; j--)
    {
      
      high = (int16_t)WEBRTC_SPL_RSHIFT_W32(fPtr[-1], 16);
      low = (int16_t)WEBRTC_SPL_RSHIFT_W32(fPtr[-1]-WEBRTC_SPL_LSHIFT_W32(((int32_t)high),16), 1);

      tmpW32 = WEBRTC_SPL_LSHIFT_W32(WEBRTC_SPL_MUL_16_16(high, (*lspPtr)), 2) +
          WEBRTC_SPL_LSHIFT_W32(WEBRTC_SPL_MUL_16_16_RSFT(low, (*lspPtr), 15), 2);

      (*fPtr) += fPtr[-2];
      (*fPtr) -= tmpW32;
      fPtr--;
    }
    (*fPtr) -= (int32_t)WEBRTC_SPL_LSHIFT_W32((int32_t)(*lspPtr), 10);

    fPtr+=i;
    lspPtr+=2;
  }
  return;
}
