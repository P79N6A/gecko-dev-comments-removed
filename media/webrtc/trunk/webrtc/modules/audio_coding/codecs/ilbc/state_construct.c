

















#include "defines.h"
#include "constants.h"





void WebRtcIlbcfix_StateConstruct(
    WebRtc_Word16 idxForMax,   

    WebRtc_Word16 *idxVec,   
    WebRtc_Word16 *syntDenum,  
    WebRtc_Word16 *Out_fix,  
    WebRtc_Word16 len    
                                  ) {
  int k;
  WebRtc_Word16 maxVal;
  WebRtc_Word16 *tmp1, *tmp2, *tmp3;
  
  WebRtc_Word16 numerator[1+LPC_FILTERORDER];
  WebRtc_Word16 sampleValVec[2*STATE_SHORT_LEN_30MS+LPC_FILTERORDER];
  WebRtc_Word16 sampleMaVec[2*STATE_SHORT_LEN_30MS+LPC_FILTERORDER];
  WebRtc_Word16 *sampleVal = &sampleValVec[LPC_FILTERORDER];
  WebRtc_Word16 *sampleMa = &sampleMaVec[LPC_FILTERORDER];
  WebRtc_Word16 *sampleAr = &sampleValVec[LPC_FILTERORDER];

  

  for (k=0; k<LPC_FILTERORDER+1; k++){
    numerator[k] = syntDenum[LPC_FILTERORDER-k];
  }

  

  maxVal = WebRtcIlbcfix_kFrgQuantMod[idxForMax];

  
  tmp1 = sampleVal;
  tmp2 = &idxVec[len-1];

  if (idxForMax<37) {
    for(k=0; k<len; k++){
      

      (*tmp1) = (WebRtc_Word16) ((WEBRTC_SPL_MUL_16_16(maxVal,WebRtcIlbcfix_kStateSq3[(*tmp2)])+(WebRtc_Word32)2097152) >> 22);
      tmp1++;
      tmp2--;
    }
  } else if (idxForMax<59) {
    for(k=0; k<len; k++){
      

      (*tmp1) = (WebRtc_Word16) ((WEBRTC_SPL_MUL_16_16(maxVal,WebRtcIlbcfix_kStateSq3[(*tmp2)])+(WebRtc_Word32)262144) >> 19);
      tmp1++;
      tmp2--;
    }
  } else {
    for(k=0; k<len; k++){
      

      (*tmp1) = (WebRtc_Word16) ((WEBRTC_SPL_MUL_16_16(maxVal,WebRtcIlbcfix_kStateSq3[(*tmp2)])+(WebRtc_Word32)65536) >> 17);
      tmp1++;
      tmp2--;
    }
  }

  
  WebRtcSpl_MemSetW16(&sampleVal[len], 0, len);

  

  
  WebRtcSpl_MemSetW16(sampleValVec, 0, (LPC_FILTERORDER));

  
  WebRtcSpl_FilterMAFastQ12(
      sampleVal, sampleMa,
      numerator, LPC_FILTERORDER+1, (WebRtc_Word16)(len + LPC_FILTERORDER));
  WebRtcSpl_MemSetW16(&sampleMa[len + LPC_FILTERORDER], 0, (len - LPC_FILTERORDER));
  WebRtcSpl_FilterARFastQ12(
      sampleMa, sampleAr,
      syntDenum, LPC_FILTERORDER+1, (WebRtc_Word16)(2*len));

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
