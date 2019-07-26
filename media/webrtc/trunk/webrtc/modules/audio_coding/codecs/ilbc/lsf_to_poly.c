

















#include "defines.h"
#include "lsf_to_lsp.h"
#include "get_lsp_poly.h"
#include "constants.h"

void WebRtcIlbcfix_Lsf2Poly(
    WebRtc_Word16 *a,     
    WebRtc_Word16 *lsf    
                            ) {
  WebRtc_Word32 f[2][6]; 

  WebRtc_Word32 *f1ptr, *f2ptr;
  WebRtc_Word16 *a1ptr, *a2ptr;
  WebRtc_Word32 tmpW32;
  WebRtc_Word16 lsp[10];
  int i;

  
  WebRtcIlbcfix_Lsf2Lsp(lsf, lsp, LPC_FILTERORDER);

  
  f1ptr=f[0];
  f2ptr=f[1];
  WebRtcIlbcfix_GetLspPoly(&lsp[0],f1ptr);
  WebRtcIlbcfix_GetLspPoly(&lsp[1],f2ptr);

  



  f1ptr=&f[0][5];
  f2ptr=&f[1][5];
  for (i=5; i>0; i--)
  {
    (*f1ptr) += (*(f1ptr-1));
    (*f2ptr) -= (*(f2ptr-1));
    f1ptr--;
    f2ptr--;
  }

  






  a[0]=4096;
  a1ptr=&a[1];
  a2ptr=&a[10];
  f1ptr=&f[0][1];
  f2ptr=&f[1][1];
  for (i=5; i>0; i--)
  {
    tmpW32 = (*f1ptr) + (*f2ptr);
    (*a1ptr) = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32((tmpW32+4096),13);

    tmpW32 = (*f1ptr) - (*f2ptr);
    (*a2ptr) = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32((tmpW32+4096),13);

    a1ptr++;
    a2ptr--;
    f1ptr++;
    f2ptr++;
  }

  return;
}
