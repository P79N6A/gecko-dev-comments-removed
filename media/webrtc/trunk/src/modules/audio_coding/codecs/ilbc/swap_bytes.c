

















#include "defines.h"





void WebRtcIlbcfix_SwapBytes(
    const WebRtc_UWord16* input,   
    WebRtc_Word16 wordLength,      
    WebRtc_UWord16* output         
                              ) {
  int k;
  for (k = wordLength; k > 0; k--) {
    *output++ = (*input >> 8)|(*input << 8);
    input++;
  }
}
