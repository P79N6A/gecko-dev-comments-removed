

















#include "defines.h"
#include "constants.h"

int32_t WebRtcIlbcfix_Smooth_odata(
    int16_t *odata,
    int16_t *psseq,
    int16_t *surround,
    int16_t C)
{
  int i;

  int16_t err;
  int32_t errs;

  for(i=0;i<80;i++) {
    odata[i]= (int16_t)WEBRTC_SPL_RSHIFT_W32(
        (WEBRTC_SPL_MUL_16_16(C, surround[i])+1024), 11);
  }

  errs=0;
  for(i=0;i<80;i++) {
    err=(int16_t)WEBRTC_SPL_RSHIFT_W16((psseq[i]-odata[i]), 3);
    errs+=WEBRTC_SPL_MUL_16_16(err, err); 
  }

  return errs;
}
