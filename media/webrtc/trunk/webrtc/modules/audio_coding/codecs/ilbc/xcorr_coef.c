

















#include "defines.h"






int WebRtcIlbcfix_XcorrCoef(
    WebRtc_Word16 *target,  
    WebRtc_Word16 *regressor, 
    WebRtc_Word16 subl,  
    WebRtc_Word16 searchLen, 
    WebRtc_Word16 offset,  
    WebRtc_Word16 step   
                            ){
  int k;
  WebRtc_Word16 maxlag;
  WebRtc_Word16 pos;
  WebRtc_Word16 max;
  WebRtc_Word16 crossCorrScale, Energyscale;
  WebRtc_Word16 crossCorrSqMod, crossCorrSqMod_Max;
  WebRtc_Word32 crossCorr, Energy;
  WebRtc_Word16 crossCorrmod, EnergyMod, EnergyMod_Max;
  WebRtc_Word16 *tp, *rp;
  WebRtc_Word16 *rp_beg, *rp_end;
  WebRtc_Word16 totscale, totscale_max;
  WebRtc_Word16 scalediff;
  WebRtc_Word32 newCrit, maxCrit;
  int shifts;

  
  crossCorrSqMod_Max=0;
  EnergyMod_Max=WEBRTC_SPL_WORD16_MAX;
  totscale_max=-500;
  maxlag=0;
  pos=0;

  
  if (step==1) {
    max=WebRtcSpl_MaxAbsValueW16(regressor, (WebRtc_Word16)(subl+searchLen-1));
    rp_beg = regressor;
    rp_end = &regressor[subl];
  } else { 
    max=WebRtcSpl_MaxAbsValueW16(&regressor[-searchLen], (WebRtc_Word16)(subl+searchLen-1));
    rp_beg = &regressor[-1];
    rp_end = &regressor[subl-1];
  }

  



  if (max>5000) {
    shifts=2;
  } else {
    shifts=0;
  }

  
  Energy=WebRtcSpl_DotProductWithScale(regressor, regressor, subl, shifts);

  for (k=0;k<searchLen;k++) {
    tp = target;
    rp = &regressor[pos];

    crossCorr=WebRtcSpl_DotProductWithScale(tp, rp, subl, shifts);

    if ((Energy>0)&&(crossCorr>0)) {

      
      crossCorrScale=(WebRtc_Word16)WebRtcSpl_NormW32(crossCorr)-16;
      crossCorrmod=(WebRtc_Word16)WEBRTC_SPL_SHIFT_W32(crossCorr, crossCorrScale);
      Energyscale=(WebRtc_Word16)WebRtcSpl_NormW32(Energy)-16;
      EnergyMod=(WebRtc_Word16)WEBRTC_SPL_SHIFT_W32(Energy, Energyscale);

      
      crossCorrSqMod=(WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT(crossCorrmod, crossCorrmod, 16);

      


      totscale=Energyscale-(crossCorrScale<<1);

      


      scalediff=totscale-totscale_max;
      scalediff=WEBRTC_SPL_MIN(scalediff,31);
      scalediff=WEBRTC_SPL_MAX(scalediff,-31);

      



      if (scalediff<0) {
        newCrit = ((WebRtc_Word32)crossCorrSqMod*EnergyMod_Max)>>(-scalediff);
        maxCrit = ((WebRtc_Word32)crossCorrSqMod_Max*EnergyMod);
      } else {
        newCrit = ((WebRtc_Word32)crossCorrSqMod*EnergyMod_Max);
        maxCrit = ((WebRtc_Word32)crossCorrSqMod_Max*EnergyMod)>>scalediff;
      }

      


      if (newCrit > maxCrit) {
        crossCorrSqMod_Max = crossCorrSqMod;
        EnergyMod_Max = EnergyMod;
        totscale_max = totscale;
        maxlag = k;
      }
    }
    pos+=step;

    
    Energy += step*(WEBRTC_SPL_RSHIFT_W32(
        ((WebRtc_Word32)(*rp_end)*(*rp_end)) - ((WebRtc_Word32)(*rp_beg)*(*rp_beg)),
        shifts));
    rp_beg+=step;
    rp_end+=step;
  }

  return(maxlag+offset);
}
