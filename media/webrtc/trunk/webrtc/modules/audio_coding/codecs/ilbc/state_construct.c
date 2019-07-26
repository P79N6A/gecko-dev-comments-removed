

















#include "defines.h"
#include "constants.h"





void WebRtcIlbcfix_StateConstruct(
    int16_t idxForMax,   

    int16_t *idxVec,   
    int16_t *syntDenum,  
    int16_t *Out_fix,  
    int16_t len    
                                  ) {
  int k;
  int16_t maxVal;
  int16_t *tmp1, *tmp2, *tmp3;
  
  int16_t numerator[1+LPC_FILTERORDER];
  int16_t sampleValVec[2*STATE_SHORT_LEN_30MS+LPC_FILTERORDER];
  int16_t sampleMaVec[2*STATE_SHORT_LEN_30MS+LPC_FILTERORDER];
  int16_t *sampleVal = &sampleValVec[LPC_FILTERORDER];
  int16_t *sampleMa = &sampleMaVec[LPC_FILTERORDER];
  int16_t *sampleAr = &sampleValVec[LPC_FILTERORDER];

  

  for (k=0; k<LPC_FILTERORDER+1; k++){
    numerator[k] = syntDenum[LPC_FILTERORDER-k];
  }

  

  maxVal = WebRtcIlbcfix_kFrgQuantMod[idxForMax];

  
  tmp1 = sampleVal;
  tmp2 = &idxVec[len-1];

  if (idxForMax<37) {
    for(k=0; k<len; k++){
      

      (*tmp1) = (int16_t) ((WEBRTC_SPL_MUL_16_16(maxVal,WebRtcIlbcfix_kStateSq3[(*tmp2)])+(int32_t)2097152) >> 22);
      tmp1++;
      tmp2--;
    }
  } else if (idxForMax<59) {
    for(k=0; k<len; k++){
      

      (*tmp1) = (int16_t) ((WEBRTC_SPL_MUL_16_16(maxVal,WebRtcIlbcfix_kStateSq3[(*tmp2)])+(int32_t)262144) >> 19);
      tmp1++;
      tmp2--;
    }
  } else {
    for(k=0; k<len; k++){
      

      (*tmp1) = (int16_t) ((WEBRTC_SPL_MUL_16_16(maxVal,WebRtcIlbcfix_kStateSq3[(*tmp2)])+(int32_t)65536) >> 17);
      tmp1++;
      tmp2--;
    }
  }

  
  WebRtcSpl_MemSetW16(&sampleVal[len], 0, len);

  

  
  WebRtcSpl_MemSetW16(sampleValVec, 0, (LPC_FILTERORDER));

  
  WebRtcSpl_FilterMAFastQ12(
      sampleVal, sampleMa,
      numerator, LPC_FILTERORDER+1, (int16_t)(len + LPC_FILTERORDER));
  WebRtcSpl_MemSetW16(&sampleMa[len + LPC_FILTERORDER], 0, (len - LPC_FILTERORDER));
  WebRtcSpl_FilterARFastQ12(
      sampleMa, sampleAr,
      syntDenum, LPC_FILTERORDER+1, (int16_t)(2*len));

  tmp1 = &sampleAr[len-1];
  tmp2 = &sampleAr[2*len-1];
  tmp3 = Out_fix;
  for(k=0;k<len;k++){
    (*tmp3) = (*tmp1) + (*tmp2);
    tmp1--;
    tmp2--;
    tmp3++;
  }
}
