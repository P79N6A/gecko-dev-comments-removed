









#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_AEC_ECHO_CANCELLATION_INTERNAL_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_AEC_ECHO_CANCELLATION_INTERNAL_H_

#include "modules/audio_processing/aec/aec_core.h"

typedef struct {
  int delayCtr;
  int sampFreq;
  int splitSampFreq;
  int scSampFreq;
  float sampFactor;  
  short nlpMode;
  short autoOnOff;
  short activity;
  short skewMode;
  int bufSizeStart;
  int knownDelay;

  short initFlag;  

  
  short counter;
  int sum;
  short firstVal;
  short checkBufSizeCtr;

  
  short msInSndCardBuf;
  short filtDelay;  
  int timeForDelayChange;
  int ECstartup;
  int checkBuffSize;
  short lastDelayDiff;

#ifdef WEBRTC_AEC_DEBUG_DUMP
  void* far_pre_buf_s16;  
  FILE* bufFile;
  FILE* delayFile;
  FILE* skewFile;
#endif

  
  void* resampler;

  int skewFrCtr;
  int resample;  
  int highSkewCtr;
  float skew;

  void* far_pre_buf;  

  int lastError;

  aec_t* aec;
} aecpc_t;

#endif  
