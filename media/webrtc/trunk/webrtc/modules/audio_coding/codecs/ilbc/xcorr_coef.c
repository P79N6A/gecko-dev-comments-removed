

















#include "defines.h"






int WebRtcIlbcfix_XcorrCoef(
    int16_t *target,  
    int16_t *regressor, 
    int16_t subl,  
    int16_t searchLen, 
    int16_t offset,  
    int16_t step   
                            ){
  int k;
  int16_t maxlag;
  int16_t pos;
  int16_t max;
  int16_t crossCorrScale, Energyscale;
  int16_t crossCorrSqMod, crossCorrSqMod_Max;
  int32_t crossCorr, Energy;
  int16_t crossCorrmod, EnergyMod, EnergyMod_Max;
  int16_t *tp, *rp;
  int16_t *rp_beg, *rp_end;
  int16_t totscale, totscale_max;
  int16_t scalediff;
  int32_t newCrit, maxCrit;
  int shifts;

  
  crossCorrSqMod_Max=0;
  EnergyMod_Max=WEBRTC_SPL_WORD16_MAX;
  totscale_max=-500;
  maxlag=0;
  pos=0;

  
  if (step==1) {
    max=WebRtcSpl_MaxAbsValueW16(regressor, (int16_t)(subl+searchLen-1));
    rp_beg = regressor;
    rp_end = &regressor[subl];
  } else { 
    max=WebRtcSpl_MaxAbsValueW16(&regressor[-searchLen], (int16_t)(subl+searchLen-1));
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

      
      crossCorrScale=(int16_t)WebRtcSpl_NormW32(crossCorr)-16;
      crossCorrmod=(int16_t)WEBRTC_SPL_SHIFT_W32(crossCorr, crossCorrScale);
      Energyscale=(int16_t)WebRtcSpl_NormW32(Energy)-16;
      EnergyMod=(int16_t)WEBRTC_SPL_SHIFT_W32(Energy, Energyscale);

      
      crossCorrSqMod=(int16_t)WEBRTC_SPL_MUL_16_16_RSFT(crossCorrmod, crossCorrmod, 16);

      


      totscale=Energyscale-(crossCorrScale<<1);

      


      scalediff=totscale-totscale_max;
      scalediff=WEBRTC_SPL_MIN(scalediff,31);
      scalediff=WEBRTC_SPL_MAX(scalediff,-31);

      



      if (scalediff<0) {
        newCrit = ((int32_t)crossCorrSqMod*EnergyMod_Max)>>(-scalediff);
        maxCrit = ((int32_t)crossCorrSqMod_Max*EnergyMod);
      } else {
        newCrit = ((int32_t)crossCorrSqMod*EnergyMod_Max);
        maxCrit = ((int32_t)crossCorrSqMod_Max*EnergyMod)>>scalediff;
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
        ((int32_t)(*rp_end)*(*rp_end)) - ((int32_t)(*rp_beg)*(*rp_beg)),
        shifts));
    rp_beg+=step;
    rp_end+=step;
  }

  return(maxlag+offset);
}
