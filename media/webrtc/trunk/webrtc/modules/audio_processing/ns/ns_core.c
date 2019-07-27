









#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "webrtc/common_audio/signal_processing/include/signal_processing_library.h"
#include "webrtc/modules/audio_processing/ns/include/noise_suppression.h"
#include "webrtc/modules/audio_processing/ns/ns_core.h"
#include "webrtc/modules/audio_processing/ns/windows_private.h"
#include "webrtc/modules/audio_processing/utility/fft4g.h"


static void set_feature_extraction_parameters(NSinst_t* self) {
  
  self->featureExtractionParams.binSizeLrt = 0.1f;
  self->featureExtractionParams.binSizeSpecFlat = 0.05f;
  self->featureExtractionParams.binSizeSpecDiff = 0.1f;

  
  self->featureExtractionParams.rangeAvgHistLrt = 1.f;

  
  
  
  self->featureExtractionParams.factor1ModelPars = 1.2f;
  
  self->featureExtractionParams.factor2ModelPars = 0.9f;

  
  self->featureExtractionParams.thresPosSpecFlat = 0.6f;

  
  
  self->featureExtractionParams.limitPeakSpacingSpecFlat =
      2 * self->featureExtractionParams.binSizeSpecFlat;
  self->featureExtractionParams.limitPeakSpacingSpecDiff =
      2 * self->featureExtractionParams.binSizeSpecDiff;

  
  self->featureExtractionParams.limitPeakWeightsSpecFlat = 0.5f;
  self->featureExtractionParams.limitPeakWeightsSpecDiff = 0.5f;

  
  self->featureExtractionParams.thresFluctLrt = 0.05f;

  
  self->featureExtractionParams.maxLrt = 1.f;
  self->featureExtractionParams.minLrt = 0.2f;

  self->featureExtractionParams.maxSpecFlat = 0.95f;
  self->featureExtractionParams.minSpecFlat = 0.1f;

  self->featureExtractionParams.maxSpecDiff = 1.f;
  self->featureExtractionParams.minSpecDiff = 0.16f;

  
  self->featureExtractionParams.thresWeightSpecFlat =
      (int)(0.3 * (self->modelUpdatePars[1]));  
  self->featureExtractionParams.thresWeightSpecDiff =
      (int)(0.3 * (self->modelUpdatePars[1]));  
}


int WebRtcNs_InitCore(NSinst_t* self, uint32_t fs) {
  int i;
  
  if (self == NULL) {
    return -1;
  }

  
  if (fs == 8000 || fs == 16000 || fs == 32000) {
    self->fs = fs;
  } else {
    return -1;
  }
  self->windShift = 0;
  if (fs == 8000) {
    
    self->blockLen = 80;
    self->anaLen = 128;
    self->window = kBlocks80w128;
  } else if (fs == 16000) {
    
    self->blockLen = 160;
    self->anaLen = 256;
    self->window = kBlocks160w256;
  } else if (fs == 32000) {
    
    self->blockLen = 160;
    self->anaLen = 256;
    self->window = kBlocks160w256;
  }
  self->magnLen = self->anaLen / 2 + 1;  

  
  self->ip[0] = 0;  
  memset(self->dataBuf, 0, sizeof(float) * ANAL_BLOCKL_MAX);
  WebRtc_rdft(self->anaLen, 1, self->dataBuf, self->ip, self->wfft);

  memset(self->analyzeBuf, 0, sizeof(float) * ANAL_BLOCKL_MAX);
  memset(self->dataBuf, 0, sizeof(float) * ANAL_BLOCKL_MAX);
  memset(self->syntBuf, 0, sizeof(float) * ANAL_BLOCKL_MAX);

  
  memset(self->dataBufHB, 0, sizeof(float) * ANAL_BLOCKL_MAX);

  
  memset(self->quantile, 0, sizeof(float) * HALF_ANAL_BLOCKL);
  for (i = 0; i < SIMULT * HALF_ANAL_BLOCKL; i++) {
    self->lquantile[i] = 8.f;
    self->density[i] = 0.3f;
  }

  for (i = 0; i < SIMULT; i++) {
    self->counter[i] =
        (int)floor((float)(END_STARTUP_LONG * (i + 1)) / (float)SIMULT);
  }

  self->updates = 0;

  
  for (i = 0; i < HALF_ANAL_BLOCKL; i++) {
    self->smooth[i] = 1.f;
  }

  
  self->aggrMode = 0;

  
  self->priorSpeechProb = 0.5f;  
  
  memset(self->magnPrevAnalyze, 0, sizeof(float) * HALF_ANAL_BLOCKL);
  
  memset(self->magnPrevProcess, 0, sizeof(float) * HALF_ANAL_BLOCKL);
  
  memset(self->noise, 0, sizeof(float) * HALF_ANAL_BLOCKL);
  
  memset(self->noisePrev, 0, sizeof(float) * HALF_ANAL_BLOCKL);
  
  memset(self->magnAvgPause, 0, sizeof(float) * HALF_ANAL_BLOCKL);
  
  memset(self->speechProb, 0, sizeof(float) * HALF_ANAL_BLOCKL);
  
  memset(self->initMagnEst, 0, sizeof(float) * HALF_ANAL_BLOCKL);
  for (i = 0; i < HALF_ANAL_BLOCKL; i++) {
    
    self->logLrtTimeAvg[i] = LRT_FEATURE_THR;
  }

  
  
  self->featureData[0] = SF_FEATURE_THR;
  self->featureData[1] = 0.f;  
  self->featureData[2] = 0.f;  
  
  self->featureData[3] = LRT_FEATURE_THR;
  
  self->featureData[4] = SF_FEATURE_THR;
  self->featureData[5] = 0.f;  
  
  self->featureData[6] = 0.f;

  
  memset(self->histLrt, 0, sizeof(int) * HIST_PAR_EST);
  memset(self->histSpecFlat, 0, sizeof(int) * HIST_PAR_EST);
  memset(self->histSpecDiff, 0, sizeof(int) * HIST_PAR_EST);


  self->blockInd = -1;  
  
  self->priorModelPars[0] = LRT_FEATURE_THR;
  
  self->priorModelPars[1] = 0.5f;
  
  self->priorModelPars[2] = 1.f;
  
  self->priorModelPars[3] = 0.5f;
  
  self->priorModelPars[4] = 1.f;
  
  self->priorModelPars[5] = 0.f;
  
  self->priorModelPars[6] = 0.f;

  
  
  self->modelUpdatePars[0] = 2;
  self->modelUpdatePars[1] = 500;  
  
  self->modelUpdatePars[2] = 0;
  
  self->modelUpdatePars[3] = self->modelUpdatePars[1];

  self->signalEnergy = 0.0;
  self->sumMagn = 0.0;
  self->whiteNoiseLevel = 0.0;
  self->pinkNoiseNumerator = 0.0;
  self->pinkNoiseExp = 0.0;

  set_feature_extraction_parameters(self);

  
  WebRtcNs_set_policy_core(self, 0);

  self->initFlag = 1;
  return 0;
}


static void NoiseEstimation(NSinst_t* self, float* magn, float* noise) {
  int i, s, offset;
  float lmagn[HALF_ANAL_BLOCKL], delta;

  if (self->updates < END_STARTUP_LONG) {
    self->updates++;
  }

  for (i = 0; i < self->magnLen; i++) {
    lmagn[i] = (float)log(magn[i]);
  }

  
  for (s = 0; s < SIMULT; s++) {
    offset = s * self->magnLen;

    
    for (i = 0; i < self->magnLen; i++) {
      
      if (self->density[offset + i] > 1.0) {
        delta = FACTOR * 1.f / self->density[offset + i];
      } else {
        delta = FACTOR;
      }

      
      if (lmagn[i] > self->lquantile[offset + i]) {
        self->lquantile[offset + i] +=
            QUANTILE * delta / (float)(self->counter[s] + 1);
      } else {
        self->lquantile[offset + i] -=
            (1.f - QUANTILE) * delta / (float)(self->counter[s] + 1);
      }

      
      if (fabs(lmagn[i] - self->lquantile[offset + i]) < WIDTH) {
        self->density[offset + i] =
            ((float)self->counter[s] * self->density[offset + i] +
             1.f / (2.f * WIDTH)) /
            (float)(self->counter[s] + 1);
      }
    }  

    if (self->counter[s] >= END_STARTUP_LONG) {
      self->counter[s] = 0;
      if (self->updates >= END_STARTUP_LONG) {
        for (i = 0; i < self->magnLen; i++) {
          self->quantile[i] = (float)exp(self->lquantile[offset + i]);
        }
      }
    }

    self->counter[s]++;
  }  

  
  if (self->updates < END_STARTUP_LONG) {
    
    for (i = 0; i < self->magnLen; i++) {
      self->quantile[i] = (float)exp(self->lquantile[offset + i]);
    }
  }

  for (i = 0; i < self->magnLen; i++) {
    noise[i] = self->quantile[i];
  }
}







static void FeatureParameterExtraction(NSinst_t* self, int flag) {
  int i, useFeatureSpecFlat, useFeatureSpecDiff, numHistLrt;
  int maxPeak1, maxPeak2;
  int weightPeak1SpecFlat, weightPeak2SpecFlat, weightPeak1SpecDiff,
      weightPeak2SpecDiff;

  float binMid, featureSum;
  float posPeak1SpecFlat, posPeak2SpecFlat, posPeak1SpecDiff, posPeak2SpecDiff;
  float fluctLrt, avgHistLrt, avgSquareHistLrt, avgHistLrtCompl;

  
  
  
  

  
  if (flag == 0) {
    
    if ((self->featureData[3] <
         HIST_PAR_EST * self->featureExtractionParams.binSizeLrt) &&
        (self->featureData[3] >= 0.0)) {
      i = (int)(self->featureData[3] /
                self->featureExtractionParams.binSizeLrt);
      self->histLrt[i]++;
    }
    
    if ((self->featureData[0] <
         HIST_PAR_EST * self->featureExtractionParams.binSizeSpecFlat) &&
        (self->featureData[0] >= 0.0)) {
      i = (int)(self->featureData[0] /
                self->featureExtractionParams.binSizeSpecFlat);
      self->histSpecFlat[i]++;
    }
    
    if ((self->featureData[4] <
         HIST_PAR_EST * self->featureExtractionParams.binSizeSpecDiff) &&
        (self->featureData[4] >= 0.0)) {
      i = (int)(self->featureData[4] /
                self->featureExtractionParams.binSizeSpecDiff);
      self->histSpecDiff[i]++;
    }
  }

  
  if (flag == 1) {
    
    
    avgHistLrt = 0.0;
    avgHistLrtCompl = 0.0;
    avgSquareHistLrt = 0.0;
    numHistLrt = 0;
    for (i = 0; i < HIST_PAR_EST; i++) {
      binMid = ((float)i + 0.5f) * self->featureExtractionParams.binSizeLrt;
      if (binMid <= self->featureExtractionParams.rangeAvgHistLrt) {
        avgHistLrt += self->histLrt[i] * binMid;
        numHistLrt += self->histLrt[i];
      }
      avgSquareHistLrt += self->histLrt[i] * binMid * binMid;
      avgHistLrtCompl += self->histLrt[i] * binMid;
    }
    if (numHistLrt > 0) {
      avgHistLrt = avgHistLrt / ((float)numHistLrt);
    }
    avgHistLrtCompl = avgHistLrtCompl / ((float)self->modelUpdatePars[1]);
    avgSquareHistLrt = avgSquareHistLrt / ((float)self->modelUpdatePars[1]);
    fluctLrt = avgSquareHistLrt - avgHistLrt * avgHistLrtCompl;
    
    if (fluctLrt < self->featureExtractionParams.thresFluctLrt) {
      
      self->priorModelPars[0] = self->featureExtractionParams.maxLrt;
    } else {
      self->priorModelPars[0] =
          self->featureExtractionParams.factor1ModelPars * avgHistLrt;
      
      if (self->priorModelPars[0] < self->featureExtractionParams.minLrt) {
        self->priorModelPars[0] = self->featureExtractionParams.minLrt;
      }
      if (self->priorModelPars[0] > self->featureExtractionParams.maxLrt) {
        self->priorModelPars[0] = self->featureExtractionParams.maxLrt;
      }
    }
    

    
    
    maxPeak1 = 0;
    maxPeak2 = 0;
    posPeak1SpecFlat = 0.0;
    posPeak2SpecFlat = 0.0;
    weightPeak1SpecFlat = 0;
    weightPeak2SpecFlat = 0;

    
    for (i = 0; i < HIST_PAR_EST; i++) {
      binMid =
          (i + 0.5f) * self->featureExtractionParams.binSizeSpecFlat;
      if (self->histSpecFlat[i] > maxPeak1) {
        
        maxPeak2 = maxPeak1;
        weightPeak2SpecFlat = weightPeak1SpecFlat;
        posPeak2SpecFlat = posPeak1SpecFlat;

        maxPeak1 = self->histSpecFlat[i];
        weightPeak1SpecFlat = self->histSpecFlat[i];
        posPeak1SpecFlat = binMid;
      } else if (self->histSpecFlat[i] > maxPeak2) {
        
        maxPeak2 = self->histSpecFlat[i];
        weightPeak2SpecFlat = self->histSpecFlat[i];
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
      binMid =
          ((float)i + 0.5f) * self->featureExtractionParams.binSizeSpecDiff;
      if (self->histSpecDiff[i] > maxPeak1) {
        
        maxPeak2 = maxPeak1;
        weightPeak2SpecDiff = weightPeak1SpecDiff;
        posPeak2SpecDiff = posPeak1SpecDiff;

        maxPeak1 = self->histSpecDiff[i];
        weightPeak1SpecDiff = self->histSpecDiff[i];
        posPeak1SpecDiff = binMid;
      } else if (self->histSpecDiff[i] > maxPeak2) {
        
        maxPeak2 = self->histSpecDiff[i];
        weightPeak2SpecDiff = self->histSpecDiff[i];
        posPeak2SpecDiff = binMid;
      }
    }

    
    useFeatureSpecFlat = 1;
    
    if ((fabs(posPeak2SpecFlat - posPeak1SpecFlat) <
         self->featureExtractionParams.limitPeakSpacingSpecFlat) &&
        (weightPeak2SpecFlat >
         self->featureExtractionParams.limitPeakWeightsSpecFlat *
             weightPeak1SpecFlat)) {
      weightPeak1SpecFlat += weightPeak2SpecFlat;
      posPeak1SpecFlat = 0.5f * (posPeak1SpecFlat + posPeak2SpecFlat);
    }
    
    if (weightPeak1SpecFlat <
            self->featureExtractionParams.thresWeightSpecFlat ||
        posPeak1SpecFlat < self->featureExtractionParams.thresPosSpecFlat) {
      useFeatureSpecFlat = 0;
    }
    
    if (useFeatureSpecFlat == 1) {
      
      self->priorModelPars[1] =
          self->featureExtractionParams.factor2ModelPars * posPeak1SpecFlat;
      
      if (self->priorModelPars[1] < self->featureExtractionParams.minSpecFlat) {
        self->priorModelPars[1] = self->featureExtractionParams.minSpecFlat;
      }
      if (self->priorModelPars[1] > self->featureExtractionParams.maxSpecFlat) {
        self->priorModelPars[1] = self->featureExtractionParams.maxSpecFlat;
      }
    }
    

    
    useFeatureSpecDiff = 1;
    
    if ((fabs(posPeak2SpecDiff - posPeak1SpecDiff) <
         self->featureExtractionParams.limitPeakSpacingSpecDiff) &&
        (weightPeak2SpecDiff >
         self->featureExtractionParams.limitPeakWeightsSpecDiff *
             weightPeak1SpecDiff)) {
      weightPeak1SpecDiff += weightPeak2SpecDiff;
      posPeak1SpecDiff = 0.5f * (posPeak1SpecDiff + posPeak2SpecDiff);
    }
    
    self->priorModelPars[3] =
        self->featureExtractionParams.factor1ModelPars * posPeak1SpecDiff;
    
    if (weightPeak1SpecDiff <
        self->featureExtractionParams.thresWeightSpecDiff) {
      useFeatureSpecDiff = 0;
    }
    
    if (self->priorModelPars[3] < self->featureExtractionParams.minSpecDiff) {
      self->priorModelPars[3] = self->featureExtractionParams.minSpecDiff;
    }
    if (self->priorModelPars[3] > self->featureExtractionParams.maxSpecDiff) {
      self->priorModelPars[3] = self->featureExtractionParams.maxSpecDiff;
    }
    

    
    
    if (fluctLrt < self->featureExtractionParams.thresFluctLrt) {
      useFeatureSpecDiff = 0;
    }

    
    
    
    
    featureSum = (float)(1 + useFeatureSpecFlat + useFeatureSpecDiff);
    self->priorModelPars[4] = 1.f / featureSum;
    self->priorModelPars[5] = ((float)useFeatureSpecFlat) / featureSum;
    self->priorModelPars[6] = ((float)useFeatureSpecDiff) / featureSum;

    
    if (self->modelUpdatePars[0] >= 1) {
      for (i = 0; i < HIST_PAR_EST; i++) {
        self->histLrt[i] = 0;
        self->histSpecFlat[i] = 0;
        self->histSpecDiff[i] = 0;
      }
    }
  }  
}




static void ComputeSpectralFlatness(NSinst_t* self, const float* magnIn) {
  int i;
  int shiftLP = 1;  
  float avgSpectralFlatnessNum, avgSpectralFlatnessDen, spectralTmp;

  
  
  avgSpectralFlatnessNum = 0.0;
  avgSpectralFlatnessDen = self->sumMagn;
  for (i = 0; i < shiftLP; i++) {
    avgSpectralFlatnessDen -= magnIn[i];
  }
  
  
  for (i = shiftLP; i < self->magnLen; i++) {
    if (magnIn[i] > 0.0) {
      avgSpectralFlatnessNum += (float)log(magnIn[i]);
    } else {
      self->featureData[0] -= SPECT_FL_TAVG * self->featureData[0];
      return;
    }
  }
  
  avgSpectralFlatnessDen = avgSpectralFlatnessDen / self->magnLen;
  avgSpectralFlatnessNum = avgSpectralFlatnessNum / self->magnLen;

  
  spectralTmp = (float)exp(avgSpectralFlatnessNum) / avgSpectralFlatnessDen;

  
  self->featureData[0] += SPECT_FL_TAVG * (spectralTmp - self->featureData[0]);
  
}









static void ComputeSnr(const NSinst_t* self,
                       const float* magn,
                       const float* noise,
                       float* snrLocPrior,
                       float* snrLocPost) {
  int i;

  for (i = 0; i < self->magnLen; i++) {
    
    
    float previousEstimateStsa = self->magnPrevAnalyze[i] /
        (self->noisePrev[i] + 0.0001f) * self->smooth[i];
    
    snrLocPost[i] = 0.f;
    if (magn[i] > noise[i]) {
      snrLocPost[i] = magn[i] / (noise[i] + 0.0001f) - 1.f;
    }
    
    
    snrLocPrior[i] =
        DD_PR_SNR * previousEstimateStsa + (1.f - DD_PR_SNR) * snrLocPost[i];
  }  
}






static void ComputeSpectralDifference(NSinst_t* self,
                                      const float* magnIn) {
  
  
  int i;
  float avgPause, avgMagn, covMagnPause, varPause, varMagn, avgDiffNormMagn;

  avgPause = 0.0;
  avgMagn = self->sumMagn;
  
  for (i = 0; i < self->magnLen; i++) {
    
    avgPause += self->magnAvgPause[i];
  }
  avgPause = avgPause / ((float)self->magnLen);
  avgMagn = avgMagn / ((float)self->magnLen);

  covMagnPause = 0.0;
  varPause = 0.0;
  varMagn = 0.0;
  
  for (i = 0; i < self->magnLen; i++) {
    covMagnPause += (magnIn[i] - avgMagn) * (self->magnAvgPause[i] - avgPause);
    varPause +=
        (self->magnAvgPause[i] - avgPause) * (self->magnAvgPause[i] - avgPause);
    varMagn += (magnIn[i] - avgMagn) * (magnIn[i] - avgMagn);
  }
  covMagnPause = covMagnPause / ((float)self->magnLen);
  varPause = varPause / ((float)self->magnLen);
  varMagn = varMagn / ((float)self->magnLen);
  
  self->featureData[6] += self->signalEnergy;

  avgDiffNormMagn =
      varMagn - (covMagnPause * covMagnPause) / (varPause + 0.0001f);
  
  avgDiffNormMagn = (float)(avgDiffNormMagn / (self->featureData[5] + 0.0001f));
  self->featureData[4] +=
      SPECT_DIFF_TAVG * (avgDiffNormMagn - self->featureData[4]);
}







static void SpeechNoiseProb(NSinst_t* self,
                            float* probSpeechFinal,
                            const float* snrLocPrior,
                            const float* snrLocPost) {
  int i, sgnMap;
  float invLrt, gainPrior, indPrior;
  float logLrtTimeAvgKsum, besselTmp;
  float indicator0, indicator1, indicator2;
  float tmpFloat1, tmpFloat2;
  float weightIndPrior0, weightIndPrior1, weightIndPrior2;
  float threshPrior0, threshPrior1, threshPrior2;
  float widthPrior, widthPrior0, widthPrior1, widthPrior2;

  widthPrior0 = WIDTH_PR_MAP;
  
  widthPrior1 = 2.f * WIDTH_PR_MAP;
  widthPrior2 = 2.f * WIDTH_PR_MAP;  

  
  threshPrior0 = self->priorModelPars[0];
  threshPrior1 = self->priorModelPars[1];
  threshPrior2 = self->priorModelPars[3];

  
  sgnMap = (int)(self->priorModelPars[2]);

  
  weightIndPrior0 = self->priorModelPars[4];
  weightIndPrior1 = self->priorModelPars[5];
  weightIndPrior2 = self->priorModelPars[6];

  
  
  logLrtTimeAvgKsum = 0.0;
  for (i = 0; i < self->magnLen; i++) {
    tmpFloat1 = 1.f + 2.f * snrLocPrior[i];
    tmpFloat2 = 2.f * snrLocPrior[i] / (tmpFloat1 + 0.0001f);
    besselTmp = (snrLocPost[i] + 1.f) * tmpFloat2;
    self->logLrtTimeAvg[i] +=
        LRT_TAVG * (besselTmp - (float)log(tmpFloat1) - self->logLrtTimeAvg[i]);
    logLrtTimeAvgKsum += self->logLrtTimeAvg[i];
  }
  logLrtTimeAvgKsum = (float)logLrtTimeAvgKsum / (self->magnLen);
  self->featureData[3] = logLrtTimeAvgKsum;
  

  
  
  widthPrior = widthPrior0;
  
  if (logLrtTimeAvgKsum < threshPrior0) {
    widthPrior = widthPrior1;
  }
  
  indicator0 =
      0.5f *
      ((float)tanh(widthPrior * (logLrtTimeAvgKsum - threshPrior0)) + 1.f);

  
  tmpFloat1 = self->featureData[0];
  widthPrior = widthPrior0;
  
  if (sgnMap == 1 && (tmpFloat1 > threshPrior1)) {
    widthPrior = widthPrior1;
  }
  if (sgnMap == -1 && (tmpFloat1 < threshPrior1)) {
    widthPrior = widthPrior1;
  }
  
  indicator1 =
      0.5f *
      ((float)tanh((float)sgnMap * widthPrior * (threshPrior1 - tmpFloat1)) +
       1.f);

  
  tmpFloat1 = self->featureData[4];
  widthPrior = widthPrior0;
  
  if (tmpFloat1 < threshPrior2) {
    widthPrior = widthPrior2;
  }
  
  indicator2 =
      0.5f * ((float)tanh(widthPrior * (tmpFloat1 - threshPrior2)) + 1.f);

  
  indPrior = weightIndPrior0 * indicator0 + weightIndPrior1 * indicator1 +
             weightIndPrior2 * indicator2;
  

  
  self->priorSpeechProb += PRIOR_UPDATE * (indPrior - self->priorSpeechProb);
  
  if (self->priorSpeechProb > 1.f) {
    self->priorSpeechProb = 1.f;
  }
  if (self->priorSpeechProb < 0.01f) {
    self->priorSpeechProb = 0.01f;
  }

  
  gainPrior = (1.f - self->priorSpeechProb) / (self->priorSpeechProb + 0.0001f);
  for (i = 0; i < self->magnLen; i++) {
    invLrt = (float)exp(-self->logLrtTimeAvg[i]);
    invLrt = (float)gainPrior * invLrt;
    probSpeechFinal[i] = 1.f / (1.f + invLrt);
  }
}





static void FeatureUpdate(NSinst_t* self,
                          const float* magn,
                          int updateParsFlag) {
  
  ComputeSpectralFlatness(self, magn);
  
  ComputeSpectralDifference(self, magn);
  
  
  
  
  if (updateParsFlag >= 1) {
    
    self->modelUpdatePars[3]--;
    
    if (self->modelUpdatePars[3] > 0) {
      FeatureParameterExtraction(self, 0);
    }
    
    if (self->modelUpdatePars[3] == 0) {
      FeatureParameterExtraction(self, 1);
      self->modelUpdatePars[3] = self->modelUpdatePars[1];
      
      if (updateParsFlag == 1) {
        self->modelUpdatePars[0] = 0;
      } else {
        
        
        self->featureData[6] =
            self->featureData[6] / ((float)self->modelUpdatePars[1]);
        self->featureData[5] =
            0.5f * (self->featureData[6] + self->featureData[5]);
        self->featureData[6] = 0.f;
      }
    }
  }
}








static void UpdateNoiseEstimate(NSinst_t* self,
                                const float* magn,
                                const float* snrLocPrior,
                                const float* snrLocPost,
                                float* noise) {
  int i;
  float probSpeech, probNonSpeech;
  
  float gammaNoiseTmp = NOISE_UPDATE;
  float gammaNoiseOld;
  float noiseUpdateTmp;

  for (i = 0; i < self->magnLen; i++) {
    probSpeech = self->speechProb[i];
    probNonSpeech = 1.f - probSpeech;
    
    
    noiseUpdateTmp = gammaNoiseTmp * self->noisePrev[i] +
                     (1.f - gammaNoiseTmp) * (probNonSpeech * magn[i] +
                                              probSpeech * self->noisePrev[i]);
    
    gammaNoiseOld = gammaNoiseTmp;
    gammaNoiseTmp = NOISE_UPDATE;
    
    if (probSpeech > PROB_RANGE) {
      gammaNoiseTmp = SPEECH_UPDATE;
    }
    
    if (probSpeech < PROB_RANGE) {
      self->magnAvgPause[i] += GAMMA_PAUSE * (magn[i] - self->magnAvgPause[i]);
    }
    
    if (gammaNoiseTmp == gammaNoiseOld) {
      noise[i] = noiseUpdateTmp;
    } else {
      noise[i] = gammaNoiseTmp * self->noisePrev[i] +
                 (1.f - gammaNoiseTmp) * (probNonSpeech * magn[i] +
                                          probSpeech * self->noisePrev[i]);
      
      
      
      if (noiseUpdateTmp < noise[i]) {
        noise[i] = noiseUpdateTmp;
      }
    }
  }  
}








static void UpdateBuffer(const float* frame,
                         int frame_length,
                         int buffer_length,
                         float* buffer) {
  assert(buffer_length < 2 * frame_length);

  memcpy(buffer,
         buffer + frame_length,
         sizeof(*buffer) * (buffer_length - frame_length));
  if (frame) {
    memcpy(buffer + buffer_length - frame_length,
           frame,
           sizeof(*buffer) * frame_length);
  } else {
    memset(buffer + buffer_length - frame_length,
           0,
           sizeof(*buffer) * frame_length);
  }
}












static void FFT(NSinst_t* self,
                float* time_data,
                int time_data_length,
                int magnitude_length,
                float* real,
                float* imag,
                float* magn) {
  int i;

  assert(magnitude_length == time_data_length / 2 + 1);

  WebRtc_rdft(time_data_length, 1, time_data, self->ip, self->wfft);

  imag[0] = 0;
  real[0] = time_data[0];
  magn[0] = fabs(real[0]) + 1.f;
  imag[magnitude_length - 1] = 0;
  real[magnitude_length - 1] = time_data[1];
  magn[magnitude_length - 1] = fabs(real[magnitude_length - 1]) + 1.f;
  for (i = 1; i < magnitude_length - 1; ++i) {
    real[i] = time_data[2 * i];
    imag[i] = time_data[2 * i + 1];
    
    magn[i] = sqrtf(real[i] * real[i] + imag[i] * imag[i]) + 1.f;
  }
}











static void IFFT(NSinst_t* self,
                 const float* real,
                 const float* imag,
                 int magnitude_length,
                 int time_data_length,
                 float* time_data) {
  int i;

  assert(time_data_length == 2 * (magnitude_length - 1));

  time_data[0] = real[0];
  time_data[1] = real[magnitude_length - 1];
  for (i = 1; i < magnitude_length - 1; ++i) {
    time_data[2 * i] = real[i];
    time_data[2 * i + 1] = imag[i];
  }
  WebRtc_rdft(time_data_length, -1, time_data, self->ip, self->wfft);

  for (i = 0; i < time_data_length; ++i) {
    time_data[i] *= 2.f / time_data_length;  
  }
}






static float Energy(const float* buffer, int length) {
  int i;
  float energy = 0.f;

  for (i = 0; i < length; ++i) {
    energy += buffer[i] * buffer[i];
  }

  return energy;
}








static void Windowing(const float* window,
                      const float* data,
                      int length,
                      float* data_windowed) {
  int i;

  for (i = 0; i < length; ++i) {
    data_windowed[i] = window[i] * data[i];
  }
}






static void ComputeDdBasedWienerFilter(const NSinst_t* self,
                                       const float* magn,
                                       float* theFilter) {
  int i;
  float snrPrior, previousEstimateStsa, currentEstimateStsa;

  for (i = 0; i < self->magnLen; i++) {
    
    previousEstimateStsa = self->magnPrevProcess[i] /
                           (self->noisePrev[i] + 0.0001f) * self->smooth[i];
    
    currentEstimateStsa = 0.f;
    if (magn[i] > self->noise[i]) {
      currentEstimateStsa = magn[i] / (self->noise[i] + 0.0001f) - 1.f;
    }
    
    
    snrPrior = DD_PR_SNR * previousEstimateStsa +
               (1.f - DD_PR_SNR) * currentEstimateStsa;
    
    theFilter[i] = snrPrior / (self->overdrive + snrPrior);
  }  
}





int WebRtcNs_set_policy_core(NSinst_t* self, int mode) {
  
  if (mode < 0 || mode > 3) {
    return (-1);
  }

  self->aggrMode = mode;
  if (mode == 0) {
    self->overdrive = 1.f;
    self->denoiseBound = 0.5f;
    self->gainmap = 0;
  } else if (mode == 1) {
    
    self->overdrive = 1.f;
    self->denoiseBound = 0.25f;
    self->gainmap = 1;
  } else if (mode == 2) {
    
    self->overdrive = 1.1f;
    self->denoiseBound = 0.125f;
    self->gainmap = 1;
  } else if (mode == 3) {
    
    self->overdrive = 1.25f;
    self->denoiseBound = 0.09f;
    self->gainmap = 1;
  }
  return 0;
}

int WebRtcNs_AnalyzeCore(NSinst_t* self, float* speechFrame) {
  int i;
  const int kStartBand = 5;  
  int updateParsFlag;
  float energy;
  float signalEnergy = 0.f;
  float sumMagn = 0.f;
  float tmpFloat1, tmpFloat2, tmpFloat3;
  float winData[ANAL_BLOCKL_MAX];
  float magn[HALF_ANAL_BLOCKL], noise[HALF_ANAL_BLOCKL];
  float snrLocPost[HALF_ANAL_BLOCKL], snrLocPrior[HALF_ANAL_BLOCKL];
  float real[ANAL_BLOCKL_MAX], imag[HALF_ANAL_BLOCKL];
  
  float sum_log_i = 0.0;
  float sum_log_i_square = 0.0;
  float sum_log_magn = 0.0;
  float sum_log_i_log_magn = 0.0;
  float parametric_exp = 0.0;
  float parametric_num = 0.0;

  
  if (self->initFlag != 1) {
    return (-1);
  }
  updateParsFlag = self->modelUpdatePars[0];

  
  UpdateBuffer(speechFrame, self->blockLen, self->anaLen, self->analyzeBuf);

  Windowing(self->window, self->analyzeBuf, self->anaLen, winData);
  energy = Energy(winData, self->anaLen);
  if (energy == 0.0) {
    
    
    
    
    
    
    
    
    return 0;
  }

  self->blockInd++;  

  FFT(self, winData, self->anaLen, self->magnLen, real, imag, magn);

  for (i = 0; i < self->magnLen; i++) {
    signalEnergy += real[i] * real[i] + imag[i] * imag[i];
    sumMagn += magn[i];
    if (self->blockInd < END_STARTUP_SHORT) {
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
  signalEnergy = signalEnergy / ((float)self->magnLen);
  self->signalEnergy = signalEnergy;
  self->sumMagn = sumMagn;

  
  NoiseEstimation(self, magn, noise);
  
  if (self->blockInd < END_STARTUP_SHORT) {
    
    self->whiteNoiseLevel += sumMagn / ((float)self->magnLen) * self->overdrive;
    
    tmpFloat1 = sum_log_i_square * ((float)(self->magnLen - kStartBand));
    tmpFloat1 -= (sum_log_i * sum_log_i);
    tmpFloat2 =
        (sum_log_i_square * sum_log_magn - sum_log_i * sum_log_i_log_magn);
    tmpFloat3 = tmpFloat2 / tmpFloat1;
    
    if (tmpFloat3 < 0.f) {
      tmpFloat3 = 0.f;
    }
    self->pinkNoiseNumerator += tmpFloat3;
    tmpFloat2 = (sum_log_i * sum_log_magn);
    tmpFloat2 -= ((float)(self->magnLen - kStartBand)) * sum_log_i_log_magn;
    tmpFloat3 = tmpFloat2 / tmpFloat1;
    
    if (tmpFloat3 < 0.f) {
      tmpFloat3 = 0.f;
    }
    if (tmpFloat3 > 1.f) {
      tmpFloat3 = 1.f;
    }
    self->pinkNoiseExp += tmpFloat3;

    
    if (self->pinkNoiseExp > 0.f) {
      
      parametric_num =
          exp(self->pinkNoiseNumerator / (float)(self->blockInd + 1));
      parametric_num *= (float)(self->blockInd + 1);
      parametric_exp = self->pinkNoiseExp / (float)(self->blockInd + 1);
    }
    for (i = 0; i < self->magnLen; i++) {
      
      
      if (self->pinkNoiseExp == 0.f) {
        
        self->parametricNoise[i] = self->whiteNoiseLevel;
      } else {
        
        float use_band = (float)(i < kStartBand ? kStartBand : i);
        self->parametricNoise[i] =
            parametric_num / pow(use_band, parametric_exp);
      }
      
      noise[i] *= (self->blockInd);
      tmpFloat2 =
          self->parametricNoise[i] * (END_STARTUP_SHORT - self->blockInd);
      noise[i] += (tmpFloat2 / (float)(self->blockInd + 1));
      noise[i] /= END_STARTUP_SHORT;
    }
  }
  
  
  if (self->blockInd < END_STARTUP_LONG) {
    self->featureData[5] *= self->blockInd;
    self->featureData[5] += signalEnergy;
    self->featureData[5] /= (self->blockInd + 1);
  }

  
  ComputeSnr(self, magn, noise, snrLocPrior, snrLocPost);

  FeatureUpdate(self, magn, updateParsFlag);
  SpeechNoiseProb(self, self->speechProb, snrLocPrior, snrLocPost);
  UpdateNoiseEstimate(self, magn, snrLocPrior, snrLocPost, noise);

  
  memcpy(self->noise, noise, sizeof(*noise) * self->magnLen);
  memcpy(self->magnPrevAnalyze, magn, sizeof(*magn) * self->magnLen);

  return 0;
}

int WebRtcNs_ProcessCore(NSinst_t* self,
                         float* speechFrame,
                         float* speechFrameHB,
                         float* outFrame,
                         float* outFrameHB) {
  
  int flagHB = 0;
  int i;

  float energy1, energy2, gain, factor, factor1, factor2;
  float fout[BLOCKL_MAX];
  float winData[ANAL_BLOCKL_MAX];
  float magn[HALF_ANAL_BLOCKL];
  float theFilter[HALF_ANAL_BLOCKL], theFilterTmp[HALF_ANAL_BLOCKL];
  float real[ANAL_BLOCKL_MAX], imag[HALF_ANAL_BLOCKL];

  
  int deltaBweHB = 1;
  int deltaGainHB = 1;
  float decayBweHB = 1.0;
  float gainMapParHB = 1.0;
  float gainTimeDomainHB = 1.0;
  float avgProbSpeechHB, avgProbSpeechHBTmp, avgFilterGainHB, gainModHB;
  float sumMagnAnalyze, sumMagnProcess;

  
  if (self->initFlag != 1) {
    return (-1);
  }
  
  if (self->fs == 32000) {
    if (speechFrameHB == NULL) {
      return -1;
    }
    flagHB = 1;
    
    deltaBweHB = (int)self->magnLen / 4;
    deltaGainHB = deltaBweHB;
  }

  
  UpdateBuffer(speechFrame, self->blockLen, self->anaLen, self->dataBuf);

  if (flagHB == 1) {
    
    UpdateBuffer(speechFrameHB, self->blockLen, self->anaLen, self->dataBufHB);
  }

  Windowing(self->window, self->dataBuf, self->anaLen, winData);
  energy1 = Energy(winData, self->anaLen);
  if (energy1 == 0.0) {
    
    
    for (i = self->windShift; i < self->blockLen + self->windShift; i++) {
      fout[i - self->windShift] = self->syntBuf[i];
    }
    
    UpdateBuffer(NULL, self->blockLen, self->anaLen, self->syntBuf);

    for (i = 0; i < self->blockLen; ++i)
      outFrame[i] =
          WEBRTC_SPL_SAT(WEBRTC_SPL_WORD16_MAX, fout[i], WEBRTC_SPL_WORD16_MIN);

    
    if (flagHB == 1)
      for (i = 0; i < self->blockLen; ++i)
        outFrameHB[i] = WEBRTC_SPL_SAT(
            WEBRTC_SPL_WORD16_MAX, self->dataBufHB[i], WEBRTC_SPL_WORD16_MIN);

    return 0;
  }

  FFT(self, winData, self->anaLen, self->magnLen, real, imag, magn);

  if (self->blockInd < END_STARTUP_SHORT) {
    for (i = 0; i < self->magnLen; i++) {
      self->initMagnEst[i] += magn[i];
    }
  }

  ComputeDdBasedWienerFilter(self, magn, theFilter);

  for (i = 0; i < self->magnLen; i++) {
    
    if (theFilter[i] < self->denoiseBound) {
      theFilter[i] = self->denoiseBound;
    }
    
    if (theFilter[i] > 1.f) {
      theFilter[i] = 1.f;
    }
    if (self->blockInd < END_STARTUP_SHORT) {
      theFilterTmp[i] =
          (self->initMagnEst[i] - self->overdrive * self->parametricNoise[i]);
      theFilterTmp[i] /= (self->initMagnEst[i] + 0.0001f);
      
      if (theFilterTmp[i] < self->denoiseBound) {
        theFilterTmp[i] = self->denoiseBound;
      }
      
      if (theFilterTmp[i] > 1.f) {
        theFilterTmp[i] = 1.f;
      }
      
      theFilter[i] *= (self->blockInd);
      theFilterTmp[i] *= (END_STARTUP_SHORT - self->blockInd);
      theFilter[i] += theFilterTmp[i];
      theFilter[i] /= (END_STARTUP_SHORT);
    }

    self->smooth[i] = theFilter[i];
    real[i] *= self->smooth[i];
    imag[i] *= self->smooth[i];
  }
  
  memcpy(self->magnPrevProcess, magn, sizeof(*magn) * self->magnLen);
  memcpy(self->noisePrev, self->noise, sizeof(self->noise[0]) * self->magnLen);
  
  IFFT(self, real, imag, self->magnLen, self->anaLen, winData);

  
  factor = 1.f;
  if (self->gainmap == 1 && self->blockInd > END_STARTUP_LONG) {
    factor1 = 1.f;
    factor2 = 1.f;

    energy2 = Energy(winData, self->anaLen);
    gain = (float)sqrt(energy2 / (energy1 + 1.f));

    
    if (gain > B_LIM) {
      factor1 = 1.f + 1.3f * (gain - B_LIM);
      if (gain * factor1 > 1.f) {
        factor1 = 1.f / gain;
      }
    }
    if (gain < B_LIM) {
      
      
      if (gain <= self->denoiseBound) {
        gain = self->denoiseBound;
      }
      factor2 = 1.f - 0.3f * (B_LIM - gain);
    }
    
    
    factor = self->priorSpeechProb * factor1 +
             (1.f - self->priorSpeechProb) * factor2;
  }  

  Windowing(self->window, winData, self->anaLen, winData);

  
  for (i = 0; i < self->anaLen; i++) {
    self->syntBuf[i] += factor * winData[i];
  }
  
  for (i = self->windShift; i < self->blockLen + self->windShift; i++) {
    fout[i - self->windShift] = self->syntBuf[i];
  }
  
  UpdateBuffer(NULL, self->blockLen, self->anaLen, self->syntBuf);

  for (i = 0; i < self->blockLen; ++i)
    outFrame[i] =
        WEBRTC_SPL_SAT(WEBRTC_SPL_WORD16_MAX, fout[i], WEBRTC_SPL_WORD16_MIN);

  
  if (flagHB == 1) {
    
    
    avgProbSpeechHB = 0.0;
    for (i = self->magnLen - deltaBweHB - 1; i < self->magnLen - 1; i++) {
      avgProbSpeechHB += self->speechProb[i];
    }
    avgProbSpeechHB = avgProbSpeechHB / ((float)deltaBweHB);
    
    
    
    sumMagnAnalyze = 0;
    sumMagnProcess = 0;
    for (i = 0; i < self->magnLen; ++i) {
      sumMagnAnalyze += self->magnPrevAnalyze[i];
      sumMagnProcess += self->magnPrevProcess[i];
    }
    avgProbSpeechHB *= sumMagnProcess / sumMagnAnalyze;
    
    
    avgFilterGainHB = 0.0;
    for (i = self->magnLen - deltaGainHB - 1; i < self->magnLen - 1; i++) {
      avgFilterGainHB += self->smooth[i];
    }
    avgFilterGainHB = avgFilterGainHB / ((float)(deltaGainHB));
    avgProbSpeechHBTmp = 2.f * avgProbSpeechHB - 1.f;
    
    gainModHB = 0.5f * (1.f + (float)tanh(gainMapParHB * avgProbSpeechHBTmp));
    
    gainTimeDomainHB = 0.5f * gainModHB + 0.5f * avgFilterGainHB;
    if (avgProbSpeechHB >= 0.5f) {
      gainTimeDomainHB = 0.25f * gainModHB + 0.75f * avgFilterGainHB;
    }
    gainTimeDomainHB = gainTimeDomainHB * decayBweHB;
    
    
    if (gainTimeDomainHB < self->denoiseBound) {
      gainTimeDomainHB = self->denoiseBound;
    }
    
    if (gainTimeDomainHB > 1.f) {
      gainTimeDomainHB = 1.f;
    }
    
    for (i = 0; i < self->blockLen; i++) {
      float o = gainTimeDomainHB * self->dataBufHB[i];
      outFrameHB[i] =
          WEBRTC_SPL_SAT(WEBRTC_SPL_WORD16_MAX, o, WEBRTC_SPL_WORD16_MIN);
    }
  }  

  return 0;
}
