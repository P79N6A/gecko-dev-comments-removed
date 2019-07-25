

















#include "defines.h"
#include "constants.h"
#include "comp_corr.h"
#include "bw_expand.h"






void WebRtcIlbcfix_DoThePlc(
    WebRtc_Word16 *PLCresidual,  
    WebRtc_Word16 *PLClpc,    
    WebRtc_Word16 PLI,     

    WebRtc_Word16 *decresidual,  
    WebRtc_Word16 *lpc,    
    WebRtc_Word16 inlag,    
    iLBC_Dec_Inst_t *iLBCdec_inst
    
                            ){
  WebRtc_Word16 i, pick;
  WebRtc_Word32 cross, ener, cross_comp, ener_comp = 0;
  WebRtc_Word32 measure, maxMeasure, energy;
  WebRtc_Word16 max, crossSquareMax, crossSquare;
  WebRtc_Word16 j, lag, tmp1, tmp2, randlag;
  WebRtc_Word16 shift1, shift2, shift3, shiftMax;
  WebRtc_Word16 scale3;
  WebRtc_Word16 corrLen;
  WebRtc_Word32 tmpW32, tmp2W32;
  WebRtc_Word16 use_gain;
  WebRtc_Word16 tot_gain;
  WebRtc_Word16 max_perSquare;
  WebRtc_Word16 scale1, scale2;
  WebRtc_Word16 totscale;
  WebRtc_Word32 nom;
  WebRtc_Word16 denom;
  WebRtc_Word16 pitchfact;
  WebRtc_Word16 use_lag;
  int ind;
  WebRtc_Word16 randvec[BLOCKL_MAX];

  
  if (PLI == 1) {

    (*iLBCdec_inst).consPLICount += 1;

    


    if (iLBCdec_inst->prevPLI != 1) {

      

      max = WebRtcSpl_MaxAbsValueW16((*iLBCdec_inst).prevResidual, (WebRtc_Word16)iLBCdec_inst->blockl);
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
      crossSquareMax = (WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT(WEBRTC_SPL_SHIFT_W32(cross, -shiftMax),
                                                                WEBRTC_SPL_SHIFT_W32(cross, -shiftMax), 15);

      for (j=inlag-2;j<=inlag+3;j++) {
        WebRtcIlbcfix_CompCorr( &cross_comp, &ener_comp,
                                iLBCdec_inst->prevResidual, j, iLBCdec_inst->blockl, corrLen, scale3);

        


        shift1 = WebRtcSpl_GetSizeInBits(WEBRTC_SPL_ABS_W32(cross_comp))-15;
        crossSquare = (WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT(WEBRTC_SPL_SHIFT_W32(cross_comp, -shift1),
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
        


        scale1=(WebRtc_Word16)WebRtcSpl_NormW32(tmp2W32)-16;
        tmp1=(WebRtc_Word16)WEBRTC_SPL_SHIFT_W32(tmp2W32, scale1);

        scale2=(WebRtc_Word16)WebRtcSpl_NormW32(ener)-16;
        tmp2=(WebRtc_Word16)WEBRTC_SPL_SHIFT_W32(ener, scale2);
        denom=(WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT(tmp1, tmp2, 16); 

        


        totscale = scale1+scale2-1;
        tmp1 = (WebRtc_Word16)WEBRTC_SPL_SHIFT_W32(cross, (totscale>>1));
        tmp2 = (WebRtc_Word16)WEBRTC_SPL_SHIFT_W32(cross, totscale-(totscale>>1));

        nom = WEBRTC_SPL_MUL_16_16(tmp1, tmp2);
        max_perSquare = (WebRtc_Word16)WebRtcSpl_DivW32W16(nom, denom);

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
      
      tmpW32 = (WebRtc_Word32)WebRtcIlbcfix_kPlcPitchFact[ind] +
          WEBRTC_SPL_MUL_16_16_RSFT(WebRtcIlbcfix_kPlcPfSlope[ind], (max_perSquare-WebRtcIlbcfix_kPlcPerSqr[ind]), 11);

      pitchfact = (WebRtc_Word16)WEBRTC_SPL_MIN(tmpW32, 32767); 

    } else { 
      pitchfact = 0;
    }

    
    use_lag = lag;
    if (lag<80) {
      use_lag = 2*lag;
    }

    
    energy = 0;

    for (i=0; i<iLBCdec_inst->blockl; i++) {

      
      iLBCdec_inst->seed = (WebRtc_Word16)(WEBRTC_SPL_MUL_16_16(iLBCdec_inst->seed, 31821)+(WebRtc_Word32)13849);
      randlag = 53 + (WebRtc_Word16)(iLBCdec_inst->seed & 63);

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
        tot_gain=(WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT(31130, use_gain, 15); 
      } else {
        tot_gain=(WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT(29491, use_gain, 15); 
      }


      

      PLCresidual[i] = (WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT(tot_gain,
                                                                (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32( (WEBRTC_SPL_MUL_16_16(pitchfact, PLCresidual[i]) +
                                                                                                       WEBRTC_SPL_MUL_16_16((32767-pitchfact), randvec[i]) + 16384),
                                                                                                      15),
                                                                15);

      

      energy += WEBRTC_SPL_MUL_16_16_RSFT(PLCresidual[i],
                                          PLCresidual[i], (iLBCdec_inst->prevScale+1));

    }

    
    if (energy < (WEBRTC_SPL_SHIFT_W32(((WebRtc_Word32)iLBCdec_inst->blockl*900),-(iLBCdec_inst->prevScale+1)))) {
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
