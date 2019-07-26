

















#include "defines.h"
#include "split_vq.h"
#include "constants.h"





void WebRtcIlbcfix_SimpleLsfQ(
    int16_t *lsfdeq, 

    int16_t *index, 
    int16_t *lsf, 

    int16_t lpc_n 
                              ){

  
  WebRtcIlbcfix_SplitVq( lsfdeq, index, lsf,
                         (int16_t*)WebRtcIlbcfix_kLsfCb, (int16_t*)WebRtcIlbcfix_kLsfDimCb, (int16_t*)WebRtcIlbcfix_kLsfSizeCb);

  if (lpc_n==2) {
    
    WebRtcIlbcfix_SplitVq( lsfdeq + LPC_FILTERORDER, index + LSF_NSPLIT,
                           lsf + LPC_FILTERORDER, (int16_t*)WebRtcIlbcfix_kLsfCb,
                           (int16_t*)WebRtcIlbcfix_kLsfDimCb, (int16_t*)WebRtcIlbcfix_kLsfSizeCb);
  }
  return;
}
