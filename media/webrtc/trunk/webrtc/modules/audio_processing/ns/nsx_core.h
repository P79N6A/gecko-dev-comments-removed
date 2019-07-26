









#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_NS_MAIN_SOURCE_NSX_CORE_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_NS_MAIN_SOURCE_NSX_CORE_H_

#include "common_audio/signal_processing/include/signal_processing_library.h"
#include "modules/audio_processing/ns/nsx_defines.h"
#include "typedefs.h"

#ifdef NS_FILEDEBUG
#include <stdio.h>
#endif

typedef struct NsxInst_t_ {
  WebRtc_UWord32          fs;

  const WebRtc_Word16*    window;
  WebRtc_Word16           analysisBuffer[ANAL_BLOCKL_MAX];
  WebRtc_Word16           synthesisBuffer[ANAL_BLOCKL_MAX];
  WebRtc_UWord16          noiseSupFilter[HALF_ANAL_BLOCKL];
  WebRtc_UWord16          overdrive; 
  WebRtc_UWord16          denoiseBound; 
  const WebRtc_Word16*    factor2Table;
  WebRtc_Word16           noiseEstLogQuantile[SIMULT* HALF_ANAL_BLOCKL];
  WebRtc_Word16           noiseEstDensity[SIMULT* HALF_ANAL_BLOCKL];
  WebRtc_Word16           noiseEstCounter[SIMULT];
  WebRtc_Word16           noiseEstQuantile[HALF_ANAL_BLOCKL];

  WebRtc_Word16           anaLen;
  int                     anaLen2;
  int                     magnLen;
  int                     aggrMode;
  int                     stages;
  int                     initFlag;
  int                     gainMap;

  WebRtc_Word32           maxLrt;
  WebRtc_Word32           minLrt;
  WebRtc_Word32           logLrtTimeAvgW32[HALF_ANAL_BLOCKL]; 
  WebRtc_Word32           featureLogLrt;
  WebRtc_Word32           thresholdLogLrt;
  WebRtc_Word16           weightLogLrt;

  WebRtc_UWord32          featureSpecDiff;
  WebRtc_UWord32          thresholdSpecDiff;
  WebRtc_Word16           weightSpecDiff;

  WebRtc_UWord32          featureSpecFlat;
  WebRtc_UWord32          thresholdSpecFlat;
  WebRtc_Word16           weightSpecFlat;

  WebRtc_Word32           avgMagnPause[HALF_ANAL_BLOCKL]; 
  WebRtc_UWord32          magnEnergy;
  WebRtc_UWord32          sumMagn;
  WebRtc_UWord32          curAvgMagnEnergy;
  WebRtc_UWord32          timeAvgMagnEnergy;
  WebRtc_UWord32          timeAvgMagnEnergyTmp;

  WebRtc_UWord32          whiteNoiseLevel;              
  WebRtc_UWord32          initMagnEst[HALF_ANAL_BLOCKL];
  WebRtc_Word32           pinkNoiseNumerator;           
  WebRtc_Word32           pinkNoiseExp;                 
  int                     minNorm;                      
  int                     zeroInputSignal;              

  WebRtc_UWord32          prevNoiseU32[HALF_ANAL_BLOCKL]; 
  WebRtc_UWord16          prevMagnU16[HALF_ANAL_BLOCKL]; 
  WebRtc_Word16           priorNonSpeechProb; 

  int                     blockIndex; 
  int                     modelUpdate; 
  int                     cntThresUpdate;

  
  WebRtc_Word16           histLrt[HIST_PAR_EST];
  WebRtc_Word16           histSpecFlat[HIST_PAR_EST];
  WebRtc_Word16           histSpecDiff[HIST_PAR_EST];

  
  WebRtc_Word16           dataBufHBFX[ANAL_BLOCKL_MAX]; 

  int                     qNoise;
  int                     prevQNoise;
  int                     prevQMagn;
  int                     blockLen10ms;

  WebRtc_Word16           real[ANAL_BLOCKL_MAX];
  WebRtc_Word16           imag[ANAL_BLOCKL_MAX];
  WebRtc_Word32           energyIn;
  int                     scaleEnergyIn;
  int                     normData;

  struct RealFFT* real_fft;
} NsxInst_t;

#ifdef __cplusplus
extern "C"
{
#endif
















WebRtc_Word32 WebRtcNsx_InitCore(NsxInst_t* inst, WebRtc_UWord32 fs);
















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


typedef void (*Denormalize)(NsxInst_t* inst,
                            int16_t* in,
                            int factor);
extern Denormalize WebRtcNsx_Denormalize;



typedef void (*CreateComplexBuffer)(NsxInst_t* inst,
                                    int16_t* in,
                                    int16_t* out);
extern CreateComplexBuffer WebRtcNsx_CreateComplexBuffer;

#if (defined WEBRTC_DETECT_ARM_NEON) || defined (WEBRTC_ARCH_ARM_NEON)



void WebRtcNsx_NoiseEstimationNeon(NsxInst_t* inst,
                                   uint16_t* magn,
                                   uint32_t* noise,
                                   int16_t* q_noise);
void WebRtcNsx_CreateComplexBufferNeon(NsxInst_t* inst,
                                       int16_t* in,
                                       int16_t* out);
void WebRtcNsx_SynthesisUpdateNeon(NsxInst_t* inst,
                                   int16_t* out_frame,
                                   int16_t gain_factor);
void WebRtcNsx_AnalysisUpdateNeon(NsxInst_t* inst,
                                  int16_t* out,
                                  int16_t* new_speech);
void WebRtcNsx_DenormalizeNeon(NsxInst_t* inst, int16_t* in, int factor);
void WebRtcNsx_PrepareSpectrumNeon(NsxInst_t* inst, int16_t* freq_buff);
#endif

extern const WebRtc_Word16 WebRtcNsx_kLogTable[9];
extern const WebRtc_Word16 WebRtcNsx_kLogTableFrac[256];
extern const WebRtc_Word16 WebRtcNsx_kCounterDiv[201];

#ifdef __cplusplus
}
#endif

#endif
