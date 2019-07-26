









#include <string.h>
#include <math.h>

#include <stdlib.h>
#include "noise_suppression.h"
#include "ns_core.h"
#include "windows_private.h"
#include "fft4g.h"
#include "signal_processing_library.h"


void WebRtcNs_set_feature_extraction_parameters(NSinst_t* inst) {
  
  inst->featureExtractionParams.binSizeLrt      = (float)0.1;
  inst->featureExtractionParams.binSizeSpecFlat = (float)0.05;
  inst->featureExtractionParams.binSizeSpecDiff = (float)0.1;

  
  inst->featureExtractionParams.rangeAvgHistLrt = (float)1.0;

  
  
  inst->featureExtractionParams.factor1ModelPars = (float)1.20; 
  inst->featureExtractionParams.factor2ModelPars = (float)0.9;  
  

  
  inst->featureExtractionParams.thresPosSpecFlat = (float)0.6;

  
  inst->featureExtractionParams.limitPeakSpacingSpecFlat = 
      2 * inst->featureExtractionParams.binSizeSpecFlat;
  inst->featureExtractionParams.limitPeakSpacingSpecDiff =
      2 * inst->featureExtractionParams.binSizeSpecDiff;

  
  inst->featureExtractionParams.limitPeakWeightsSpecFlat = (float)0.5;
  inst->featureExtractionParams.limitPeakWeightsSpecDiff = (float)0.5;

  
  inst->featureExtractionParams.thresFluctLrt = (float)0.05;

  
  inst->featureExtractionParams.maxLrt = (float)1.0;
  inst->featureExtractionParams.minLrt = (float)0.20;

  inst->featureExtractionParams.maxSpecFlat = (float)0.95;
  inst->featureExtractionParams.minSpecFlat = (float)0.10;

  inst->featureExtractionParams.maxSpecDiff = (float)1.0;
  inst->featureExtractionParams.minSpecDiff = (float)0.16;

  
  inst->featureExtractionParams.thresWeightSpecFlat = (int)(0.3
      * (inst->modelUpdatePars[1])); 
  inst->featureExtractionParams.thresWeightSpecDiff = (int)(0.3
      * (inst->modelUpdatePars[1])); 
}


int WebRtcNs_InitCore(NSinst_t* inst, WebRtc_UWord32 fs) {
  int i;
  

  
  if (inst == NULL) {
    return -1;
  }

  
  if (fs == 8000 || fs == 16000 || fs == 32000) {
    inst->fs = fs;
  } else {
    return -1;
  }
  inst->windShift = 0;
  if (fs == 8000) {
    
    inst->blockLen = 80;
    inst->blockLen10ms = 80;
    inst->anaLen = 128;
    inst->window = kBlocks80w128;
    inst->outLen = 0;
  } else if (fs == 16000) {
    
    inst->blockLen = 160;
    inst->blockLen10ms = 160;
    inst->anaLen = 256;
    inst->window = kBlocks160w256;
    inst->outLen = 0;
  } else if (fs == 32000) {
    
    inst->blockLen = 160;
    inst->blockLen10ms = 160;
    inst->anaLen = 256;
    inst->window = kBlocks160w256;
    inst->outLen = 0;
  }
  inst->magnLen = inst->anaLen / 2 + 1; 

  
  inst->ip[0] = 0; 
  memset(inst->dataBuf, 0, sizeof(float) * ANAL_BLOCKL_MAX);
  WebRtc_rdft(inst->anaLen, 1, inst->dataBuf, inst->ip, inst->wfft);

  memset(inst->dataBuf, 0, sizeof(float) * ANAL_BLOCKL_MAX);
  memset(inst->syntBuf, 0, sizeof(float) * ANAL_BLOCKL_MAX);

  
  memset(inst->dataBufHB, 0, sizeof(float) * ANAL_BLOCKL_MAX);

  
  memset(inst->quantile, 0, sizeof(float) * HALF_ANAL_BLOCKL);
  for (i = 0; i < SIMULT * HALF_ANAL_BLOCKL; i++) {
    inst->lquantile[i] = (float)8.0;
    inst->density[i] = (float)0.3;
  }

  for (i = 0; i < SIMULT; i++) {
    inst->counter[i] = (int)floor((float)(END_STARTUP_LONG * (i + 1)) / (float)SIMULT);
  }

  inst->updates = 0;

  
  for (i = 0; i < HALF_ANAL_BLOCKL; i++) {
    inst->smooth[i] = (float)1.0;
  }

  
  inst->aggrMode = 0;

  
  inst->priorSpeechProb = (float)0.5; 
  for (i = 0; i < HALF_ANAL_BLOCKL; i++) {
    inst->magnPrev[i]      = (float)0.0; 
    inst->noisePrev[i]     = (float)0.0; 
    inst->logLrtTimeAvg[i] = LRT_FEATURE_THR; 
    inst->magnAvgPause[i]  = (float)0.0; 
    inst->speechProbHB[i]  = (float)0.0; 
    inst->initMagnEst[i]   = (float)0.0; 
  }

  
  inst->featureData[0] = SF_FEATURE_THR;  
  inst->featureData[1] = (float)0.0;      
  inst->featureData[2] = (float)0.0;      
  inst->featureData[3] = LRT_FEATURE_THR; 
  inst->featureData[4] = SF_FEATURE_THR;  
  inst->featureData[5] = (float)0.0;      
  inst->featureData[6] = (float)0.0;      

  
  for (i = 0; i < HIST_PAR_EST; i++) {
    inst->histLrt[i] = 0;
    inst->histSpecFlat[i] = 0;
    inst->histSpecDiff[i] = 0;
  }

  inst->blockInd = -1; 
  inst->priorModelPars[0] = LRT_FEATURE_THR; 
  inst->priorModelPars[1] = (float)0.5;      
  
  inst->priorModelPars[2] = (float)1.0;      
  
  inst->priorModelPars[3] = (float)0.5;      
  
  inst->priorModelPars[4] = (float)1.0;      
  inst->priorModelPars[5] = (float)0.0;      
  
  inst->priorModelPars[6] = (float)0.0;      
  

  inst->modelUpdatePars[0] = 2;   
  
  inst->modelUpdatePars[1] = 500; 
  inst->modelUpdatePars[2] = 0;   
  
  inst->modelUpdatePars[3] = inst->modelUpdatePars[1];

  inst->signalEnergy = 0.0;
  inst->sumMagn = 0.0;
  inst->whiteNoiseLevel = 0.0;
  inst->pinkNoiseNumerator = 0.0;
  inst->pinkNoiseExp = 0.0;

  WebRtcNs_set_feature_extraction_parameters(inst); 

  
  WebRtcNs_set_policy_core(inst, 0);


  memset(inst->outBuf, 0, sizeof(float) * 3 * BLOCKL_MAX);

  inst->initFlag = 1;
  return 0;
}

int WebRtcNs_set_policy_core(NSinst_t* inst, int mode) {
  
  if (mode < 0 || mode > 3) {
    return (-1);
  }

  inst->aggrMode = mode;
  if (mode == 0) {
    inst->overdrive = (float)1.0;
    inst->denoiseBound = (float)0.5;
    inst->gainmap = 0;
  } else if (mode == 1) {
    
    inst->overdrive = (float)1.0;
    inst->denoiseBound = (float)0.25;
    inst->gainmap = 1;
  } else if (mode == 2) {
    
    inst->overdrive = (float)1.1;
    inst->denoiseBound = (float)0.125;
    inst->gainmap = 1;
  } else if (mode == 3) {
    
    inst->overdrive = (float)1.25;
    inst->denoiseBound = (float)0.09;
    inst->gainmap = 1;
  }
  return 0;
}


void WebRtcNs_NoiseEstimation(NSinst_t* inst, float* magn, float* noise) {
  int i, s, offset;
  float lmagn[HALF_ANAL_BLOCKL], delta;

  if (inst->updates < END_STARTUP_LONG) {
    inst->updates++;
  }

  for (i = 0; i < inst->magnLen; i++) {
    lmagn[i] = (float)log(magn[i]);
  }

  
  for (s = 0; s < SIMULT; s++) {
    offset = s * inst->magnLen;

    
    for (i = 0; i < inst->magnLen; i++) {
      
      if (inst->density[offset + i] > 1.0) {
        delta = FACTOR * (float)1.0 / inst->density[offset + i];
      } else {
        delta = FACTOR;
      }

      
      if (lmagn[i] > inst->lquantile[offset + i]) {
        inst->lquantile[offset + i] += QUANTILE * delta
                                       / (float)(inst->counter[s] + 1);
      } else {
        inst->lquantile[offset + i] -= ((float)1.0 - QUANTILE) * delta
                                       / (float)(inst->counter[s] + 1);
      }

      
      if (fabs(lmagn[i] - inst->lquantile[offset + i]) < WIDTH) {
        inst->density[offset + i] = ((float)inst->counter[s] * inst->density[offset
            + i] + (float)1.0 / ((float)2.0 * WIDTH)) / (float)(inst->counter[s] + 1);
      }
    } 

    if (inst->counter[s] >= END_STARTUP_LONG) {
      inst->counter[s] = 0;
      if (inst->updates >= END_STARTUP_LONG) {
        for (i = 0; i < inst->magnLen; i++) {
          inst->quantile[i] = (float)exp(inst->lquantile[offset + i]);
        }
      }
    }

    inst->counter[s]++;
  } 

  
  if (inst->updates < END_STARTUP_LONG) {
    
    for (i = 0; i < inst->magnLen; i++) {
      inst->quantile[i] = (float)exp(inst->lquantile[offset + i]);
    }
  }

  for (i = 0; i < inst->magnLen; i++) {
    noise[i] = inst->quantile[i];
  }
}






void WebRtcNs_FeatureParameterExtraction(NSinst_t* inst, int flag) {
  int i, useFeatureSpecFlat, useFeatureSpecDiff, numHistLrt;
  int maxPeak1, maxPeak2;
  int weightPeak1SpecFlat, weightPeak2SpecFlat, weightPeak1SpecDiff, weightPeak2SpecDiff;

  float binMid, featureSum;
  float posPeak1SpecFlat, posPeak2SpecFlat, posPeak1SpecDiff, posPeak2SpecDiff;
  float fluctLrt, avgHistLrt, avgSquareHistLrt, avgHistLrtCompl;

  
  
  
  

  
  if (flag == 0) {
    
    if ((inst->featureData[3] < HIST_PAR_EST * inst->featureExtractionParams.binSizeLrt)
        && (inst->featureData[3] >= 0.0)) {
      i = (int)(inst->featureData[3] / inst->featureExtractionParams.binSizeLrt);
      inst->histLrt[i]++;
    }
    
    if ((inst->featureData[0] < HIST_PAR_EST
         * inst->featureExtractionParams.binSizeSpecFlat)
        && (inst->featureData[0] >= 0.0)) {
      i = (int)(inst->featureData[0] / inst->featureExtractionParams.binSizeSpecFlat);
      inst->histSpecFlat[i]++;
    }
    
    if ((inst->featureData[4] < HIST_PAR_EST
         * inst->featureExtractionParams.binSizeSpecDiff)
        && (inst->featureData[4] >= 0.0)) {
      i = (int)(inst->featureData[4] / inst->featureExtractionParams.binSizeSpecDiff);
      inst->histSpecDiff[i]++;
    }
  }

  
  if (flag == 1) {
    
    avgHistLrt = 0.0;
    avgHistLrtCompl = 0.0;
    avgSquareHistLrt = 0.0;
    numHistLrt = 0;
    for (i = 0; i < HIST_PAR_EST; i++) {
      binMid = ((float)i + (float)0.5) * inst->featureExtractionParams.binSizeLrt;
      if (binMid <= inst->featureExtractionParams.rangeAvgHistLrt) {
        avgHistLrt += inst->histLrt[i] * binMid;
        numHistLrt += inst->histLrt[i];
      }
      avgSquareHistLrt += inst->histLrt[i] * binMid * binMid;
      avgHistLrtCompl += inst->histLrt[i] * binMid;
    }
    if (numHistLrt > 0) {
      avgHistLrt = avgHistLrt / ((float)numHistLrt);
    }
    avgHistLrtCompl = avgHistLrtCompl / ((float)inst->modelUpdatePars[1]);
    avgSquareHistLrt = avgSquareHistLrt / ((float)inst->modelUpdatePars[1]);
    fluctLrt = avgSquareHistLrt - avgHistLrt * avgHistLrtCompl;
    
    if (fluctLrt < inst->featureExtractionParams.thresFluctLrt) {
      
      inst->priorModelPars[0] = inst->featureExtractionParams.maxLrt;
    } else {
      inst->priorModelPars[0] = inst->featureExtractionParams.factor1ModelPars
                                * avgHistLrt;
      
      if (inst->priorModelPars[0] < inst->featureExtractionParams.minLrt) {
        inst->priorModelPars[0] = inst->featureExtractionParams.minLrt;
      }
      if (inst->priorModelPars[0] > inst->featureExtractionParams.maxLrt) {
        inst->priorModelPars[0] = inst->featureExtractionParams.maxLrt;
      }
    }
    

    
    
    maxPeak1 = 0;
    maxPeak2 = 0;
    posPeak1SpecFlat = 0.0;
    posPeak2SpecFlat = 0.0;
    weightPeak1SpecFlat = 0;
    weightPeak2SpecFlat = 0;

    
    for (i = 0; i < HIST_PAR_EST; i++) {
      binMid = ((float)i + (float)0.5) * inst->featureExtractionParams.binSizeSpecFlat;
      if (inst->histSpecFlat[i] > maxPeak1) {
        
        maxPeak2 = maxPeak1;
        weightPeak2SpecFlat = weightPeak1SpecFlat;
        posPeak2SpecFlat = posPeak1SpecFlat;

        maxPeak1 = inst->histSpecFlat[i];
        weightPeak1SpecFlat = inst->histSpecFlat[i];
        posPeak1SpecFlat = binMid;
      } else if (inst->histSpecFlat[i] > maxPeak2) {
        
        maxPeak2 = inst->histSpecFlat[i];
        weightPeak2SpecFlat = inst->histSpecFlat[i];
        posPeak2SpecFlat = binMid;
      }
    }

    
    maxPeak1 = 0;
    maxPeak2 = 0;
    posPeak1SpecDiff = 0.0;
    posPeak2SpecDiff = 0.0;
    weightPeak1SpecDiff = 0;
    weightPeak2SpecDiff = 0;
    
    for (i = 0; i < HIST_PAR_EST; i++) {
      binMid = ((float)i + (float)0.5) * inst->featureExtractionParams.binSizeSpecDiff;
      if (inst->histSpecDiff[i] > maxPeak1) {
        
        maxPeak2 = maxPeak1;
        weightPeak2SpecDiff = weightPeak1SpecDiff;
        posPeak2SpecDiff = posPeak1SpecDiff;

        maxPeak1 = inst->histSpecDiff[i];
        weightPeak1SpecDiff = inst->histSpecDiff[i];
        posPeak1SpecDiff = binMid;
      } else if (inst->histSpecDiff[i] > maxPeak2) {
        
        maxPeak2 = inst->histSpecDiff[i];
        weightPeak2SpecDiff = inst->histSpecDiff[i];
        posPeak2SpecDiff = binMid;
      }
    }

    
    useFeatureSpecFlat = 1;
    
    if ((fabs(posPeak2SpecFlat - posPeak1SpecFlat)
         < inst->featureExtractionParams.limitPeakSpacingSpecFlat)
        && (weightPeak2SpecFlat
            > inst->featureExtractionParams.limitPeakWeightsSpecFlat
            * weightPeak1SpecFlat)) {
      weightPeak1SpecFlat += weightPeak2SpecFlat;
      posPeak1SpecFlat = (float)0.5 * (posPeak1SpecFlat + posPeak2SpecFlat);
    }
    
    if (weightPeak1SpecFlat < inst->featureExtractionParams.thresWeightSpecFlat
        || posPeak1SpecFlat < inst->featureExtractionParams.thresPosSpecFlat) {
      useFeatureSpecFlat = 0;
    }
    
    if (useFeatureSpecFlat == 1) {
      
      inst->priorModelPars[1] = inst->featureExtractionParams.factor2ModelPars
                                * posPeak1SpecFlat;
      
      if (inst->priorModelPars[1] < inst->featureExtractionParams.minSpecFlat) {
        inst->priorModelPars[1] = inst->featureExtractionParams.minSpecFlat;
      }
      if (inst->priorModelPars[1] > inst->featureExtractionParams.maxSpecFlat) {
        inst->priorModelPars[1] = inst->featureExtractionParams.maxSpecFlat;
      }
    }
    

    
    useFeatureSpecDiff = 1;
    
    if ((fabs(posPeak2SpecDiff - posPeak1SpecDiff)
         < inst->featureExtractionParams.limitPeakSpacingSpecDiff)
        && (weightPeak2SpecDiff
            > inst->featureExtractionParams.limitPeakWeightsSpecDiff
            * weightPeak1SpecDiff)) {
      weightPeak1SpecDiff += weightPeak2SpecDiff;
      posPeak1SpecDiff = (float)0.5 * (posPeak1SpecDiff + posPeak2SpecDiff);
    }
    
    inst->priorModelPars[3] = inst->featureExtractionParams.factor1ModelPars
                              * posPeak1SpecDiff;
    
    if (weightPeak1SpecDiff < inst->featureExtractionParams.thresWeightSpecDiff) {
      useFeatureSpecDiff = 0;
    }
    
    if (inst->priorModelPars[3] < inst->featureExtractionParams.minSpecDiff) {
      inst->priorModelPars[3] = inst->featureExtractionParams.minSpecDiff;
    }
    if (inst->priorModelPars[3] > inst->featureExtractionParams.maxSpecDiff) {
      inst->priorModelPars[3] = inst->featureExtractionParams.maxSpecDiff;
    }
    

    
    
    if (fluctLrt < inst->featureExtractionParams.thresFluctLrt) {
      useFeatureSpecDiff = 0;
    }

    
    
    
    
    featureSum = (float)(1 + useFeatureSpecFlat + useFeatureSpecDiff);
    inst->priorModelPars[4] = (float)1.0 / featureSum;
    inst->priorModelPars[5] = ((float)useFeatureSpecFlat) / featureSum;
    inst->priorModelPars[6] = ((float)useFeatureSpecDiff) / featureSum;

    
    if (inst->modelUpdatePars[0] >= 1) {
      for (i = 0; i < HIST_PAR_EST; i++) {
        inst->histLrt[i] = 0;
        inst->histSpecFlat[i] = 0;
        inst->histSpecDiff[i] = 0;
      }
    }
  } 
}




void WebRtcNs_ComputeSpectralFlatness(NSinst_t* inst, float* magnIn) {
  int i;
  int shiftLP = 1; 
  float avgSpectralFlatnessNum, avgSpectralFlatnessDen, spectralTmp;

  
  
  avgSpectralFlatnessNum = 0.0;
  avgSpectralFlatnessDen = inst->sumMagn;
  for (i = 0; i < shiftLP; i++) {
    avgSpectralFlatnessDen -= magnIn[i];
  }
  
  for (i = shiftLP; i < inst->magnLen; i++) {
    if (magnIn[i] > 0.0) {
      avgSpectralFlatnessNum += (float)log(magnIn[i]);
    } else {
      inst->featureData[0] -= SPECT_FL_TAVG * inst->featureData[0];
      return;
    }
  }
  
  avgSpectralFlatnessDen = avgSpectralFlatnessDen / inst->magnLen;
  avgSpectralFlatnessNum = avgSpectralFlatnessNum / inst->magnLen;

  
  spectralTmp = (float)exp(avgSpectralFlatnessNum) / avgSpectralFlatnessDen;

  
  inst->featureData[0] += SPECT_FL_TAVG * (spectralTmp - inst->featureData[0]);
  
}





void WebRtcNs_ComputeSpectralDifference(NSinst_t* inst, float* magnIn) {
  
  int i;
  float avgPause, avgMagn, covMagnPause, varPause, varMagn, avgDiffNormMagn;

  avgPause = 0.0;
  avgMagn = inst->sumMagn;
  
  for (i = 0; i < inst->magnLen; i++) {
    
    avgPause += inst->magnAvgPause[i];
  }
  avgPause = avgPause / ((float)inst->magnLen);
  avgMagn = avgMagn / ((float)inst->magnLen);

  covMagnPause = 0.0;
  varPause = 0.0;
  varMagn = 0.0;
  
  for (i = 0; i < inst->magnLen; i++) {
    covMagnPause += (magnIn[i] - avgMagn) * (inst->magnAvgPause[i] - avgPause);
    varPause += (inst->magnAvgPause[i] - avgPause) * (inst->magnAvgPause[i] - avgPause);
    varMagn += (magnIn[i] - avgMagn) * (magnIn[i] - avgMagn);
  }
  covMagnPause = covMagnPause / ((float)inst->magnLen);
  varPause = varPause / ((float)inst->magnLen);
  varMagn = varMagn / ((float)inst->magnLen);
  
  inst->featureData[6] += inst->signalEnergy;

  avgDiffNormMagn = varMagn - (covMagnPause * covMagnPause) / (varPause + (float)0.0001);
  
  avgDiffNormMagn = (float)(avgDiffNormMagn / (inst->featureData[5] + (float)0.0001));
  inst->featureData[4] += SPECT_DIFF_TAVG * (avgDiffNormMagn - inst->featureData[4]);
}







void WebRtcNs_SpeechNoiseProb(NSinst_t* inst, float* probSpeechFinal, float* snrLocPrior,
                              float* snrLocPost) {
  int i, sgnMap;
  float invLrt, gainPrior, indPrior;
  float logLrtTimeAvgKsum, besselTmp;
  float indicator0, indicator1, indicator2;
  float tmpFloat1, tmpFloat2;
  float weightIndPrior0, weightIndPrior1, weightIndPrior2;
  float threshPrior0, threshPrior1, threshPrior2;
  float widthPrior, widthPrior0, widthPrior1, widthPrior2;

  widthPrior0 = WIDTH_PR_MAP;
  widthPrior1 = (float)2.0 * WIDTH_PR_MAP; 
  
  widthPrior2 = (float)2.0 * WIDTH_PR_MAP; 

  
  threshPrior0 = inst->priorModelPars[0];
  threshPrior1 = inst->priorModelPars[1];
  threshPrior2 = inst->priorModelPars[3];

  
  sgnMap = (int)(inst->priorModelPars[2]);

  
  weightIndPrior0 = inst->priorModelPars[4];
  weightIndPrior1 = inst->priorModelPars[5];
  weightIndPrior2 = inst->priorModelPars[6];

  
  
  logLrtTimeAvgKsum = 0.0;
  for (i = 0; i < inst->magnLen; i++) {
    tmpFloat1 = (float)1.0 + (float)2.0 * snrLocPrior[i];
    tmpFloat2 = (float)2.0 * snrLocPrior[i] / (tmpFloat1 + (float)0.0001);
    besselTmp = (snrLocPost[i] + (float)1.0) * tmpFloat2;
    inst->logLrtTimeAvg[i] += LRT_TAVG * (besselTmp - (float)log(tmpFloat1)
                                          - inst->logLrtTimeAvg[i]);
    logLrtTimeAvgKsum += inst->logLrtTimeAvg[i];
  }
  logLrtTimeAvgKsum = (float)logLrtTimeAvgKsum / (inst->magnLen);
  inst->featureData[3] = logLrtTimeAvgKsum;
  

  
  
  

  
  widthPrior = widthPrior0;
  
  if (logLrtTimeAvgKsum < threshPrior0) {
    widthPrior = widthPrior1;
  }
  
  indicator0 = (float)0.5 * ((float)tanh(widthPrior *
      (logLrtTimeAvgKsum - threshPrior0)) + (float)1.0);

  
  tmpFloat1 = inst->featureData[0];
  widthPrior = widthPrior0;
  
  if (sgnMap == 1 && (tmpFloat1 > threshPrior1)) {
    widthPrior = widthPrior1;
  }
  if (sgnMap == -1 && (tmpFloat1 < threshPrior1)) {
    widthPrior = widthPrior1;
  }
  
  indicator1 = (float)0.5 * ((float)tanh((float)sgnMap * 
      widthPrior * (threshPrior1 - tmpFloat1)) + (float)1.0);

  
  tmpFloat1 = inst->featureData[4];
  widthPrior = widthPrior0;
  
  if (tmpFloat1 < threshPrior2) {
    widthPrior = widthPrior2;
  }
  
  indicator2 = (float)0.5 * ((float)tanh(widthPrior * (tmpFloat1 - threshPrior2))
                             + (float)1.0);

  
  indPrior = weightIndPrior0 * indicator0 + weightIndPrior1 * indicator1 + weightIndPrior2
             * indicator2;
  

  
  inst->priorSpeechProb += PRIOR_UPDATE * (indPrior - inst->priorSpeechProb);
  
  if (inst->priorSpeechProb > 1.0) {
    inst->priorSpeechProb = (float)1.0;
  }
  if (inst->priorSpeechProb < 0.01) {
    inst->priorSpeechProb = (float)0.01;
  }

  
  gainPrior = ((float)1.0 - inst->priorSpeechProb) / (inst->priorSpeechProb + (float)0.0001);
  for (i = 0; i < inst->magnLen; i++) {
    invLrt = (float)exp(-inst->logLrtTimeAvg[i]);
    invLrt = (float)gainPrior * invLrt;
    probSpeechFinal[i] = (float)1.0 / ((float)1.0 + invLrt);
  }
}

int WebRtcNs_ProcessCore(NSinst_t* inst,
                         short* speechFrame,
                         short* speechFrameHB,
                         short* outFrame,
                         short* outFrameHB) {
  

  int     flagHB = 0;
  int     i;
  const int kStartBand = 5; 
  int     updateParsFlag;

  float   energy1, energy2, gain, factor, factor1, factor2;
  float   signalEnergy, sumMagn;
  float   snrPrior, currentEstimateStsa;
  float   tmpFloat1, tmpFloat2, tmpFloat3, probSpeech, probNonSpeech;
  float   gammaNoiseTmp, gammaNoiseOld;
  float   noiseUpdateTmp, fTmp, dTmp;
  float   fin[BLOCKL_MAX], fout[BLOCKL_MAX];
  float   winData[ANAL_BLOCKL_MAX];
  float   magn[HALF_ANAL_BLOCKL], noise[HALF_ANAL_BLOCKL];
  float   theFilter[HALF_ANAL_BLOCKL], theFilterTmp[HALF_ANAL_BLOCKL];
  float   snrLocPost[HALF_ANAL_BLOCKL], snrLocPrior[HALF_ANAL_BLOCKL];
  float   probSpeechFinal[HALF_ANAL_BLOCKL] = { 0 };
  float   previousEstimateStsa[HALF_ANAL_BLOCKL];
  float   real[ANAL_BLOCKL_MAX], imag[HALF_ANAL_BLOCKL];
  
  float   sum_log_i = 0.0;
  float   sum_log_i_square = 0.0;
  float   sum_log_magn = 0.0;
  float   sum_log_i_log_magn = 0.0;
  float   parametric_noise = 0.0;
  float   parametric_exp = 0.0;
  float   parametric_num = 0.0;

  
  int     deltaBweHB = 1;
  int     deltaGainHB = 1;
  float   decayBweHB = 1.0;
  float   gainMapParHB = 1.0;
  float   gainTimeDomainHB = 1.0;
  float   avgProbSpeechHB, avgProbSpeechHBTmp, avgFilterGainHB, gainModHB;

  
  if (inst->initFlag != 1) {
    return (-1);
  }
  
  if (inst->fs == 32000) {
    if (speechFrameHB == NULL) {
      return -1;
    }
    flagHB = 1;
    
    deltaBweHB = (int)inst->magnLen / 4;
    deltaGainHB = deltaBweHB;
  }
  
  updateParsFlag = inst->modelUpdatePars[0];
  

  
  
  for (i = 0; i < inst->blockLen10ms; i++) {
    fin[i] = (float)speechFrame[i];
  }
  
  memcpy(inst->dataBuf, inst->dataBuf + inst->blockLen10ms,
         sizeof(float) * (inst->anaLen - inst->blockLen10ms));
  memcpy(inst->dataBuf + inst->anaLen - inst->blockLen10ms, fin,
         sizeof(float) * inst->blockLen10ms);

  if (flagHB == 1) {
    
    for (i = 0; i < inst->blockLen10ms; i++) {
      fin[i] = (float)speechFrameHB[i];
    }
    
    memcpy(inst->dataBufHB, inst->dataBufHB + inst->blockLen10ms,
           sizeof(float) * (inst->anaLen - inst->blockLen10ms));
    memcpy(inst->dataBufHB + inst->anaLen - inst->blockLen10ms, fin,
           sizeof(float) * inst->blockLen10ms);
  }

  
  if (inst->outLen == 0) {
    
    energy1 = 0.0;
    for (i = 0; i < inst->anaLen; i++) {
      winData[i] = inst->window[i] * inst->dataBuf[i];
      energy1 += winData[i] * winData[i];
    }
    if (energy1 == 0.0) {
      
      
      
      
      
      
      
      

      
      for (i = inst->windShift; i < inst->blockLen + inst->windShift; i++) {
        fout[i - inst->windShift] = inst->syntBuf[i];
      }
      
      memcpy(inst->syntBuf, inst->syntBuf + inst->blockLen,
             sizeof(float) * (inst->anaLen - inst->blockLen));
      memset(inst->syntBuf + inst->anaLen - inst->blockLen, 0,
             sizeof(float) * inst->blockLen);

      
      inst->outLen = inst->blockLen - inst->blockLen10ms;
      if (inst->blockLen > inst->blockLen10ms) {
        for (i = 0; i < inst->outLen; i++) {
          inst->outBuf[i] = fout[i + inst->blockLen10ms];
        }
      }
      
      for (i = 0; i < inst->blockLen10ms; i++) {
        dTmp = fout[i];
        if (dTmp < WEBRTC_SPL_WORD16_MIN) {
          dTmp = WEBRTC_SPL_WORD16_MIN;
        } else if (dTmp > WEBRTC_SPL_WORD16_MAX) {
          dTmp = WEBRTC_SPL_WORD16_MAX;
        }
        outFrame[i] = (short)dTmp;
      }

      
      if (flagHB == 1) {
        for (i = 0; i < inst->blockLen10ms; i++) {
          dTmp = inst->dataBufHB[i];
          if (dTmp < WEBRTC_SPL_WORD16_MIN) {
            dTmp = WEBRTC_SPL_WORD16_MIN;
          } else if (dTmp > WEBRTC_SPL_WORD16_MAX) {
            dTmp = WEBRTC_SPL_WORD16_MAX;
          }
          outFrameHB[i] = (short)dTmp;
        }
      } 
      
      return 0;
    }

    
    inst->blockInd++; 
    
    WebRtc_rdft(inst->anaLen, 1, winData, inst->ip, inst->wfft);

    imag[0] = 0;
    real[0] = winData[0];
    magn[0] = (float)(fabs(real[0]) + 1.0f);
    imag[inst->magnLen - 1] = 0;
    real[inst->magnLen - 1] = winData[1];
    magn[inst->magnLen - 1] = (float)(fabs(real[inst->magnLen - 1]) + 1.0f);
    signalEnergy = (float)(real[0] * real[0]) + 
                   (float)(real[inst->magnLen - 1] * real[inst->magnLen - 1]);
    sumMagn = magn[0] + magn[inst->magnLen - 1];
    if (inst->blockInd < END_STARTUP_SHORT) {
      inst->initMagnEst[0] += magn[0];
      inst->initMagnEst[inst->magnLen - 1] += magn[inst->magnLen - 1];
      tmpFloat2 = log((float)(inst->magnLen - 1));
      sum_log_i = tmpFloat2;
      sum_log_i_square = tmpFloat2 * tmpFloat2;
      tmpFloat1 = log(magn[inst->magnLen - 1]);
      sum_log_magn = tmpFloat1;
      sum_log_i_log_magn = tmpFloat2 * tmpFloat1;
    }
    for (i = 1; i < inst->magnLen - 1; i++) {
      real[i] = winData[2 * i];
      imag[i] = winData[2 * i + 1];
      
      fTmp = real[i] * real[i];
      fTmp += imag[i] * imag[i];
      signalEnergy += fTmp;
      magn[i] = ((float)sqrt(fTmp)) + 1.0f;
      sumMagn += magn[i];
      if (inst->blockInd < END_STARTUP_SHORT) {
        inst->initMagnEst[i] += magn[i];
        if (i >= kStartBand) {
          tmpFloat2 = log((float)i);
          sum_log_i += tmpFloat2;
          sum_log_i_square += tmpFloat2 * tmpFloat2;
          tmpFloat1 = log(magn[i]);
          sum_log_magn += tmpFloat1;
          sum_log_i_log_magn += tmpFloat2 * tmpFloat1;
        }
      }
    }
    signalEnergy = signalEnergy / ((float)inst->magnLen);
    inst->signalEnergy = signalEnergy;
    inst->sumMagn = sumMagn;

    
    WebRtcNs_ComputeSpectralFlatness(inst, magn);
    
    WebRtcNs_NoiseEstimation(inst, magn, noise);
    
    if (inst->blockInd < END_STARTUP_SHORT) {
      
      inst->whiteNoiseLevel += sumMagn / ((float)inst->magnLen) * inst->overdrive;
      
      tmpFloat1 = sum_log_i_square * ((float)(inst->magnLen - kStartBand));
      tmpFloat1 -= (sum_log_i * sum_log_i);
      tmpFloat2 = (sum_log_i_square * sum_log_magn - sum_log_i * sum_log_i_log_magn);
      tmpFloat3 = tmpFloat2 / tmpFloat1;
      
      if (tmpFloat3 < 0.0f) {
        tmpFloat3 = 0.0f;
      }
      inst->pinkNoiseNumerator += tmpFloat3;
      tmpFloat2 = (sum_log_i * sum_log_magn);
      tmpFloat2 -= ((float)(inst->magnLen - kStartBand)) * sum_log_i_log_magn;
      tmpFloat3 = tmpFloat2 / tmpFloat1;
      
      if (tmpFloat3 < 0.0f) {
        tmpFloat3 = 0.0f;
      }
      if (tmpFloat3 > 1.0f) {
        tmpFloat3 = 1.0f;
      }
      inst->pinkNoiseExp += tmpFloat3;

      
      if (inst->pinkNoiseExp == 0.0f) {
        
        parametric_noise = inst->whiteNoiseLevel;
      } else {
        
        parametric_num = exp(inst->pinkNoiseNumerator / (float)(inst->blockInd + 1));
        parametric_num *= (float)(inst->blockInd + 1);
        parametric_exp = inst->pinkNoiseExp / (float)(inst->blockInd + 1);
        parametric_noise = parametric_num / pow((float)kStartBand, parametric_exp);
      }
      for (i = 0; i < inst->magnLen; i++) {
        
        if ((inst->pinkNoiseExp > 0.0f) && (i >= kStartBand)) {
          
          parametric_noise = parametric_num / pow((float)i, parametric_exp);
        }
        theFilterTmp[i] = (inst->initMagnEst[i] - inst->overdrive * parametric_noise);
        theFilterTmp[i] /= (inst->initMagnEst[i] + (float)0.0001);
        
        noise[i] *= (inst->blockInd);
        tmpFloat2 = parametric_noise * (END_STARTUP_SHORT - inst->blockInd);
        noise[i] += (tmpFloat2 / (float)(inst->blockInd + 1));
        noise[i] /= END_STARTUP_SHORT;
      }
    }
    
    
    if (inst->blockInd < END_STARTUP_LONG) {
      inst->featureData[5] *= inst->blockInd;
      inst->featureData[5] += signalEnergy;
      inst->featureData[5] /= (inst->blockInd + 1);
    }

#ifdef PROCESS_FLOW_0
    if (inst->blockInd > END_STARTUP_LONG) {
      
      for (i = 0; i < inst->magnLen; i++) {
        noise[i] = (float)0.6 * inst->noisePrev[i] + (float)0.4 * noise[i];
      }
      for (i = 0; i < inst->magnLen; i++) {
        
        theFilter[i] = (magn[i] - inst->overdrive * noise[i]) / (magn[i] + (float)0.0001);
      }
    }
#else
    
    
    
    

    
    for (i = 0; i < inst->magnLen; i++) {
      
      snrLocPost[i] = (float)0.0;
      if (magn[i] > noise[i]) {
        snrLocPost[i] = magn[i] / (noise[i] + (float)0.0001) - (float)1.0;
      }
      
      
      previousEstimateStsa[i] = inst->magnPrev[i] / (inst->noisePrev[i] + (float)0.0001)
                                * (inst->smooth[i]);
      
      
      snrLocPrior[i] = DD_PR_SNR * previousEstimateStsa[i] + ((float)1.0 - DD_PR_SNR)
                       * snrLocPost[i];
      
    } 
#ifdef PROCESS_FLOW_1
    for (i = 0; i < inst->magnLen; i++) {
      
      tmpFloat1 = inst->overdrive + snrLocPrior[i];
      tmpFloat2 = (float)snrLocPrior[i] / tmpFloat1;
      theFilter[i] = (float)tmpFloat2;
    } 
#endif
    

    
    
    
#ifdef PROCESS_FLOW_2
    
    WebRtcNs_ComputeSpectralDifference(inst, magn);
    
    
    if (updateParsFlag >= 1) {
      
      inst->modelUpdatePars[3]--;
      
      if (inst->modelUpdatePars[3] > 0) {
        WebRtcNs_FeatureParameterExtraction(inst, 0);
      }
      
      if (inst->modelUpdatePars[3] == 0) {
        WebRtcNs_FeatureParameterExtraction(inst, 1);
        inst->modelUpdatePars[3] = inst->modelUpdatePars[1];
        
        if (updateParsFlag == 1) {
          inst->modelUpdatePars[0] = 0;
        } else {
          
          
          inst->featureData[6] = inst->featureData[6]
                                 / ((float)inst->modelUpdatePars[1]);
          inst->featureData[5] = (float)0.5 * (inst->featureData[6]
                                               + inst->featureData[5]);
          inst->featureData[6] = (float)0.0;
        }
      }
    }
    
    WebRtcNs_SpeechNoiseProb(inst, probSpeechFinal, snrLocPrior, snrLocPost);
    
    gammaNoiseTmp = NOISE_UPDATE;
    for (i = 0; i < inst->magnLen; i++) {
      probSpeech = probSpeechFinal[i];
      probNonSpeech = (float)1.0 - probSpeech;
      
      
      noiseUpdateTmp = gammaNoiseTmp * inst->noisePrev[i] + ((float)1.0 - gammaNoiseTmp)
                       * (probNonSpeech * magn[i] + probSpeech * inst->noisePrev[i]);
      
      
      gammaNoiseOld = gammaNoiseTmp;
      gammaNoiseTmp = NOISE_UPDATE;
      
      if (probSpeech > PROB_RANGE) {
        gammaNoiseTmp = SPEECH_UPDATE;
      }
      
      if (probSpeech < PROB_RANGE) {
        inst->magnAvgPause[i] += GAMMA_PAUSE * (magn[i] - inst->magnAvgPause[i]);
      }
      
      if (gammaNoiseTmp == gammaNoiseOld) {
        noise[i] = noiseUpdateTmp;
      } else {
        noise[i] = gammaNoiseTmp * inst->noisePrev[i] + ((float)1.0 - gammaNoiseTmp)
                   * (probNonSpeech * magn[i] + probSpeech * inst->noisePrev[i]);
        
        
        if (noiseUpdateTmp < noise[i]) {
          noise[i] = noiseUpdateTmp;
        }
      }
    } 
    

    
    
    
    for (i = 0; i < inst->magnLen; i++) {
      
      currentEstimateStsa = (float)0.0;
      if (magn[i] > noise[i]) {
        currentEstimateStsa = magn[i] / (noise[i] + (float)0.0001) - (float)1.0;
      }
      
      
      snrPrior = DD_PR_SNR * previousEstimateStsa[i] + ((float)1.0 - DD_PR_SNR)
                 * currentEstimateStsa;
      
      tmpFloat1 = inst->overdrive + snrPrior;
      tmpFloat2 = (float)snrPrior / tmpFloat1;
      theFilter[i] = (float)tmpFloat2;
    } 
    
#endif
#endif

    for (i = 0; i < inst->magnLen; i++) {
      
      if (theFilter[i] < inst->denoiseBound) {
        theFilter[i] = inst->denoiseBound;
      }
      
      if (theFilter[i] > (float)1.0) {
        theFilter[i] = 1.0;
      }
      if (inst->blockInd < END_STARTUP_SHORT) {
        
        if (theFilterTmp[i] < inst->denoiseBound) {
          theFilterTmp[i] = inst->denoiseBound;
        }
        
        if (theFilterTmp[i] > (float)1.0) {
          theFilterTmp[i] = 1.0;
        }
        
        theFilter[i] *= (inst->blockInd);
        theFilterTmp[i] *= (END_STARTUP_SHORT - inst->blockInd);
        theFilter[i] += theFilterTmp[i];
        theFilter[i] /= (END_STARTUP_SHORT);
      }
      
#ifdef PROCESS_FLOW_0
      inst->smooth[i] *= SMOOTH; 
      inst->smooth[i] += ((float)1.0 - SMOOTH) * theFilter[i];
#else
      inst->smooth[i] = theFilter[i];
#endif
      real[i] *= inst->smooth[i];
      imag[i] *= inst->smooth[i];
    }
    
    for (i = 0; i < inst->magnLen; i++) {
      inst->noisePrev[i] = noise[i];
      inst->magnPrev[i] = magn[i];
    }
    
    winData[0] = real[0];
    winData[1] = real[inst->magnLen - 1];
    for (i = 1; i < inst->magnLen - 1; i++) {
      winData[2 * i] = real[i];
      winData[2 * i + 1] = imag[i];
    }
    WebRtc_rdft(inst->anaLen, -1, winData, inst->ip, inst->wfft);

    for (i = 0; i < inst->anaLen; i++) {
      real[i] = 2.0f * winData[i] / inst->anaLen; 
    }

    
    factor = (float)1.0;
    if (inst->gainmap == 1 && inst->blockInd > END_STARTUP_LONG) {
      factor1 = (float)1.0;
      factor2 = (float)1.0;

      energy2 = 0.0;
      for (i = 0; i < inst->anaLen; i++) {
        energy2 += (float)real[i] * (float)real[i];
      }
      gain = (float)sqrt(energy2 / (energy1 + (float)1.0));

#ifdef PROCESS_FLOW_2
      
      if (gain > B_LIM) {
        factor1 = (float)1.0 + (float)1.3 * (gain - B_LIM);
        if (gain * factor1 > (float)1.0) {
          factor1 = (float)1.0 / gain;
        }
      }
      if (gain < B_LIM) {
        
        
        if (gain <= inst->denoiseBound) {
          gain = inst->denoiseBound;
        }
        factor2 = (float)1.0 - (float)0.3 * (B_LIM - gain);
      }
      
      
      factor = inst->priorSpeechProb * factor1 + ((float)1.0 - inst->priorSpeechProb)
               * factor2;
#else
      if (gain > B_LIM) {
        factor = (float)1.0 + (float)1.3 * (gain - B_LIM);
      } else {
        factor = (float)1.0 + (float)2.0 * (gain - B_LIM);
      }
      if (gain * factor > (float)1.0) {
        factor = (float)1.0 / gain;
      }
#endif
    } 

    
    for (i = 0; i < inst->anaLen; i++) {
      inst->syntBuf[i] += factor * inst->window[i] * (float)real[i];
    }
    
    for (i = inst->windShift; i < inst->blockLen + inst->windShift; i++) {
      fout[i - inst->windShift] = inst->syntBuf[i];
    }
    
    memcpy(inst->syntBuf, inst->syntBuf + inst->blockLen,
           sizeof(float) * (inst->anaLen - inst->blockLen));
    memset(inst->syntBuf + inst->anaLen - inst->blockLen, 0,
           sizeof(float) * inst->blockLen);

    
    inst->outLen = inst->blockLen - inst->blockLen10ms;
    if (inst->blockLen > inst->blockLen10ms) {
      for (i = 0; i < inst->outLen; i++) {
        inst->outBuf[i] = fout[i + inst->blockLen10ms];
      }
    }
  } 
  else {
    for (i = 0; i < inst->blockLen10ms; i++) {
      fout[i] = inst->outBuf[i];
    }
    memcpy(inst->outBuf, inst->outBuf + inst->blockLen10ms,
           sizeof(float) * (inst->outLen - inst->blockLen10ms));
    memset(inst->outBuf + inst->outLen - inst->blockLen10ms, 0,
           sizeof(float) * inst->blockLen10ms);
    inst->outLen -= inst->blockLen10ms;
  }

  
  for (i = 0; i < inst->blockLen10ms; i++) {
    dTmp = fout[i];
    if (dTmp < WEBRTC_SPL_WORD16_MIN) {
      dTmp = WEBRTC_SPL_WORD16_MIN;
    } else if (dTmp > WEBRTC_SPL_WORD16_MAX) {
      dTmp = WEBRTC_SPL_WORD16_MAX;
    }
    outFrame[i] = (short)dTmp;
  }

  
  if (flagHB == 1) {
    for (i = 0; i < inst->magnLen; i++) {
      inst->speechProbHB[i] = probSpeechFinal[i];
    }
    
    
    avgProbSpeechHB = 0.0;
    for (i = inst->magnLen - deltaBweHB - 1; i < inst->magnLen - 1; i++) {
      avgProbSpeechHB += inst->speechProbHB[i];
    }
    avgProbSpeechHB = avgProbSpeechHB / ((float)deltaBweHB);
    
    
    avgFilterGainHB = 0.0;
    for (i = inst->magnLen - deltaGainHB - 1; i < inst->magnLen - 1; i++) {
      avgFilterGainHB += inst->smooth[i];
    }
    avgFilterGainHB = avgFilterGainHB / ((float)(deltaGainHB));
    avgProbSpeechHBTmp = (float)2.0 * avgProbSpeechHB - (float)1.0;
    
    gainModHB = (float)0.5 * ((float)1.0 + (float)tanh(gainMapParHB * avgProbSpeechHBTmp));
    
    gainTimeDomainHB = (float)0.5 * gainModHB + (float)0.5 * avgFilterGainHB;
    if (avgProbSpeechHB >= (float)0.5) {
      gainTimeDomainHB = (float)0.25 * gainModHB + (float)0.75 * avgFilterGainHB;
    }
    gainTimeDomainHB = gainTimeDomainHB * decayBweHB;
    
    
    if (gainTimeDomainHB < inst->denoiseBound) {
      gainTimeDomainHB = inst->denoiseBound;
    }
    
    if (gainTimeDomainHB > (float)1.0) {
      gainTimeDomainHB = 1.0;
    }
    
    for (i = 0; i < inst->blockLen10ms; i++) {
      dTmp = gainTimeDomainHB * inst->dataBufHB[i];
      if (dTmp < WEBRTC_SPL_WORD16_MIN) {
        dTmp = WEBRTC_SPL_WORD16_MIN;
      } else if (dTmp > WEBRTC_SPL_WORD16_MAX) {
        dTmp = WEBRTC_SPL_WORD16_MAX;
      }
      outFrameHB[i] = (short)dTmp;
    }
  } 
  

  return 0;
}
