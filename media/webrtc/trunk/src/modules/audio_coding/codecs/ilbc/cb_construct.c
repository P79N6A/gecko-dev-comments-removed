

















#include "defines.h"
#include "gain_dequant.h"
#include "get_cd_vec.h"





void WebRtcIlbcfix_CbConstruct(
    WebRtc_Word16 *decvector,  
    WebRtc_Word16 *index,   
    WebRtc_Word16 *gain_index,  
    WebRtc_Word16 *mem,   
    WebRtc_Word16 lMem,   
    WebRtc_Word16 veclen   
                               ){
  int j;
  WebRtc_Word16 gain[CB_NSTAGES];
  
  WebRtc_Word16 cbvec0[SUBL];
  WebRtc_Word16 cbvec1[SUBL];
  WebRtc_Word16 cbvec2[SUBL];
  WebRtc_Word32 a32;
  WebRtc_Word16 *gainPtr;

  

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
    decvector[j] = (WebRtc_Word16) WEBRTC_SPL_RSHIFT_W32(a32 + 8192, 14);
  }

  return;
}
