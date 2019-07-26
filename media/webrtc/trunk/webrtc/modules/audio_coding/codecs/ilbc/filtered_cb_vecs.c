

















#include "defines.h"
#include "constants.h"







void WebRtcIlbcfix_FilteredCbVecs(
    int16_t *cbvectors, 
    int16_t *CBmem,  

    int lMem,  
    int16_t samples    
                                  ) {

  
  WebRtcSpl_MemSetW16(CBmem+lMem, 0, CB_HALFFILTERLEN);
  WebRtcSpl_MemSetW16(CBmem-CB_HALFFILTERLEN, 0, CB_HALFFILTERLEN);
  WebRtcSpl_MemSetW16(cbvectors, 0, lMem-samples);

  

  WebRtcSpl_FilterMAFastQ12(
      CBmem+CB_HALFFILTERLEN+lMem-samples, cbvectors+lMem-samples,
      (int16_t*)WebRtcIlbcfix_kCbFiltersRev, CB_FILTERLEN, samples);

  return;
}
