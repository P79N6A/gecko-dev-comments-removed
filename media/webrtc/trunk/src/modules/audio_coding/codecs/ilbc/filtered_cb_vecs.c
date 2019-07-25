

















#include "defines.h"
#include "constants.h"







void WebRtcIlbcfix_FilteredCbVecs(
    WebRtc_Word16 *cbvectors, 
    WebRtc_Word16 *CBmem,  

    int lMem,  
    WebRtc_Word16 samples    
                                  ) {

  
  WebRtcSpl_MemSetW16(CBmem+lMem, 0, CB_HALFFILTERLEN);
  WebRtcSpl_MemSetW16(CBmem-CB_HALFFILTERLEN, 0, CB_HALFFILTERLEN);
  WebRtcSpl_MemSetW16(cbvectors, 0, lMem-samples);

  

  WebRtcSpl_FilterMAFastQ12(
      CBmem+CB_HALFFILTERLEN+lMem-samples, cbvectors+lMem-samples,
      (WebRtc_Word16*)WebRtcIlbcfix_kCbFiltersRev, CB_FILTERLEN, samples);

  return;
}
