

















#include "defines.h"
#include "constants.h"
#include "chebyshev.h"






void WebRtcIlbcfix_Poly2Lsp(
    WebRtc_Word16 *a,  
    WebRtc_Word16 *lsp, 
    WebRtc_Word16 *old_lsp 

                            ) {
  WebRtc_Word16 f[2][6]; 
  WebRtc_Word16 *a_i_ptr, *a_10mi_ptr;
  WebRtc_Word16 *f1ptr, *f2ptr;
  WebRtc_Word32 tmpW32;
  WebRtc_Word16 x, y, xlow, ylow, xmid, ymid, xhigh, yhigh, xint;
  WebRtc_Word16 shifts, sign;
  int i, j;
  int foundFreqs;
  int fi_select;

  







  a_i_ptr = a + 1;
  a_10mi_ptr = a + 10;
  f1ptr = f[0];
  f2ptr = f[1];
  (*f1ptr) = 1024; 
  (*f2ptr) = 1024; 
  for (i = 0; i < 5; i++) {
    (*(f1ptr+1)) = (WebRtc_Word16)(WEBRTC_SPL_RSHIFT_W32(((WebRtc_Word32)(*a_i_ptr)+(*a_10mi_ptr)), 2) - (*f1ptr));
    (*(f2ptr+1)) = (WebRtc_Word16)(WEBRTC_SPL_RSHIFT_W32(((WebRtc_Word32)(*a_i_ptr)-(*a_10mi_ptr)), 2) + (*f2ptr));
    a_i_ptr++;
    a_10mi_ptr--;
    f1ptr++;
    f2ptr++;
  }

  



  fi_select = 0; 

  foundFreqs = 0;

  xlow = WebRtcIlbcfix_kCosGrid[0];
  ylow = WebRtcIlbcfix_Chebyshev(xlow, f[fi_select]);

  





  for (j = 1; j < COS_GRID_POINTS && foundFreqs < 10; j++) {
    xhigh = xlow;
    yhigh = ylow;
    xlow = WebRtcIlbcfix_kCosGrid[j];
    ylow = WebRtcIlbcfix_Chebyshev(xlow, f[fi_select]);

    if (WEBRTC_SPL_MUL_16_16(ylow, yhigh) <= 0) {
      
      for (i = 0; i < 4; i++) {
        
        xmid = WEBRTC_SPL_RSHIFT_W16(xlow, 1) + WEBRTC_SPL_RSHIFT_W16(xhigh, 1);
        ymid = WebRtcIlbcfix_Chebyshev(xmid, f[fi_select]);

        if (WEBRTC_SPL_MUL_16_16(ylow, ymid) <= 0) {
          yhigh = ymid;
          xhigh = xmid;
        } else {
          ylow = ymid;
          xlow = xmid;
        }
      }

      




      x = xhigh - xlow;
      y = yhigh - ylow;

      if (y == 0) {
        xint = xlow;
      } else {
        sign = y;
        y = WEBRTC_SPL_ABS_W16(y);
        shifts = (WebRtc_Word16)WebRtcSpl_NormW32(y)-16;
        y = WEBRTC_SPL_LSHIFT_W16(y, shifts);
        y = (WebRtc_Word16)WebRtcSpl_DivW32W16(536838144, y); 

        tmpW32 = WEBRTC_SPL_MUL_16_16_RSFT(x, y, (19-shifts));

        
        y = (WebRtc_Word16)(tmpW32&0xFFFF);

        if (sign < 0) {
          y = -y;
        }
        
        tmpW32 = WEBRTC_SPL_MUL_16_16_RSFT(ylow, y, 10);
        xint = xlow-(WebRtc_Word16)(tmpW32&0xFFFF);
      }

      
      lsp[foundFreqs] = (WebRtc_Word16)xint;
      foundFreqs++;

      
      if (foundFreqs<10) {
        xlow = xint;
        
        fi_select = ((fi_select+1)&0x1);

        ylow = WebRtcIlbcfix_Chebyshev(xlow, f[fi_select]);
      }
    }
  }

  
  if (foundFreqs < 10) {
    WEBRTC_SPL_MEMCPY_W16(lsp, old_lsp, 10);
  }
  return;
}
