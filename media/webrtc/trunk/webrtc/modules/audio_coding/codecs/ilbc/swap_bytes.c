

















#include "defines.h"





void WebRtcIlbcfix_SwapBytes(
    const uint16_t* input,   
    int16_t wordLength,      
    uint16_t* output         
                              ) {
  int k;
  for (k = wordLength; k > 0; k--) {
    *output++ = (*input >> 8)|(*input << 8);
    input++;
  }
}
