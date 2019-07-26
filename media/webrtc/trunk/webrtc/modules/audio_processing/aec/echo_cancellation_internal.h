









#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_AEC_ECHO_CANCELLATION_INTERNAL_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_AEC_ECHO_CANCELLATION_INTERNAL_H_

#include "webrtc/modules/audio_processing/aec/aec_core.h"
#include "webrtc/modules/audio_processing/utility/ring_buffer.h"

typedef struct {
  int delayCtr;
  int sampFreq;
  int splitSampFreq;
  int scSampFreq;
  float sampFactor;  
  short skewMode;
  int bufSizeStart;
  int knownDelay;
  int rate_factor;

  short initFlag;  

  
  short counter;
  int sum;
  short firstVal;
  short checkBufSizeCtr;

  
  short msInSndCardBuf;
  short filtDelay;  
  int timeForDelayChange;
  int startup_phase;
  int checkBuffSize;
  short lastDelayDiff;

#ifdef WEBRTC_AEC_DEBUG_DUMP
  RingBuffer* far_pre_buf_s16;  
  FILE* bufFile;
  FILE* delayFile;
  FILE* skewFile;
#endif

  
  void* resampler;

  int skewFrCtr;
  int resample;  
  int highSkewCtr;
  float skew;

  RingBuffer* far_pre_buf;  

  int lastError;

  int farend_started;

  AecCore* aec;
} aecpc_t;

#endif  
