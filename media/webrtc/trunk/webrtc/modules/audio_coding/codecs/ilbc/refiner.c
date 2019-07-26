

















#include "defines.h"
#include "constants.h"
#include "enh_upsample.h"
#include "my_corr.h"









void WebRtcIlbcfix_Refiner(
    int16_t *updStartPos, 
    int16_t *idata,   
    int16_t idatal,   
    int16_t centerStartPos, 
    int16_t estSegPos,  
    int16_t *surround,  

    int16_t gain    
                           ){
  int16_t estSegPosRounded,searchSegStartPos,searchSegEndPos,corrdim;
  int16_t tloc,tloc2,i,st,en,fraction;

  int32_t maxtemp, scalefact;
  int16_t *filtStatePtr, *polyPtr;
  
  int16_t filt[7];
  int32_t corrVecUps[ENH_CORRDIM*ENH_UPS0];
  int32_t corrVecTemp[ENH_CORRDIM];
  int16_t vect[ENH_VECTL];
  int16_t corrVec[ENH_CORRDIM];

  

  estSegPosRounded=WEBRTC_SPL_RSHIFT_W16((estSegPos - 2),2);

  searchSegStartPos=estSegPosRounded-ENH_SLOP;

  if (searchSegStartPos<0) {
    searchSegStartPos=0;
  }
  searchSegEndPos=estSegPosRounded+ENH_SLOP;

  if(searchSegEndPos+ENH_BLOCKL >= idatal) {
    searchSegEndPos=idatal-ENH_BLOCKL-1;
  }
  corrdim=searchSegEndPos-searchSegStartPos+1;

  


  WebRtcIlbcfix_MyCorr(corrVecTemp,idata+searchSegStartPos,
                       (int16_t)(corrdim+ENH_BLOCKL-1),idata+centerStartPos,ENH_BLOCKL);

  

  maxtemp=WebRtcSpl_MaxAbsValueW32(corrVecTemp, (int16_t)corrdim);

  scalefact=WebRtcSpl_GetSizeInBits(maxtemp)-15;

  if (scalefact>0) {
    for (i=0;i<corrdim;i++) {
      corrVec[i]=(int16_t)WEBRTC_SPL_RSHIFT_W32(corrVecTemp[i], scalefact);
    }
  } else {
    for (i=0;i<corrdim;i++) {
      corrVec[i]=(int16_t)corrVecTemp[i];
    }
  }
  
  for (i=corrdim;i<ENH_CORRDIM;i++) {
    corrVec[i]=0;
  }

  
  WebRtcIlbcfix_EnhUpsample(corrVecUps,corrVec);

  
  tloc=WebRtcSpl_MaxIndexW32(corrVecUps, (int16_t) (ENH_UPS0*corrdim));

  

  *updStartPos = (int16_t)WEBRTC_SPL_MUL_16_16(searchSegStartPos,4) + tloc + 4;

  tloc2 = WEBRTC_SPL_RSHIFT_W16((tloc+3), 2);

  st=searchSegStartPos+tloc2-ENH_FL0;

  

  if(st<0){
    WebRtcSpl_MemSetW16(vect, 0, (int16_t)(-st));
    WEBRTC_SPL_MEMCPY_W16(&vect[-st], idata, (ENH_VECTL+st));
  }
  else{
    en=st+ENH_VECTL;

    if(en>idatal){
      WEBRTC_SPL_MEMCPY_W16(vect, &idata[st],
                            (ENH_VECTL-(en-idatal)));
      WebRtcSpl_MemSetW16(&vect[ENH_VECTL-(en-idatal)], 0,
                          (int16_t)(en-idatal));
    }
    else {
      WEBRTC_SPL_MEMCPY_W16(vect, &idata[st], ENH_VECTL);
    }
  }
  
  fraction=(int16_t)WEBRTC_SPL_MUL_16_16(tloc2,ENH_UPS0)-tloc;

  

  filtStatePtr = filt + 6;
  polyPtr = (int16_t*)WebRtcIlbcfix_kEnhPolyPhaser[fraction];
  for (i=0;i<7;i++) {
    *filtStatePtr-- = *polyPtr++;
  }

  WebRtcSpl_FilterMAFastQ12(
      &vect[6], vect, filt,
      ENH_FLO_MULT2_PLUS1, ENH_BLOCKL);

  
  WebRtcSpl_AddAffineVectorToVector(
      surround, vect, gain,
      (int32_t)32768, 16, ENH_BLOCKL);

  return;
}
