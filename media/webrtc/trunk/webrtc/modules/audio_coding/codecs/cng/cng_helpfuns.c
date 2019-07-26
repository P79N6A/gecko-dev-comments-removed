









#include "cng_helpfuns.h"

#include "signal_processing_library.h"
#include "typedefs.h"
#include "webrtc_cng.h"


void WebRtcCng_K2a16(int16_t* k, int useOrder, int16_t* a) {
  int16_t any[WEBRTC_SPL_MAX_LPC_ORDER + 1];
  int16_t *aptr, *aptr2, *anyptr;
  const int16_t *kptr;
  int m, i;

  kptr = k;
  *a = 4096;  
  *any = *a;
  a[1] = (*k + 4) >> 3;
  for (m = 1; m < useOrder; m++) {
    kptr++;
    aptr = a;
    aptr++;
    aptr2 = &a[m];
    anyptr = any;
    anyptr++;

    any[m + 1] = (*kptr + 4) >> 3;
    for (i = 0; i < m; i++) {
      *anyptr++ = (*aptr++) +
          (WebRtc_Word16)(
              (((WebRtc_Word32)(*aptr2--) * (WebRtc_Word32) * kptr) + 16384)
                  >> 15);
    }

    aptr = a;
    anyptr = any;
    for (i = 0; i < (m + 2); i++) {
      *aptr++ = *anyptr++;
    }
  }
}
