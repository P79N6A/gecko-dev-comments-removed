

















#include "defines.h"
#include "constants.h"






void WebRtcIlbcfix_CreateAugmentedVec(
    WebRtc_Word16 index,  
    WebRtc_Word16 *buffer,  

    WebRtc_Word16 *cbVec  
                                      ) {
  WebRtc_Word16 ilow;
  WebRtc_Word16 *ppo, *ppi;
  WebRtc_Word16 cbVecTmp[4];

  ilow = index-4;

  
  ppo = buffer-index;
  WEBRTC_SPL_MEMCPY_W16(cbVec, ppo, index);

  
  ppo = buffer - 4;
  ppi = buffer - index - 4;

  


  WebRtcSpl_ElementwiseVectorMult(&cbVec[ilow], ppi, WebRtcIlbcfix_kAlpha, 4, 15);
  WebRtcSpl_ReverseOrderMultArrayElements(cbVecTmp, ppo, &WebRtcIlbcfix_kAlpha[3], 4, 15);
  WebRtcSpl_AddVectorsAndShift(&cbVec[ilow], &cbVec[ilow], cbVecTmp, 4, 0);

  
  ppo = buffer - index;
  WEBRTC_SPL_MEMCPY_W16(cbVec+index,ppo,(SUBL-index));
}
