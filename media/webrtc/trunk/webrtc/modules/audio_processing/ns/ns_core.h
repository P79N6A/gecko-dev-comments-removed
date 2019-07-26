









#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_NS_MAIN_SOURCE_NS_CORE_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_NS_MAIN_SOURCE_NS_CORE_H_

#include "defines.h"

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

  WebRtc_UWord32  fs;
  int             blockLen;
  int             blockLen10ms;
  int             windShift;
  int             outLen;
  int             anaLen;
  int             magnLen;
  int             aggrMode;
  const float*    window;
  float           dataBuf[ANAL_BLOCKL_MAX];
  float           syntBuf[ANAL_BLOCKL_MAX];
  float           outBuf[3 * BLOCKL_MAX];

  int             initFlag;
  
  float           density[SIMULT* HALF_ANAL_BLOCKL];
  float           lquantile[SIMULT* HALF_ANAL_BLOCKL];
  float           quantile[HALF_ANAL_BLOCKL];
  int             counter[SIMULT];
  int             updates;
  
  float           smooth[HALF_ANAL_BLOCKL];
  float           overdrive;
  float           denoiseBound;
  int             gainmap;
  
  int             ip[IP_LENGTH];
  float           wfft[W_LENGTH];

  
  WebRtc_Word32   blockInd;                           
  int             modelUpdatePars[4];                 
  
  float           priorModelPars[7];                  
  float           noisePrev[HALF_ANAL_BLOCKL];        
  float           magnPrev[HALF_ANAL_BLOCKL];         
  float           logLrtTimeAvg[HALF_ANAL_BLOCKL];    
  float           priorSpeechProb;                    
  float           featureData[7];                     
  float           magnAvgPause[HALF_ANAL_BLOCKL];     
  float           signalEnergy;                       
  float           sumMagn;                            
  float           whiteNoiseLevel;                    
  float           initMagnEst[HALF_ANAL_BLOCKL];      
  float           pinkNoiseNumerator;                 
  float           pinkNoiseExp;                       
  NSParaExtract_t featureExtractionParams;            
  
  int             histLrt[HIST_PAR_EST];
  int             histSpecFlat[HIST_PAR_EST];
  int             histSpecDiff[HIST_PAR_EST];
  
  float           speechProbHB[HALF_ANAL_BLOCKL];     
  float           dataBufHB[ANAL_BLOCKL_MAX];         

} NSinst_t;


#ifdef __cplusplus
extern "C" {
#endif
















int WebRtcNs_InitCore(NSinst_t* inst, WebRtc_UWord32 fs);
















int WebRtcNs_set_policy_core(NSinst_t* inst, int mode);





















int WebRtcNs_ProcessCore(NSinst_t* inst,
                         short* inFrameLow,
                         short* inFrameHigh,
                         short* outFrameLow,
                         short* outFrameHigh);


#ifdef __cplusplus
}
#endif
#endif
