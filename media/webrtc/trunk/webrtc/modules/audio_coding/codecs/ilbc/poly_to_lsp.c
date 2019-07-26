

















#include "defines.h"
#include "constants.h"
#include "chebyshev.h"






void WebRtcIlbcfix_Poly2Lsp(
    int16_t *a,  
    int16_t *lsp, 
    int16_t *old_lsp 

                            ) {
  int16_t f[2][6]; 
  int16_t *a_i_ptr, *a_10mi_ptr;
  int16_t *f1ptr, *f2ptr;
  int32_t tmpW32;
  int16_t x, y, xlow, ylow, xmid, ymid, xhigh, yhigh, xint;
  int16_t shifts, sign;
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
    (*(f1ptr+1)) = (int16_t)(WEBRTC_SPL_RSHIFT_W32(((int32_t)(*a_i_ptr)+(*a_10mi_ptr)), 2) - (*f1ptr));
    (*(f2ptr+1)) = (int16_t)(WEBRTC_SPL_RSHIFT_W32(((int32_t)(*a_i_ptr)-(*a_10mi_ptr)), 2) + (*f2ptr));
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
        shifts = (int16_t)WebRtcSpl_NormW32(y)-16;
        y = WEBRTC_SPL_LSHIFT_W16(y, shifts);
        y = (int16_t)WebRtcSpl_DivW32W16(536838144, y); 

        tmpW32 = WEBRTC_SPL_MUL_16_16_RSFT(x, y, (19-shifts));

        
        y = (int16_t)(tmpW32&0xFFFF);

        if (sign < 0) {
          y = -y;
        }
        
        tmpW32 = WEBRTC_SPL_MUL_16_16_RSFT(ylow, y, 10);
        xint = xlow-(int16_t)(tmpW32&0xFFFF);
      }

      
      lsp[foundFreqs] = (int16_t)xint;
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
