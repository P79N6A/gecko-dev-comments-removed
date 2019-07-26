

















#include "defines.h"
#include "constants.h"






void WebRtcIlbcfix_CreateAugmentedVec(
    int16_t index,  
    int16_t *buffer,  

    int16_t *cbVec  
                                      ) {
  int16_t ilow;
  int16_t *ppo, *ppi;
  int16_t cbVecTmp[4];

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
