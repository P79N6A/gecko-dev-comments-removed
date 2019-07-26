

















#include "defines.h"
#include "constants.h"





void WebRtcIlbcfix_Lsp2Lsf(
    WebRtc_Word16 *lsp, 
    WebRtc_Word16 *lsf, 

    WebRtc_Word16 m  
                           )
{
  WebRtc_Word16 i, k;
  WebRtc_Word16 diff; 
  WebRtc_Word16 freq; 
  WebRtc_Word16 *lspPtr, *lsfPtr, *cosTblPtr;
  WebRtc_Word16 tmp;

  
  k = 63;

  




  lspPtr = &lsp[9];
  lsfPtr = &lsf[9];
  cosTblPtr=(WebRtc_Word16*)&WebRtcIlbcfix_kCos[k];
  for(i=m-1; i>=0; i--)
  {
    



    while( (((WebRtc_Word32)(*cosTblPtr)-(*lspPtr)) < 0)&&(k>0) )
    {
      k-=1;
      cosTblPtr--;
    }

    
    diff = (*lspPtr)-(*cosTblPtr);

    




    
    tmp = (WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT(WebRtcIlbcfix_kAcosDerivative[k],diff, 11);

    
    freq = (WebRtc_Word16)WEBRTC_SPL_LSHIFT_W16(k,9)+tmp;

    
    (*lsfPtr) = (WebRtc_Word16)(((WebRtc_Word32)freq*25736)>>15);

    lsfPtr--;
    lspPtr--;
  }

  return;
}
