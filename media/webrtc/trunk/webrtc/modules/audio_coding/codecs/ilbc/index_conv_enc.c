

















#include "defines.h"




void WebRtcIlbcfix_IndexConvEnc(
    int16_t *index   
                                ){
  int k;

  for (k=4;k<6;k++) {
    



    if ((index[k]>=108)&&(index[k]<172)) {
      index[k]-=64;
    } else if (index[k]>=236) {
      index[k]-=128;
    } else {
      
    }
  }
}
