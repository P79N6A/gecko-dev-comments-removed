

















#include "defines.h"
#include "split_vq.h"
#include "constants.h"





void WebRtcIlbcfix_SimpleLsfQ(
    WebRtc_Word16 *lsfdeq, 

    WebRtc_Word16 *index, 
    WebRtc_Word16 *lsf, 

    WebRtc_Word16 lpc_n 
                              ){

  
  WebRtcIlbcfix_SplitVq( lsfdeq, index, lsf,
                         (WebRtc_Word16*)WebRtcIlbcfix_kLsfCb, (WebRtc_Word16*)WebRtcIlbcfix_kLsfDimCb, (WebRtc_Word16*)WebRtcIlbcfix_kLsfSizeCb);

  if (lpc_n==2) {
    
    WebRtcIlbcfix_SplitVq( lsfdeq + LPC_FILTERORDER, index + LSF_NSPLIT,
                           lsf + LPC_FILTERORDER, (WebRtc_Word16*)WebRtcIlbcfix_kLsfCb,
                           (WebRtc_Word16*)WebRtcIlbcfix_kLsfDimCb, (WebRtc_Word16*)WebRtcIlbcfix_kLsfSizeCb);
  }
  return;
}
