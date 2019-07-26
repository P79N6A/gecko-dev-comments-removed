









#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_AEC_AEC_CORE_INTERNAL_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_AEC_AEC_CORE_INTERNAL_H_

#ifdef WEBRTC_AEC_DEBUG_DUMP
#include <stdio.h>
#endif

#include "webrtc/modules/audio_processing/aec/aec_core.h"
#include "webrtc/modules/audio_processing/utility/ring_buffer.h"
#include "webrtc/typedefs.h"



enum {
  kExtendedNumPartitions = 32
};
static const int kNormalNumPartitions = 12;



static const float kExtendedMu = 0.4f;
static const float kExtendedErrorThreshold = 1.0e-6f;

typedef struct PowerLevel {
  float sfrsum;
  int sfrcounter;
  float framelevel;
  float frsum;
  int frcounter;
  float minlevel;
  float averagelevel;
} PowerLevel;

struct AecCore {
  int farBufWritePos, farBufReadPos;

  int knownDelay;
  int inSamples, outSamples;
  int delayEstCtr;

  RingBuffer* nearFrBuf;
  RingBuffer* outFrBuf;

  RingBuffer* nearFrBufH;
  RingBuffer* outFrBufH;

  float dBuf[PART_LEN2];  
  float eBuf[PART_LEN2];  

  float dBufH[PART_LEN2];  

  float xPow[PART_LEN1];
  float dPow[PART_LEN1];
  float dMinPow[PART_LEN1];
  float dInitMinPow[PART_LEN1];
  float* noisePow;

  float xfBuf[2][kExtendedNumPartitions * PART_LEN1];  
  float wfBuf[2][kExtendedNumPartitions * PART_LEN1];  
  complex_t sde[PART_LEN1];  
  complex_t sxd[PART_LEN1];  
  
  complex_t xfwBuf[kExtendedNumPartitions * PART_LEN1];

  float sx[PART_LEN1], sd[PART_LEN1], se[PART_LEN1];  
  float hNs[PART_LEN1];
  float hNlFbMin, hNlFbLocalMin;
  float hNlXdAvgMin;
  int hNlNewMin, hNlMinCtr;
  float overDrive, overDriveSm;
  int nlp_mode;
  float outBuf[PART_LEN];
  int delayIdx;

  short stNearState, echoState;
  short divergeState;

  int xfBufBlockPos;

  RingBuffer* far_buf;
  RingBuffer* far_buf_windowed;
  int system_delay;  

  int mult;  
  int sampFreq;
  uint32_t seed;

  float normal_mu;               
  float normal_error_threshold;  

  int noiseEstCtr;

  PowerLevel farlevel;
  PowerLevel nearlevel;
  PowerLevel linoutlevel;
  PowerLevel nlpoutlevel;

  int metricsMode;
  int stateCounter;
  Stats erl;
  Stats erle;
  Stats aNlp;
  Stats rerl;

  
  int freq_avg_ic;       
  int flag_Hband_cn;     
  float cn_scale_Hband;  

  int delay_histogram[kHistorySizeBlocks];
  int delay_logging_enabled;
  void* delay_estimator_farend;
  void* delay_estimator;

  
  int extended_filter_enabled;
  
  int num_partitions;

#ifdef WEBRTC_AEC_DEBUG_DUMP
  RingBuffer* far_time_buf;
  FILE* farFile;
  FILE* nearFile;
  FILE* outFile;
  FILE* outLinearFile;
#endif
};

typedef void (*WebRtcAec_FilterFar_t)(AecCore* aec, float yf[2][PART_LEN1]);
extern WebRtcAec_FilterFar_t WebRtcAec_FilterFar;
typedef void (*WebRtcAec_ScaleErrorSignal_t)(AecCore* aec,
                                             float ef[2][PART_LEN1]);
extern WebRtcAec_ScaleErrorSignal_t WebRtcAec_ScaleErrorSignal;
typedef void (*WebRtcAec_FilterAdaptation_t)(AecCore* aec,
                                             float* fft,
                                             float ef[2][PART_LEN1]);
extern WebRtcAec_FilterAdaptation_t WebRtcAec_FilterAdaptation;
typedef void (*WebRtcAec_OverdriveAndSuppress_t)(AecCore* aec,
                                                 float hNl[PART_LEN1],
                                                 const float hNlFb,
                                                 float efw[2][PART_LEN1]);
extern WebRtcAec_OverdriveAndSuppress_t WebRtcAec_OverdriveAndSuppress;

#endif  
