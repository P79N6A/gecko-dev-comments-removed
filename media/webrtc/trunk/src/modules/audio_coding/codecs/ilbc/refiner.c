

















#include "defines.h"
#include "constants.h"
#include "enh_upsample.h"
#include "my_corr.h"









void WebRtcIlbcfix_Refiner(
    WebRtc_Word16 *updStartPos, 
    WebRtc_Word16 *idata,   
    WebRtc_Word16 idatal,   
    WebRtc_Word16 centerStartPos, 
    WebRtc_Word16 estSegPos,  
    WebRtc_Word16 *surround,  

    WebRtc_Word16 gain    
                           ){
  WebRtc_Word16 estSegPosRounded,searchSegStartPos,searchSegEndPos,corrdim;
  WebRtc_Word16 tloc,tloc2,i,st,en,fraction;

  WebRtc_Word32 maxtemp, scalefact;
  WebRtc_Word16 *filtStatePtr, *polyPtr;
  
  WebRtc_Word16 filt[7];
  WebRtc_Word32 corrVecUps[ENH_CORRDIM*ENH_UPS0];
  WebRtc_Word32 corrVecTemp[ENH_CORRDIM];
  WebRtc_Word16 vect[ENH_VECTL];
  WebRtc_Word16 corrVec[ENH_CORRDIM];

  

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
                       (WebRtc_Word16)(corrdim+ENH_BLOCKL-1),idata+centerStartPos,ENH_BLOCKL);

  

  maxtemp=WebRtcSpl_MaxAbsValueW32(corrVecTemp, (WebRtc_Word16)corrdim);

  scalefact=WebRtcSpl_GetSizeInBits(maxtemp)-15;

  if (scalefact>0) {
    for (i=0;i<corrdim;i++) {
      corrVec[i]=(WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(corrVecTemp[i], scalefact);
    }
  } else {
    for (i=0;i<corrdim;i++) {
      corrVec[i]=(WebRtc_Word16)corrVecTemp[i];
    }
  }
  
  for (i=corrdim;i<ENH_CORRDIM;i++) {
    corrVec[i]=0;
  }

  
  WebRtcIlbcfix_EnhUpsample(corrVecUps,corrVec);

  
  tloc=WebRtcSpl_MaxIndexW32(corrVecUps, (WebRtc_Word16) (ENH_UPS0*corrdim));

  

  *updStartPos = (WebRtc_Word16)WEBRTC_SPL_MUL_16_16(searchSegStartPos,4) + tloc + 4;

  tloc2 = WEBRTC_SPL_RSHIFT_W16((tloc+3), 2);

  st=searchSegStartPos+tloc2-ENH_FL0;

  

  if(st<0){
    WebRtcSpl_MemSetW16(vect, 0, (WebRtc_Word16)(-st));
    WEBRTC_SPL_MEMCPY_W16(&vect[-st], idata, (ENH_VECTL+st));
  }
  else{
    en=st+ENH_VECTL;

    if(en>idatal){
      WEBRTC_SPL_MEMCPY_W16(vect, &idata[st],
                            (ENH_VECTL-(en-idatal)));
      WebRtcSpl_MemSetW16(&vect[ENH_VECTL-(en-idatal)], 0,
                          (WebRtc_Word16)(en-idatal));
    }
    else {
      WEBRTC_SPL_MEMCPY_W16(vect, &idata[st], ENH_VECTL);
    }
  }
  
  fraction=(WebRtc_Word16)WEBRTC_SPL_MUL_16_16(tloc2,ENH_UPS0)-tloc;

  

  filtStatePtr = filt + 6;
  polyPtr = (WebRtc_Word16*)WebRtcIlbcfix_kEnhPolyPhaser[fraction];
  for (i=0;i<7;i++) {
    *filtStatePtr-- = *polyPtr++;
  }

  WebRtcSpl_FilterMAFastQ12(
      &vect[6], vect, filt,
      ENH_FLO_MULT2_PLUS1, ENH_BLOCKL);

  
  WebRtcSpl_AddAffineVectorToVector(
      surround, vect, gain,
      (WebRtc_Word32)32768, 16, ENH_BLOCKL);

  return;
}
