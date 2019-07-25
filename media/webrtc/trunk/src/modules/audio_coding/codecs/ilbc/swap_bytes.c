

















#include "defines.h"





void WebRtcIlbcfix_SwapBytes(
    WebRtc_UWord16 *sequence,   
    WebRtc_Word16 wordLength   
                              ) {
  int k;
  WebRtc_UWord16 temp=0;
  for( k=wordLength; k>0; k-- ) {
    temp = (*sequence >> 8)|(*sequence << 8);
    *sequence++ = temp;
    
  }
}
