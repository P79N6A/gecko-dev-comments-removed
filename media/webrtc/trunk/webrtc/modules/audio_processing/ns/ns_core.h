









#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_NS_NS_CORE_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_NS_NS_CORE_H_

#include "webrtc/modules/audio_processing/ns/defines.h"

typedef struct NSParaExtract_t_ {
  
  float binSizeLrt;
  float binSizeSpecFlat;
  float binSizeSpecDiff;
  
  float rangeAvgHistLrt;
  
  
  float factor1ModelPars;  
  float factor2ModelPars;  
                           
  
  float thresPosSpecFlat;
  
  
  float limitPeakSpacingSpecFlat;
  float limitPeakSpacingSpecDiff;
  
  float limitPeakWeightsSpecFlat;
  float limitPeakWeightsSpecDiff;
  
  float thresFluctLrt;
  
  float maxLrt;
  float minLrt;
  float maxSpecFlat;
  float minSpecFlat;
  float maxSpecDiff;
  float minSpecDiff;
  
  int thresWeightSpecFlat;
  int thresWeightSpecDiff;

} NSParaExtract_t;

typedef struct NSinst_t_ {
  uint32_t fs;
  int blockLen;
  int windShift;
  int anaLen;
  int magnLen;
  int aggrMode;
  const float* window;
  float analyzeBuf[ANAL_BLOCKL_MAX];
  float dataBuf[ANAL_BLOCKL_MAX];
  float syntBuf[ANAL_BLOCKL_MAX];

  int initFlag;
  
  float density[SIMULT * HALF_ANAL_BLOCKL];
  float lquantile[SIMULT * HALF_ANAL_BLOCKL];
  float quantile[HALF_ANAL_BLOCKL];
  int counter[SIMULT];
  int updates;
  
  float smooth[HALF_ANAL_BLOCKL];
  float overdrive;
  float denoiseBound;
  int gainmap;
  
  int ip[IP_LENGTH];
  float wfft[W_LENGTH];

  
  int32_t blockInd;  
  int modelUpdatePars[4];  
  
  float priorModelPars[7];  
  float noise[HALF_ANAL_BLOCKL];  
  float noisePrev[HALF_ANAL_BLOCKL];  
  
  float magnPrevAnalyze[HALF_ANAL_BLOCKL];
  
  float magnPrevProcess[HALF_ANAL_BLOCKL];
  float logLrtTimeAvg[HALF_ANAL_BLOCKL];  
  float priorSpeechProb;  
  float featureData[7];
  
  float magnAvgPause[HALF_ANAL_BLOCKL];
  float signalEnergy;  
  float sumMagn;
  float whiteNoiseLevel;  
  float initMagnEst[HALF_ANAL_BLOCKL];  
  float pinkNoiseNumerator;  
  float pinkNoiseExp;  
  float parametricNoise[HALF_ANAL_BLOCKL];
  
  NSParaExtract_t featureExtractionParams;
  
  int histLrt[HIST_PAR_EST];
  int histSpecFlat[HIST_PAR_EST];
  int histSpecDiff[HIST_PAR_EST];
  
  float speechProb[HALF_ANAL_BLOCKL];  
  float dataBufHB[ANAL_BLOCKL_MAX];  

} NSinst_t;

#ifdef __cplusplus
extern "C" {
#endif
















int WebRtcNs_InitCore(NSinst_t* self, uint32_t fs);
















int WebRtcNs_set_policy_core(NSinst_t* self, int mode);
















int WebRtcNs_AnalyzeCore(NSinst_t* self, float* speechFrame);



















int WebRtcNs_ProcessCore(NSinst_t* self,
                         float* inFrameLow,
                         float* inFrameHigh,
                         float* outFrameLow,
                         float* outFrameHigh);

#ifdef __cplusplus
}
#endif
#endif
