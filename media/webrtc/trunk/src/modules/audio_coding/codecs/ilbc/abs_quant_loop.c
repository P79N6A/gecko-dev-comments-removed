

















#include "defines.h"
#include "constants.h"
#include "sort_sq.h"

void WebRtcIlbcfix_AbsQuantLoop(
    WebRtc_Word16 *syntOutIN,
    WebRtc_Word16 *in_weightedIN,
    WebRtc_Word16 *weightDenumIN,
    WebRtc_Word16 *quantLenIN,
    WebRtc_Word16 *idxVecIN
                                )
{
  int n, k1, k2;
  WebRtc_Word16 index;
  WebRtc_Word32 toQW32;
  WebRtc_Word32 toQ32;
  WebRtc_Word16 tmp16a;
  WebRtc_Word16 xq;

  WebRtc_Word16 *syntOut   = syntOutIN;
  WebRtc_Word16 *in_weighted  = in_weightedIN;
  WebRtc_Word16 *weightDenum  = weightDenumIN;
  WebRtc_Word16 *quantLen  = quantLenIN;
  WebRtc_Word16 *idxVec   = idxVecIN;

  n=0;

  for(k1=0;k1<2;k1++) {
    for(k2=0;k2<quantLen[k1];k2++){

      
      WebRtcSpl_FilterARFastQ12(
          syntOut, syntOut,
          weightDenum, LPC_FILTERORDER+1, 1);

      
      toQW32 = (WebRtc_Word32)(*in_weighted) - (WebRtc_Word32)(*syntOut);

      toQ32 = (((WebRtc_Word32)toQW32)<<2);

      if (toQ32 > 32767) {
        toQ32 = (WebRtc_Word32) 32767;
      } else if (toQ32 < -32768) {
        toQ32 = (WebRtc_Word32) -32768;
      }

      
      if (toQW32<(-7577)) {
        
        index=0;
      } else if (toQW32>8151) {
        
        index=7;
      } else {
        


        WebRtcIlbcfix_SortSq(&xq, &index,
                             (WebRtc_Word16)toQ32,
                             WebRtcIlbcfix_kStateSq3, 8);
      }

      
      (*idxVec++) = index;

      
      tmp16a = ((WebRtcIlbcfix_kStateSq3[index] + 2 ) >> 2);

      *syntOut     = (WebRtc_Word16) (tmp16a + (WebRtc_Word32)(*in_weighted) - toQW32);

      n++;
      syntOut++; in_weighted++;
    }
    
    weightDenum += 11;
  }
}
