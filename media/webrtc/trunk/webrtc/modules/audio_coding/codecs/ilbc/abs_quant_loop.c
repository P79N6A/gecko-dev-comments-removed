

















#include "defines.h"
#include "constants.h"
#include "sort_sq.h"

void WebRtcIlbcfix_AbsQuantLoop(int16_t *syntOutIN, int16_t *in_weightedIN,
                                int16_t *weightDenumIN, int16_t *quantLenIN,
                                int16_t *idxVecIN ) {
  int n, k1, k2;
  int16_t index;
  int32_t toQW32;
  int32_t toQ32;
  int16_t tmp16a;
  int16_t xq;

  int16_t *syntOut   = syntOutIN;
  int16_t *in_weighted  = in_weightedIN;
  int16_t *weightDenum  = weightDenumIN;
  int16_t *quantLen  = quantLenIN;
  int16_t *idxVec   = idxVecIN;

  n=0;

  for(k1=0;k1<2;k1++) {
    for(k2=0;k2<quantLen[k1];k2++){

      
      WebRtcSpl_FilterARFastQ12(
          syntOut, syntOut,
          weightDenum, LPC_FILTERORDER+1, 1);

      
      toQW32 = (int32_t)(*in_weighted) - (int32_t)(*syntOut);

      toQ32 = (((int32_t)toQW32)<<2);

      if (toQ32 > 32767) {
        toQ32 = (int32_t) 32767;
      } else if (toQ32 < -32768) {
        toQ32 = (int32_t) -32768;
      }

      
      if (toQW32<(-7577)) {
        
        index=0;
      } else if (toQW32>8151) {
        
        index=7;
      } else {
        


        WebRtcIlbcfix_SortSq(&xq, &index,
                             (int16_t)toQ32,
                             WebRtcIlbcfix_kStateSq3, 8);
      }

      
      (*idxVec++) = index;

      
      tmp16a = ((WebRtcIlbcfix_kStateSq3[index] + 2 ) >> 2);

      *syntOut     = (int16_t) (tmp16a + (int32_t)(*in_weighted) - toQW32);

      n++;
      syntOut++; in_weighted++;
    }
    
    weightDenum += 11;
  }
}
