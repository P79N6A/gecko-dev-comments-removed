









#include "webrtc/modules/audio_processing/ns/include/noise_suppression_x.h"
#include "webrtc/modules/audio_processing/ns/nsx_core.h"

static const int16_t kIndicatorTable[17] = {
  0, 2017, 3809, 5227, 6258, 6963, 7424, 7718,
  7901, 8014, 8084, 8126, 8152, 8168, 8177, 8183, 8187
};





void WebRtcNsx_SpeechNoiseProb(NsxInst_t* inst,
                               uint16_t* nonSpeechProbFinal,
                               uint32_t* priorLocSnr,
                               uint32_t* postLocSnr) {

  uint32_t zeros, num, den, tmpU32no1, tmpU32no2, tmpU32no3;
  int32_t invLrtFX, indPriorFX, tmp32, tmp32no1, tmp32no2, besselTmpFX32;
  int32_t frac32, logTmp;
  int32_t logLrtTimeAvgKsumFX;
  int16_t indPriorFX16;
  int16_t tmp16, tmp16no1, tmp16no2, tmpIndFX, tableIndex, frac, intPart;
  int i, normTmp, normTmp2, nShifts;

  
  
  logLrtTimeAvgKsumFX = 0;
  for (i = 0; i < inst->magnLen; i++) {
    besselTmpFX32 = (int32_t)postLocSnr[i]; 
    normTmp = WebRtcSpl_NormU32(postLocSnr[i]);
    num = WEBRTC_SPL_LSHIFT_U32(postLocSnr[i], normTmp); 
    if (normTmp > 10) {
      den = WEBRTC_SPL_LSHIFT_U32(priorLocSnr[i], normTmp - 11); 
    } else {
      den = WEBRTC_SPL_RSHIFT_U32(priorLocSnr[i], 11 - normTmp); 
    }
    if (den > 0) {
      besselTmpFX32 -= WEBRTC_SPL_UDIV(num, den); 
    } else {
      besselTmpFX32 -= num; 
    }

    
    
    
    zeros = WebRtcSpl_NormU32(priorLocSnr[i]);
    frac32 = (int32_t)(((priorLocSnr[i] << zeros) & 0x7FFFFFFF) >> 19);
    tmp32 = WEBRTC_SPL_MUL(frac32, frac32);
    tmp32 = WEBRTC_SPL_RSHIFT_W32(WEBRTC_SPL_MUL(tmp32, -43), 19);
    tmp32 += WEBRTC_SPL_MUL_16_16_RSFT((int16_t)frac32, 5412, 12);
    frac32 = tmp32 + 37;
    
    tmp32 = (int32_t)(((31 - zeros) << 12) + frac32) - (11 << 12); 
    logTmp = WEBRTC_SPL_RSHIFT_W32(WEBRTC_SPL_MUL_32_16(tmp32, 178), 8);
                                                  
    tmp32no1 = WEBRTC_SPL_RSHIFT_W32(logTmp + inst->logLrtTimeAvgW32[i], 1);
                                                  
    inst->logLrtTimeAvgW32[i] += (besselTmpFX32 - tmp32no1); 

    logLrtTimeAvgKsumFX += inst->logLrtTimeAvgW32[i]; 
  }
  inst->featureLogLrt = WEBRTC_SPL_RSHIFT_W32(logLrtTimeAvgKsumFX * 5,
                                              inst->stages + 10);
                                                  
  

  
  
  

  
  
  
  
  tmpIndFX = 16384; 
  tmp32no1 = logLrtTimeAvgKsumFX - inst->thresholdLogLrt; 
  nShifts = 7 - inst->stages; 
  
  if (tmp32no1 < 0) {
    tmpIndFX = 0;
    tmp32no1 = -tmp32no1;
    
    nShifts++;
  }
  tmp32no1 = WEBRTC_SPL_SHIFT_W32(tmp32no1, nShifts); 
  
  tableIndex = (int16_t)WEBRTC_SPL_RSHIFT_W32(tmp32no1, 14);
  if ((tableIndex < 16) && (tableIndex >= 0)) {
    tmp16no2 = kIndicatorTable[tableIndex];
    tmp16no1 = kIndicatorTable[tableIndex + 1] - kIndicatorTable[tableIndex];
    frac = (int16_t)(tmp32no1 & 0x00003fff); 
    tmp16no2 += (int16_t)WEBRTC_SPL_MUL_16_16_RSFT(tmp16no1, frac, 14);
    if (tmpIndFX == 0) {
      tmpIndFX = 8192 - tmp16no2; 
    } else {
      tmpIndFX = 8192 + tmp16no2; 
    }
  }
  indPriorFX = WEBRTC_SPL_MUL_16_16(inst->weightLogLrt, tmpIndFX); 

  
  if (inst->weightSpecFlat) {
    tmpU32no1 = WEBRTC_SPL_UMUL(inst->featureSpecFlat, 400); 
    tmpIndFX = 16384; 
    
    tmpU32no2 = inst->thresholdSpecFlat - tmpU32no1; 
    nShifts = 4;
    if (inst->thresholdSpecFlat < tmpU32no1) {
      tmpIndFX = 0;
      tmpU32no2 = tmpU32no1 - inst->thresholdSpecFlat;
      
      nShifts++;
    }
    tmp32no1 = (int32_t)WebRtcSpl_DivU32U16(WEBRTC_SPL_LSHIFT_U32(tmpU32no2,
                                                                  nShifts), 25);
                                                     
    tmpU32no1 = WebRtcSpl_DivU32U16(WEBRTC_SPL_LSHIFT_U32(tmpU32no2, nShifts),
                                    25); 
    
    
    
    
    tableIndex = (int16_t)WEBRTC_SPL_RSHIFT_U32(tmpU32no1, 14);
    if (tableIndex < 16) {
      tmp16no2 = kIndicatorTable[tableIndex];
      tmp16no1 = kIndicatorTable[tableIndex + 1] - kIndicatorTable[tableIndex];
      frac = (int16_t)(tmpU32no1 & 0x00003fff); 
      tmp16no2 += (int16_t)WEBRTC_SPL_MUL_16_16_RSFT(tmp16no1, frac, 14);
      if (tmpIndFX) {
        tmpIndFX = 8192 + tmp16no2; 
      } else {
        tmpIndFX = 8192 - tmp16no2; 
      }
    }
    indPriorFX += WEBRTC_SPL_MUL_16_16(inst->weightSpecFlat, tmpIndFX); 
  }

  
  if (inst->weightSpecDiff) {
    tmpU32no1 = 0;
    if (inst->featureSpecDiff) {
      normTmp = WEBRTC_SPL_MIN(20 - inst->stages,
                               WebRtcSpl_NormU32(inst->featureSpecDiff));
      tmpU32no1 = WEBRTC_SPL_LSHIFT_U32(inst->featureSpecDiff, normTmp);
                                                         
      tmpU32no2 = WEBRTC_SPL_RSHIFT_U32(inst->timeAvgMagnEnergy,
                                        20 - inst->stages - normTmp);
      if (tmpU32no2 > 0) {
        
        tmpU32no1 = WEBRTC_SPL_UDIV(tmpU32no1, tmpU32no2);
      } else {
        tmpU32no1 = (uint32_t)(0x7fffffff);
      }
    }
    tmpU32no3 = WEBRTC_SPL_UDIV(WEBRTC_SPL_LSHIFT_U32(inst->thresholdSpecDiff,
                                                      17),
                                25);
    tmpU32no2 = tmpU32no1 - tmpU32no3;
    nShifts = 1;
    tmpIndFX = 16384; 
    
    if (tmpU32no2 & 0x80000000) {
      tmpIndFX = 0;
      tmpU32no2 = tmpU32no3 - tmpU32no1;
      
      nShifts--;
    }
    tmpU32no1 = WEBRTC_SPL_RSHIFT_U32(tmpU32no2, nShifts);
    
    


    tableIndex = (int16_t)WEBRTC_SPL_RSHIFT_U32(tmpU32no1, 14);
    if (tableIndex < 16) {
      tmp16no2 = kIndicatorTable[tableIndex];
      tmp16no1 = kIndicatorTable[tableIndex + 1] - kIndicatorTable[tableIndex];
      frac = (int16_t)(tmpU32no1 & 0x00003fff); 
      tmp16no2 += (int16_t)WEBRTC_SPL_MUL_16_16_RSFT_WITH_ROUND(
                    tmp16no1, frac, 14);
      if (tmpIndFX) {
        tmpIndFX = 8192 + tmp16no2;
      } else {
        tmpIndFX = 8192 - tmp16no2;
      }
    }
    indPriorFX += WEBRTC_SPL_MUL_16_16(inst->weightSpecDiff, tmpIndFX); 
  }

  
  
  
  
  indPriorFX16 = WebRtcSpl_DivW32W16ResW16(98307 - indPriorFX, 6); 
  

  
  
  
  
  tmp16 = indPriorFX16 - inst->priorNonSpeechProb; 
  inst->priorNonSpeechProb += (int16_t)WEBRTC_SPL_MUL_16_16_RSFT(
                                PRIOR_UPDATE_Q14, tmp16, 14); 

  

  memset(nonSpeechProbFinal, 0, sizeof(uint16_t) * inst->magnLen);

  if (inst->priorNonSpeechProb > 0) {
    for (i = 0; i < inst->magnLen; i++) {
      
      
      
      
      
      
      
      
      if (inst->logLrtTimeAvgW32[i] < 65300) {
        tmp32no1 = WEBRTC_SPL_RSHIFT_W32(WEBRTC_SPL_MUL(
                                           inst->logLrtTimeAvgW32[i], 23637),
                                         14); 
        intPart = (int16_t)WEBRTC_SPL_RSHIFT_W32(tmp32no1, 12);
        if (intPart < -8) {
          intPart = -8;
        }
        frac = (int16_t)(tmp32no1 & 0x00000fff); 

        
        tmp32no2 = WEBRTC_SPL_RSHIFT_W32(frac * frac * 44, 19); 
        tmp32no2 += WEBRTC_SPL_MUL_16_16_RSFT(frac, 84, 7); 
        invLrtFX = WEBRTC_SPL_LSHIFT_W32(1, 8 + intPart)
                   + WEBRTC_SPL_SHIFT_W32(tmp32no2, intPart - 4); 

        normTmp = WebRtcSpl_NormW32(invLrtFX);
        normTmp2 = WebRtcSpl_NormW16((16384 - inst->priorNonSpeechProb));
        if (normTmp + normTmp2 >= 7) {
          if (normTmp + normTmp2 < 15) {
            invLrtFX = WEBRTC_SPL_RSHIFT_W32(invLrtFX, 15 - normTmp2 - normTmp);
            
            tmp32no1 = WEBRTC_SPL_MUL_32_16(invLrtFX,
                                            (16384 - inst->priorNonSpeechProb));
            
            invLrtFX = WEBRTC_SPL_SHIFT_W32(tmp32no1, 7 - normTmp - normTmp2);
                                                                  
          } else {
            tmp32no1 = WEBRTC_SPL_MUL_32_16(invLrtFX,
                                            (16384 - inst->priorNonSpeechProb));
                                                                  
            invLrtFX = WEBRTC_SPL_RSHIFT_W32(tmp32no1, 8); 
          }

          tmp32no1 = WEBRTC_SPL_LSHIFT_W32((int32_t)inst->priorNonSpeechProb,
                                           8); 

          nonSpeechProbFinal[i] = (uint16_t)WEBRTC_SPL_DIV(tmp32no1,
              (int32_t)inst->priorNonSpeechProb + invLrtFX); 
        }
      }
    }
  }
}

