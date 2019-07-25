

















#include "defines.h"
#include "constants.h"
#include "refiner.h"
#include "nearest_neighbor.h"





void WebRtcIlbcfix_GetSyncSeq(
    WebRtc_Word16 *idata,   
    WebRtc_Word16 idatal,   
    WebRtc_Word16 centerStartPos, 
    WebRtc_Word16 *period,   
    WebRtc_Word16 *plocs,   
    WebRtc_Word16 periodl,   
    WebRtc_Word16 hl,    
    WebRtc_Word16 *surround  

                              ){
  WebRtc_Word16 i,centerEndPos,q;
  
  WebRtc_Word16 lagBlock[2*ENH_HL+1];
  WebRtc_Word16 blockStartPos[2*ENH_HL+1]; 
  WebRtc_Word16 plocs2[ENH_PLOCSL];

  centerEndPos=centerStartPos+ENH_BLOCKL-1;

  

  WebRtcIlbcfix_NearestNeighbor(lagBlock+hl,plocs,
                                (WebRtc_Word16)WEBRTC_SPL_MUL_16_16(2, (centerStartPos+centerEndPos)),
                                periodl);

  blockStartPos[hl]=(WebRtc_Word16)WEBRTC_SPL_MUL_16_16(4, centerStartPos);

  


  for(q=hl-1;q>=0;q--) {
    blockStartPos[q]=blockStartPos[q+1]-period[lagBlock[q+1]];

    WebRtcIlbcfix_NearestNeighbor(lagBlock+q, plocs,
                                  (WebRtc_Word16)(blockStartPos[q] + (WebRtc_Word16)WEBRTC_SPL_MUL_16_16(4, ENH_BLOCKL_HALF)-period[lagBlock[q+1]]),
                                  periodl);

    if((blockStartPos[q]-(WebRtc_Word16)WEBRTC_SPL_MUL_16_16(4, ENH_OVERHANG))>=0) {

      

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
                                  (WebRtc_Word16)(blockStartPos[q-1]+
                                                  (WebRtc_Word16)WEBRTC_SPL_MUL_16_16(4, ENH_BLOCKL_HALF)),periodl);

    blockStartPos[q]=blockStartPos[q-1]+period[lagBlock[q]];

    if( (blockStartPos[q]+(WebRtc_Word16)WEBRTC_SPL_MUL_16_16(4, (ENH_BLOCKL+ENH_OVERHANG)))
        <
        (WebRtc_Word16)WEBRTC_SPL_MUL_16_16(4, idatal)) {

      

      WebRtcIlbcfix_Refiner(blockStartPos+q, idata, idatal,
                            centerStartPos,blockStartPos[q],surround,WebRtcIlbcfix_kEnhWt[2*hl-q]);

    }
    else {
      

    }
  }
}
