

















#include "defines.h"





void WebRtcIlbcfix_MyCorr(
    WebRtc_Word32 *corr,  
    WebRtc_Word16 *seq1,  
    WebRtc_Word16 dim1,  
    const WebRtc_Word16 *seq2, 
    WebRtc_Word16 dim2   
                          ){
  WebRtc_Word16 max, scale, loops;

  


  max=WebRtcSpl_MaxAbsValueW16(seq1, dim1);
  scale=WebRtcSpl_GetSizeInBits(max);

  scale = (WebRtc_Word16)(WEBRTC_SPL_MUL_16_16(2,scale)-26);
  if (scale<0) {
    scale=0;
  }

  loops=dim1-dim2+1;

  
  WebRtcSpl_CrossCorrelation(corr, (WebRtc_Word16*)seq2, seq1, dim2, loops, scale, 1);

  return;
}
