

















#include "defines.h"
#include "constants.h"
#include "get_sync_seq.h"
#include "smooth.h"






void WebRtcIlbcfix_Enhancer(
    WebRtc_Word16 *odata,   
    WebRtc_Word16 *idata,   
    WebRtc_Word16 idatal,   
    WebRtc_Word16 centerStartPos, 
    WebRtc_Word16 *period,   
    WebRtc_Word16 *plocs,   
    WebRtc_Word16 periodl   
                            ){
  
  WebRtc_Word16 surround[ENH_BLOCKL];

  WebRtcSpl_MemSetW16(surround, 0, ENH_BLOCKL);

  

  WebRtcIlbcfix_GetSyncSeq(idata, idatal, centerStartPos, period, plocs,
                           periodl, ENH_HL, surround);

  

  WebRtcIlbcfix_Smooth(odata, idata+centerStartPos, surround);
}
