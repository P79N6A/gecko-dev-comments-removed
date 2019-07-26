

















#include "defines.h"





void WebRtcIlbcfix_MyCorr(
    int32_t *corr,  
    int16_t *seq1,  
    int16_t dim1,  
    const int16_t *seq2, 
    int16_t dim2   
                          ){
  int16_t max, scale, loops;

  


  max=WebRtcSpl_MaxAbsValueW16(seq1, dim1);
  scale=WebRtcSpl_GetSizeInBits(max);

  scale = (int16_t)(WEBRTC_SPL_MUL_16_16(2,scale)-26);
  if (scale<0) {
    scale=0;
  }

  loops=dim1-dim2+1;

  
  WebRtcSpl_CrossCorrelation(corr, (int16_t*)seq2, seq1, dim2, loops, scale, 1);

  return;
}
