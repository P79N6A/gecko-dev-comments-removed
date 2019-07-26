

















#include "defines.h"
#include "gain_dequant.h"
#include "get_cd_vec.h"





void WebRtcIlbcfix_CbConstruct(
    int16_t *decvector,  
    int16_t *index,   
    int16_t *gain_index,  
    int16_t *mem,   
    int16_t lMem,   
    int16_t veclen   
                               ){
  int j;
  int16_t gain[CB_NSTAGES];
  
  int16_t cbvec0[SUBL];
  int16_t cbvec1[SUBL];
  int16_t cbvec2[SUBL];
  int32_t a32;
  int16_t *gainPtr;

  

  gain[0] = WebRtcIlbcfix_GainDequant(gain_index[0], 16384, 0);
  gain[1] = WebRtcIlbcfix_GainDequant(gain_index[1], gain[0], 1);
  gain[2] = WebRtcIlbcfix_GainDequant(gain_index[2], gain[1], 2);

  

  
  WebRtcIlbcfix_GetCbVec(cbvec0, mem, index[0], lMem, veclen);
  WebRtcIlbcfix_GetCbVec(cbvec1, mem, index[1], lMem, veclen);
  WebRtcIlbcfix_GetCbVec(cbvec2, mem, index[2], lMem, veclen);

  gainPtr = &gain[0];
  for (j=0;j<veclen;j++) {
    a32  = WEBRTC_SPL_MUL_16_16(*gainPtr++, cbvec0[j]);
    a32 += WEBRTC_SPL_MUL_16_16(*gainPtr++, cbvec1[j]);
    a32 += WEBRTC_SPL_MUL_16_16(*gainPtr, cbvec2[j]);
    gainPtr -= 2;
    decvector[j] = (int16_t) WEBRTC_SPL_RSHIFT_W32(a32 + 8192, 14);
  }

  return;
}
