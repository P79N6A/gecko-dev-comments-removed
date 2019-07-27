















#include "settings.h"
#include "signal_processing_library.h"
#include "webrtc/typedefs.h"




void WebRtcIsacfix_FilterArLoop(int16_t* ar_g_Q0,     
                                int16_t* ar_f_Q0,     
                                int16_t* cth_Q15,     
                                int16_t* sth_Q15,     
                                int16_t order_coef) { 
  int n = 0;

  for (n = 0; n < HALF_SUBFRAMELEN - 1; n++) {
    int k = 0;
    int16_t tmpAR = 0;
    int32_t tmp32 = 0;
    int32_t tmp32_2 = 0;

    tmpAR = ar_f_Q0[n + 1];
    for (k = order_coef - 1; k >= 0; k--) {
      tmp32 = (cth_Q15[k] * tmpAR - sth_Q15[k] * ar_g_Q0[k] + 16384) >> 15;
      tmp32_2 = (sth_Q15[k] * tmpAR + cth_Q15[k] * ar_g_Q0[k] + 16384) >> 15;
      tmpAR   = (int16_t)WebRtcSpl_SatW32ToW16(tmp32);
      ar_g_Q0[k + 1] = (int16_t)WebRtcSpl_SatW32ToW16(tmp32_2);
    }
    ar_f_Q0[n + 1] = tmpAR;
    ar_g_Q0[0] = tmpAR;
  }
}
