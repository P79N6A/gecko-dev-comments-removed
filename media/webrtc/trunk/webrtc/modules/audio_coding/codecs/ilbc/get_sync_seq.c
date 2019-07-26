

















#include "defines.h"
#include "constants.h"
#include "refiner.h"
#include "nearest_neighbor.h"





void WebRtcIlbcfix_GetSyncSeq(
    int16_t *idata,   
    int16_t idatal,   
    int16_t centerStartPos, 
    int16_t *period,   
    int16_t *plocs,   
    int16_t periodl,   
    int16_t hl,    
    int16_t *surround  

                              ){
  int16_t i,centerEndPos,q;
  
  int16_t lagBlock[2*ENH_HL+1];
  int16_t blockStartPos[2*ENH_HL+1]; 
  int16_t plocs2[ENH_PLOCSL];

  centerEndPos=centerStartPos+ENH_BLOCKL-1;

  

  WebRtcIlbcfix_NearestNeighbor(lagBlock+hl,plocs,
                                (int16_t)WEBRTC_SPL_MUL_16_16(2, (centerStartPos+centerEndPos)),
                                periodl);

  blockStartPos[hl]=(int16_t)WEBRTC_SPL_MUL_16_16(4, centerStartPos);

  


  for(q=hl-1;q>=0;q--) {
    blockStartPos[q]=blockStartPos[q+1]-period[lagBlock[q+1]];

    WebRtcIlbcfix_NearestNeighbor(lagBlock+q, plocs,
                                  (int16_t)(blockStartPos[q] + (int16_t)WEBRTC_SPL_MUL_16_16(4, ENH_BLOCKL_HALF)-period[lagBlock[q+1]]),
                                  periodl);

    if((blockStartPos[q]-(int16_t)WEBRTC_SPL_MUL_16_16(4, ENH_OVERHANG))>=0) {

      

      WebRtcIlbcfix_Refiner(blockStartPos+q,idata,idatal,
                            centerStartPos,blockStartPos[q],surround,WebRtcIlbcfix_kEnhWt[q]);

    } else {
      

    }
  }

  


  for(i=0;i<periodl;i++) {
    plocs2[i]=(plocs[i]-period[i]);
  }

  for(q=hl+1;q<=WEBRTC_SPL_MUL_16_16(2, hl);q++) {

    WebRtcIlbcfix_NearestNeighbor(lagBlock+q,plocs2,
                                  (int16_t)(blockStartPos[q-1]+
                                                  (int16_t)WEBRTC_SPL_MUL_16_16(4, ENH_BLOCKL_HALF)),periodl);

    blockStartPos[q]=blockStartPos[q-1]+period[lagBlock[q]];

    if( (blockStartPos[q]+(int16_t)WEBRTC_SPL_MUL_16_16(4, (ENH_BLOCKL+ENH_OVERHANG)))
        <
        (int16_t)WEBRTC_SPL_MUL_16_16(4, idatal)) {

      

      WebRtcIlbcfix_Refiner(blockStartPos+q, idata, idatal,
                            centerStartPos,blockStartPos[q],surround,WebRtcIlbcfix_kEnhWt[2*hl-q]);

    }
    else {
      

    }
  }
}
