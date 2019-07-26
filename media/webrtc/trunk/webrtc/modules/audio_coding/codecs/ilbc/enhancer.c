

















#include "defines.h"
#include "constants.h"
#include "get_sync_seq.h"
#include "smooth.h"






void WebRtcIlbcfix_Enhancer(
    int16_t *odata,   
    int16_t *idata,   
    int16_t idatal,   
    int16_t centerStartPos, 
    int16_t *period,   
    int16_t *plocs,   
    int16_t periodl   
                            ){
  
  int16_t surround[ENH_BLOCKL];

  WebRtcSpl_MemSetW16(surround, 0, ENH_BLOCKL);

  

  WebRtcIlbcfix_GetSyncSeq(idata, idatal, centerStartPos, period, plocs,
                           periodl, ENH_HL, surround);

  

  WebRtcIlbcfix_Smooth(odata, idata+centerStartPos, surround);
}
