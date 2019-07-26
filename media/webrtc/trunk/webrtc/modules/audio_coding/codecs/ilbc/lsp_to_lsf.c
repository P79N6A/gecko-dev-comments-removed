

















#include "defines.h"
#include "constants.h"





void WebRtcIlbcfix_Lsp2Lsf(
    int16_t *lsp, 
    int16_t *lsf, 

    int16_t m  
                           )
{
  int16_t i, k;
  int16_t diff; 
  int16_t freq; 
  int16_t *lspPtr, *lsfPtr, *cosTblPtr;
  int16_t tmp;

  
  k = 63;

  




  lspPtr = &lsp[9];
  lsfPtr = &lsf[9];
  cosTblPtr=(int16_t*)&WebRtcIlbcfix_kCos[k];
  for(i=m-1; i>=0; i--)
  {
    



    while( (((int32_t)(*cosTblPtr)-(*lspPtr)) < 0)&&(k>0) )
    {
      k-=1;
      cosTblPtr--;
    }

    
    diff = (*lspPtr)-(*cosTblPtr);

    




    
    tmp = (int16_t)WEBRTC_SPL_MUL_16_16_RSFT(WebRtcIlbcfix_kAcosDerivative[k],diff, 11);

    
    freq = (int16_t)WEBRTC_SPL_LSHIFT_W16(k,9)+tmp;

    
    (*lsfPtr) = (int16_t)(((int32_t)freq*25736)>>15);

    lsfPtr--;
    lspPtr--;
  }

  return;
}
