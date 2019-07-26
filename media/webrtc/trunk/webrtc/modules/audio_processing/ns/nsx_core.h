









#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_NS_MAIN_SOURCE_NSX_CORE_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_NS_MAIN_SOURCE_NSX_CORE_H_

#ifdef NS_FILEDEBUG
#include <stdio.h>
#endif

#include "webrtc/common_audio/signal_processing/include/signal_processing_library.h"
#include "webrtc/modules/audio_processing/ns/nsx_defines.h"
#include "webrtc/typedefs.h"

typedef struct NsxInst_t_ {
  uint32_t                fs;

  const int16_t*          window;
  int16_t                 analysisBuffer[ANAL_BLOCKL_MAX];
  int16_t                 synthesisBuffer[ANAL_BLOCKL_MAX];
  uint16_t                noiseSupFilter[HALF_ANAL_BLOCKL];
  uint16_t                overdrive; 
  uint16_t                denoiseBound; 
  const int16_t*          factor2Table;
  int16_t                 noiseEstLogQuantile[SIMULT* HALF_ANAL_BLOCKL];
  int16_t                 noiseEstDensity[SIMULT* HALF_ANAL_BLOCKL];
  int16_t                 noiseEstCounter[SIMULT];
  int16_t                 noiseEstQuantile[HALF_ANAL_BLOCKL];

  int                     anaLen;
  int                     anaLen2;
  int                     magnLen;
  int                     aggrMode;
  int                     stages;
  int                     initFlag;
  int                     gainMap;

  int32_t                 maxLrt;
  int32_t                 minLrt;
  
  int32_t                 logLrtTimeAvgW32[HALF_ANAL_BLOCKL];
  int32_t                 featureLogLrt;
  int32_t                 thresholdLogLrt;
  int16_t                 weightLogLrt;

  uint32_t                featureSpecDiff;
  uint32_t                thresholdSpecDiff;
  int16_t                 weightSpecDiff;

  uint32_t                featureSpecFlat;
  uint32_t                thresholdSpecFlat;
  int16_t                 weightSpecFlat;

  
  int32_t                 avgMagnPause[HALF_ANAL_BLOCKL];
  uint32_t                magnEnergy;
  uint32_t                sumMagn;
  uint32_t                curAvgMagnEnergy;
  uint32_t                timeAvgMagnEnergy;
  uint32_t                timeAvgMagnEnergyTmp;

  uint32_t                whiteNoiseLevel;  
  
  uint32_t                initMagnEst[HALF_ANAL_BLOCKL];
  
  int32_t                 pinkNoiseNumerator;  
  int32_t                 pinkNoiseExp;  
  int                     minNorm;  
  int                     zeroInputSignal;  

  
  uint32_t                prevNoiseU32[HALF_ANAL_BLOCKL];
  
  uint16_t                prevMagnU16[HALF_ANAL_BLOCKL];
  
  int16_t                 priorNonSpeechProb;

  int                     blockIndex;  
  
  int                     modelUpdate;
  int                     cntThresUpdate;

  
  int16_t                 histLrt[HIST_PAR_EST];
  int16_t                 histSpecFlat[HIST_PAR_EST];
  int16_t                 histSpecDiff[HIST_PAR_EST];

  
  int16_t                 dataBufHBFX[ANAL_BLOCKL_MAX];  

  int                     qNoise;
  int                     prevQNoise;
  int                     prevQMagn;
  int                     blockLen10ms;

  int16_t                 real[ANAL_BLOCKL_MAX];
  int16_t                 imag[ANAL_BLOCKL_MAX];
  int32_t                 energyIn;
  int                     scaleEnergyIn;
  int                     normData;

  struct RealFFT* real_fft;
} NsxInst_t;

#ifdef __cplusplus
extern "C"
{
#endif
















int32_t WebRtcNsx_InitCore(NsxInst_t* inst, uint32_t fs);
















int WebRtcNsx_set_policy_core(NsxInst_t* inst, int mode);



















int WebRtcNsx_ProcessCore(NsxInst_t* inst,
                          short* inFrameLow,
                          short* inFrameHigh,
                          short* outFrameLow,
                          short* outFrameHigh);






typedef void (*NoiseEstimation)(NsxInst_t* inst,
                                uint16_t* magn,
                                uint32_t* noise,
                                int16_t* q_noise);
extern NoiseEstimation WebRtcNsx_NoiseEstimation;


typedef void (*PrepareSpectrum)(NsxInst_t* inst,
                                int16_t* freq_buff);
extern PrepareSpectrum WebRtcNsx_PrepareSpectrum;



typedef void (*SynthesisUpdate)(NsxInst_t* inst,
                                int16_t* out_frame,
                                int16_t gain_factor);
extern SynthesisUpdate WebRtcNsx_SynthesisUpdate;


typedef void (*AnalysisUpdate)(NsxInst_t* inst,
                               int16_t* out,
                               int16_t* new_speech);
extern AnalysisUpdate WebRtcNsx_AnalysisUpdate;

#if (defined WEBRTC_DETECT_ARM_NEON) || defined (WEBRTC_ARCH_ARM_NEON)



void WebRtcNsx_NoiseEstimationNeon(NsxInst_t* inst,
                                   uint16_t* magn,
                                   uint32_t* noise,
                                   int16_t* q_noise);
void WebRtcNsx_SynthesisUpdateNeon(NsxInst_t* inst,
                                   int16_t* out_frame,
                                   int16_t gain_factor);
void WebRtcNsx_AnalysisUpdateNeon(NsxInst_t* inst,
                                  int16_t* out,
                                  int16_t* new_speech);
void WebRtcNsx_PrepareSpectrumNeon(NsxInst_t* inst, int16_t* freq_buff);
#endif

#ifdef __cplusplus
}
#endif

#endif
