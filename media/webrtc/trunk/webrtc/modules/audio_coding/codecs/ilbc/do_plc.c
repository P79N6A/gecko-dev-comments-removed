

















#include "defines.h"
#include "constants.h"
#include "comp_corr.h"
#include "bw_expand.h"






void WebRtcIlbcfix_DoThePlc(
    int16_t *PLCresidual,  
    int16_t *PLClpc,    
    int16_t PLI,     

    int16_t *decresidual,  
    int16_t *lpc,    
    int16_t inlag,    
    iLBC_Dec_Inst_t *iLBCdec_inst
    
                            ){
  int16_t i, pick;
  int32_t cross, ener, cross_comp, ener_comp = 0;
  int32_t measure, maxMeasure, energy;
  int16_t max, crossSquareMax, crossSquare;
  int16_t j, lag, tmp1, tmp2, randlag;
  int16_t shift1, shift2, shift3, shiftMax;
  int16_t scale3;
  int16_t corrLen;
  int32_t tmpW32, tmp2W32;
  int16_t use_gain;
  int16_t tot_gain;
  int16_t max_perSquare;
  int16_t scale1, scale2;
  int16_t totscale;
  int32_t nom;
  int16_t denom;
  int16_t pitchfact;
  int16_t use_lag;
  int ind;
  int16_t randvec[BLOCKL_MAX];

  
  if (PLI == 1) {

    (*iLBCdec_inst).consPLICount += 1;

    


    if (iLBCdec_inst->prevPLI != 1) {

      

      max = WebRtcSpl_MaxAbsValueW16((*iLBCdec_inst).prevResidual, (int16_t)iLBCdec_inst->blockl);
      scale3 = (WebRtcSpl_GetSizeInBits(max)<<1) - 25;
      if (scale3 < 0) {
        scale3 = 0;
      }

      

      iLBCdec_inst->prevScale = scale3;

      

      lag = inlag - 3;

      
      corrLen = WEBRTC_SPL_MIN(60, iLBCdec_inst->blockl-(inlag+3));

      WebRtcIlbcfix_CompCorr( &cross, &ener,
                              iLBCdec_inst->prevResidual, lag, iLBCdec_inst->blockl, corrLen, scale3);

      
      shiftMax = WebRtcSpl_GetSizeInBits(WEBRTC_SPL_ABS_W32(cross))-15;
      crossSquareMax = (int16_t)WEBRTC_SPL_MUL_16_16_RSFT(WEBRTC_SPL_SHIFT_W32(cross, -shiftMax),
                                                                WEBRTC_SPL_SHIFT_W32(cross, -shiftMax), 15);

      for (j=inlag-2;j<=inlag+3;j++) {
        WebRtcIlbcfix_CompCorr( &cross_comp, &ener_comp,
                                iLBCdec_inst->prevResidual, j, iLBCdec_inst->blockl, corrLen, scale3);

        


        shift1 = WebRtcSpl_GetSizeInBits(WEBRTC_SPL_ABS_W32(cross_comp))-15;
        crossSquare = (int16_t)WEBRTC_SPL_MUL_16_16_RSFT(WEBRTC_SPL_SHIFT_W32(cross_comp, -shift1),
                                                               WEBRTC_SPL_SHIFT_W32(cross_comp, -shift1), 15);

        shift2 = WebRtcSpl_GetSizeInBits(ener)-15;
        measure = WEBRTC_SPL_MUL_16_16(WEBRTC_SPL_SHIFT_W32(ener, -shift2),
                                       crossSquare);

        shift3 = WebRtcSpl_GetSizeInBits(ener_comp)-15;
        maxMeasure = WEBRTC_SPL_MUL_16_16(WEBRTC_SPL_SHIFT_W32(ener_comp, -shift3),
                                          crossSquareMax);

        

        if(((shiftMax<<1)+shift3) > ((shift1<<1)+shift2)) {
          tmp1 = WEBRTC_SPL_MIN(31, (shiftMax<<1)+shift3-(shift1<<1)-shift2);
          tmp2 = 0;
        } else {
          tmp1 = 0;
          tmp2 = WEBRTC_SPL_MIN(31, (shift1<<1)+shift2-(shiftMax<<1)-shift3);
        }

        if ((measure>>tmp1) > (maxMeasure>>tmp2)) {
          
          lag = j;
          crossSquareMax = crossSquare;
          cross = cross_comp;
          shiftMax = shift1;
          ener = ener_comp;
        }
      }

      







      tmp2W32=WebRtcSpl_DotProductWithScale(&iLBCdec_inst->prevResidual[iLBCdec_inst->blockl-corrLen],
                                            &iLBCdec_inst->prevResidual[iLBCdec_inst->blockl-corrLen],
                                            corrLen, scale3);

      if ((tmp2W32>0)&&(ener_comp>0)) {
        


        scale1=(int16_t)WebRtcSpl_NormW32(tmp2W32)-16;
        tmp1=(int16_t)WEBRTC_SPL_SHIFT_W32(tmp2W32, scale1);

        scale2=(int16_t)WebRtcSpl_NormW32(ener)-16;
        tmp2=(int16_t)WEBRTC_SPL_SHIFT_W32(ener, scale2);
        denom=(int16_t)WEBRTC_SPL_MUL_16_16_RSFT(tmp1, tmp2, 16); 

        


        totscale = scale1+scale2-1;
        tmp1 = (int16_t)WEBRTC_SPL_SHIFT_W32(cross, (totscale>>1));
        tmp2 = (int16_t)WEBRTC_SPL_SHIFT_W32(cross, totscale-(totscale>>1));

        nom = WEBRTC_SPL_MUL_16_16(tmp1, tmp2);
        max_perSquare = (int16_t)WebRtcSpl_DivW32W16(nom, denom);

      } else {
        max_perSquare = 0;
      }
    }

    

    else {
      lag = iLBCdec_inst->prevLag;
      max_perSquare = iLBCdec_inst->perSquare;
    }

    


    use_gain = 32767;   

    if (iLBCdec_inst->consPLICount*iLBCdec_inst->blockl>320) {
      use_gain = 29491;  
    } else if (iLBCdec_inst->consPLICount*iLBCdec_inst->blockl>640) {
      use_gain = 22938;  
    } else if (iLBCdec_inst->consPLICount*iLBCdec_inst->blockl>960) {
      use_gain = 16384;  
    } else if (iLBCdec_inst->consPLICount*iLBCdec_inst->blockl>1280) {
      use_gain = 0;   
    }

    





    if (max_perSquare>7868) { 
      pitchfact = 32767;
    } else if (max_perSquare>839) { 
      
      ind = 5;
      while ((max_perSquare<WebRtcIlbcfix_kPlcPerSqr[ind])&&(ind>0)) {
        ind--;
      }
      
      tmpW32 = (int32_t)WebRtcIlbcfix_kPlcPitchFact[ind] +
          WEBRTC_SPL_MUL_16_16_RSFT(WebRtcIlbcfix_kPlcPfSlope[ind], (max_perSquare-WebRtcIlbcfix_kPlcPerSqr[ind]), 11);

      pitchfact = (int16_t)WEBRTC_SPL_MIN(tmpW32, 32767); 

    } else { 
      pitchfact = 0;
    }

    
    use_lag = lag;
    if (lag<80) {
      use_lag = 2*lag;
    }

    
    energy = 0;

    for (i=0; i<iLBCdec_inst->blockl; i++) {

      
      iLBCdec_inst->seed = (int16_t)(WEBRTC_SPL_MUL_16_16(iLBCdec_inst->seed, 31821)+(int32_t)13849);
      randlag = 53 + (int16_t)(iLBCdec_inst->seed & 63);

      pick = i - randlag;

      if (pick < 0) {
        randvec[i] = iLBCdec_inst->prevResidual[iLBCdec_inst->blockl+pick];
      } else {
        randvec[i] = iLBCdec_inst->prevResidual[pick];
      }

      
      pick = i - use_lag;

      if (pick < 0) {
        PLCresidual[i] = iLBCdec_inst->prevResidual[iLBCdec_inst->blockl+pick];
      } else {
        PLCresidual[i] = PLCresidual[pick];
      }

      
      if (i<80) {
        tot_gain=use_gain;
      } else if (i<160) {
        tot_gain=(int16_t)WEBRTC_SPL_MUL_16_16_RSFT(31130, use_gain, 15); 
      } else {
        tot_gain=(int16_t)WEBRTC_SPL_MUL_16_16_RSFT(29491, use_gain, 15); 
      }


      

      PLCresidual[i] = (int16_t)WEBRTC_SPL_MUL_16_16_RSFT(tot_gain,
                                                                (int16_t)WEBRTC_SPL_RSHIFT_W32( (WEBRTC_SPL_MUL_16_16(pitchfact, PLCresidual[i]) +
                                                                                                       WEBRTC_SPL_MUL_16_16((32767-pitchfact), randvec[i]) + 16384),
                                                                                                      15),
                                                                15);

      

      energy += WEBRTC_SPL_MUL_16_16_RSFT(PLCresidual[i],
                                          PLCresidual[i], (iLBCdec_inst->prevScale+1));

    }

    
    if (energy < (WEBRTC_SPL_SHIFT_W32(((int32_t)iLBCdec_inst->blockl*900),-(iLBCdec_inst->prevScale+1)))) {
      energy = 0;
      for (i=0; i<iLBCdec_inst->blockl; i++) {
        PLCresidual[i] = randvec[i];
      }
    }

    
    WEBRTC_SPL_MEMCPY_W16(PLClpc, (*iLBCdec_inst).prevLpc, LPC_FILTERORDER+1);

    
    iLBCdec_inst->prevLag = lag;
    iLBCdec_inst->perSquare = max_perSquare;
  }

  

  else {
    WEBRTC_SPL_MEMCPY_W16(PLCresidual, decresidual, iLBCdec_inst->blockl);
    WEBRTC_SPL_MEMCPY_W16(PLClpc, lpc, (LPC_FILTERORDER+1));
    iLBCdec_inst->consPLICount = 0;
  }

  
  iLBCdec_inst->prevPLI = PLI;
  WEBRTC_SPL_MEMCPY_W16(iLBCdec_inst->prevLpc, PLClpc, (LPC_FILTERORDER+1));
  WEBRTC_SPL_MEMCPY_W16(iLBCdec_inst->prevResidual, PLCresidual, iLBCdec_inst->blockl);

  return;
}
