

















#include "defines.h"




















void WebRtcIlbcfix_GetLspPoly(
    WebRtc_Word16 *lsp, 
    WebRtc_Word32 *f)  
{
  WebRtc_Word32 tmpW32;
  int i, j;
  WebRtc_Word16 high, low;
  WebRtc_Word16 *lspPtr;
  WebRtc_Word32 *fPtr;

  lspPtr = lsp;
  fPtr = f;
  
  (*fPtr) = (WebRtc_Word32)16777216;
  fPtr++;

  (*fPtr) = WEBRTC_SPL_MUL((*lspPtr), -1024);
  fPtr++;
  lspPtr+=2;

  for(i=2; i<=5; i++)
  {
    (*fPtr) = fPtr[-2];

    for(j=i; j>1; j--)
    {
      
      high = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(fPtr[-1], 16);
      low = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(fPtr[-1]-WEBRTC_SPL_LSHIFT_W32(((WebRtc_Word32)high),16), 1);

      tmpW32 = WEBRTC_SPL_LSHIFT_W32(WEBRTC_SPL_MUL_16_16(high, (*lspPtr)), 2) +
          WEBRTC_SPL_LSHIFT_W32(WEBRTC_SPL_MUL_16_16_RSFT(low, (*lspPtr), 15), 2);

      (*fPtr) += fPtr[-2];
      (*fPtr) -= tmpW32;
      fPtr--;
    }
    (*fPtr) -= (WebRtc_Word32)WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)(*lspPtr), 10);

    fPtr+=i;
    lspPtr+=2;
  }
  return;
}
