












#include "webrtc/modules/audio_processing/aec/include/echo_cancellation.h"

#include <math.h>
#ifdef WEBRTC_AEC_DEBUG_DUMP
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>

#include "webrtc/common_audio/signal_processing/include/signal_processing_library.h"
#include "webrtc/modules/audio_processing/aec/aec_core.h"
#include "webrtc/modules/audio_processing/aec/aec_resampler.h"
#include "webrtc/modules/audio_processing/aec/echo_cancellation_internal.h"
#include "webrtc/modules/audio_processing/utility/ring_buffer.h"
#include "webrtc/typedefs.h"






































#if defined(WEBRTC_CHROMIUM_BUILD) && defined(WEBRTC_MAC)
#define WEBRTC_UNTRUSTED_DELAY

#if defined(WEBRTC_MAC)
static const int kDelayDiffOffsetSamples = -160;
#else

static const int kDelayDiffOffsetSamples = 0;
#endif
#endif

#if defined(WEBRTC_MAC)
static const int kFixedDelayMs = 20;
#else
static const int kFixedDelayMs = 50;
#endif
#if !defined(WEBRTC_UNTRUSTED_DELAY)
static const int kMinTrustedDelayMs = 20;
#endif
static const int kMaxTrustedDelayMs = 500;





#define MAX_RESAMP_LEN (5 * FRAME_LEN)

static const int kMaxBufSizeStart = 62;  
static const int sampMsNb = 8;           
static const int initCheck = 42;

#ifdef WEBRTC_AEC_DEBUG_DUMP
int webrtc_aec_instance_count = 0;
#endif



static void EstBufDelayNormal(aecpc_t* aecInst);
static void EstBufDelayExtended(aecpc_t* aecInst);
static int ProcessNormal(aecpc_t* self,
                         const int16_t* near,
                         const int16_t* near_high,
                         int16_t* out,
                         int16_t* out_high,
                         int16_t num_samples,
                         int16_t reported_delay_ms,
                         int32_t skew);
static void ProcessExtended(aecpc_t* self,
                            const int16_t* near,
                            const int16_t* near_high,
                            int16_t* out,
                            int16_t* out_high,
                            int16_t num_samples,
                            int16_t reported_delay_ms,
                            int32_t skew);

int32_t WebRtcAec_Create(void** aecInst) {
  aecpc_t* aecpc;
  if (aecInst == NULL) {
    return -1;
  }

  aecpc = malloc(sizeof(aecpc_t));
  *aecInst = aecpc;
  if (aecpc == NULL) {
    return -1;
  }

  if (WebRtcAec_CreateAec(&aecpc->aec) == -1) {
    WebRtcAec_Free(aecpc);
    aecpc = NULL;
    return -1;
  }

  if (WebRtcAec_CreateResampler(&aecpc->resampler) == -1) {
    WebRtcAec_Free(aecpc);
    aecpc = NULL;
    return -1;
  }
  
  
  
  aecpc->far_pre_buf =
      WebRtc_CreateBuffer(PART_LEN2 + kResamplerBufferSize, sizeof(float));
  if (!aecpc->far_pre_buf) {
    WebRtcAec_Free(aecpc);
    aecpc = NULL;
    return -1;
  }

  aecpc->initFlag = 0;
  aecpc->lastError = 0;

#ifdef WEBRTC_AEC_DEBUG_DUMP
  aecpc->far_pre_buf_s16 =
      WebRtc_CreateBuffer(PART_LEN2 + kResamplerBufferSize, sizeof(int16_t));
  if (!aecpc->far_pre_buf_s16) {
    WebRtcAec_Free(aecpc);
    aecpc = NULL;
    return -1;
  }
  {
    char filename[64];
    sprintf(filename, "aec_buf%d.dat", webrtc_aec_instance_count);
    aecpc->bufFile = fopen(filename, "wb");
    sprintf(filename, "aec_skew%d.dat", webrtc_aec_instance_count);
    aecpc->skewFile = fopen(filename, "wb");
    sprintf(filename, "aec_delay%d.dat", webrtc_aec_instance_count);
    aecpc->delayFile = fopen(filename, "wb");
    webrtc_aec_instance_count++;
  }
#endif

  return 0;
}

int32_t WebRtcAec_Free(void* aecInst) {
  aecpc_t* aecpc = aecInst;

  if (aecpc == NULL) {
    return -1;
  }

  WebRtc_FreeBuffer(aecpc->far_pre_buf);

#ifdef WEBRTC_AEC_DEBUG_DUMP
  WebRtc_FreeBuffer(aecpc->far_pre_buf_s16);
  fclose(aecpc->bufFile);
  fclose(aecpc->skewFile);
  fclose(aecpc->delayFile);
#endif

  WebRtcAec_FreeAec(aecpc->aec);
  WebRtcAec_FreeResampler(aecpc->resampler);
  free(aecpc);

  return 0;
}

int32_t WebRtcAec_Init(void* aecInst, int32_t sampFreq, int32_t scSampFreq) {
  aecpc_t* aecpc = aecInst;
  AecConfig aecConfig;

  if (sampFreq != 8000 && sampFreq != 16000 && sampFreq != 32000) {
    aecpc->lastError = AEC_BAD_PARAMETER_ERROR;
    return -1;
  }
  aecpc->sampFreq = sampFreq;

  if (scSampFreq < 1 || scSampFreq > 96000) {
    aecpc->lastError = AEC_BAD_PARAMETER_ERROR;
    return -1;
  }
  aecpc->scSampFreq = scSampFreq;

  
  if (WebRtcAec_InitAec(aecpc->aec, aecpc->sampFreq) == -1) {
    aecpc->lastError = AEC_UNSPECIFIED_ERROR;
    return -1;
  }

  if (WebRtcAec_InitResampler(aecpc->resampler, aecpc->scSampFreq) == -1) {
    aecpc->lastError = AEC_UNSPECIFIED_ERROR;
    return -1;
  }

  if (WebRtc_InitBuffer(aecpc->far_pre_buf) == -1) {
    aecpc->lastError = AEC_UNSPECIFIED_ERROR;
    return -1;
  }
  WebRtc_MoveReadPtr(aecpc->far_pre_buf, -PART_LEN);  

  aecpc->initFlag = initCheck;  

  if (aecpc->sampFreq == 32000) {
    aecpc->splitSampFreq = 16000;
  } else {
    aecpc->splitSampFreq = sampFreq;
  }

  aecpc->delayCtr = 0;
  aecpc->sampFactor = (aecpc->scSampFreq * 1.0f) / aecpc->splitSampFreq;
  
  aecpc->rate_factor = aecpc->splitSampFreq / 8000;

  aecpc->sum = 0;
  aecpc->counter = 0;
  aecpc->checkBuffSize = 1;
  aecpc->firstVal = 0;

  aecpc->startup_phase = 1;
  aecpc->bufSizeStart = 0;
  aecpc->checkBufSizeCtr = 0;
  aecpc->msInSndCardBuf = 0;
  aecpc->filtDelay = -1;  
  aecpc->timeForDelayChange = 0;
  aecpc->knownDelay = 0;
  aecpc->lastDelayDiff = 0;

  aecpc->skewFrCtr = 0;
  aecpc->resample = kAecFalse;
  aecpc->highSkewCtr = 0;
  aecpc->skew = 0;

  aecpc->farend_started = 0;

  
  aecConfig.nlpMode = kAecNlpModerate;
  aecConfig.skewMode = kAecFalse;
  aecConfig.metricsMode = kAecFalse;
  aecConfig.delay_logging = kAecFalse;

  if (WebRtcAec_set_config(aecpc, aecConfig) == -1) {
    aecpc->lastError = AEC_UNSPECIFIED_ERROR;
    return -1;
  }

#ifdef WEBRTC_AEC_DEBUG_DUMP
  if (WebRtc_InitBuffer(aecpc->far_pre_buf_s16) == -1) {
    aecpc->lastError = AEC_UNSPECIFIED_ERROR;
    return -1;
  }
  WebRtc_MoveReadPtr(aecpc->far_pre_buf_s16, -PART_LEN);  
#endif

  return 0;
}


int32_t WebRtcAec_BufferFarend(void* aecInst,
                               const int16_t* farend,
                               int16_t nrOfSamples) {
  aecpc_t* aecpc = aecInst;
  int32_t retVal = 0;
  int newNrOfSamples = (int)nrOfSamples;
  short newFarend[MAX_RESAMP_LEN];
  const int16_t* farend_ptr = farend;
  float tmp_farend[MAX_RESAMP_LEN];
  const float* farend_float = tmp_farend;
  float skew;
  int i = 0;

  if (farend == NULL) {
    aecpc->lastError = AEC_NULL_POINTER_ERROR;
    return -1;
  }

  if (aecpc->initFlag != initCheck) {
    aecpc->lastError = AEC_UNINITIALIZED_ERROR;
    return -1;
  }

  
  if (nrOfSamples != 80 && nrOfSamples != 160) {
    aecpc->lastError = AEC_BAD_PARAMETER_ERROR;
    return -1;
  }

  skew = aecpc->skew;

  if (aecpc->skewMode == kAecTrue && aecpc->resample == kAecTrue) {
    
    WebRtcAec_ResampleLinear(aecpc->resampler,
                             farend,
                             nrOfSamples,
                             skew,
                             newFarend,
                             &newNrOfSamples);
    farend_ptr = (const int16_t*)newFarend;
  }

  aecpc->farend_started = 1;
  WebRtcAec_SetSystemDelay(aecpc->aec,
                           WebRtcAec_system_delay(aecpc->aec) + newNrOfSamples);

#ifdef WEBRTC_AEC_DEBUG_DUMP
  WebRtc_WriteBuffer(
      aecpc->far_pre_buf_s16, farend_ptr, (size_t)newNrOfSamples);
#endif
  
  for (i = 0; i < newNrOfSamples; i++) {
    tmp_farend[i] = (float)farend_ptr[i];
  }
  WebRtc_WriteBuffer(aecpc->far_pre_buf, farend_float, (size_t)newNrOfSamples);

  
  while (WebRtc_available_read(aecpc->far_pre_buf) >= PART_LEN2) {
    
    WebRtc_ReadBuffer(
        aecpc->far_pre_buf, (void**)&farend_float, tmp_farend, PART_LEN2);

    WebRtcAec_BufferFarendPartition(aecpc->aec, farend_float);

    
    WebRtc_MoveReadPtr(aecpc->far_pre_buf, -PART_LEN);
#ifdef WEBRTC_AEC_DEBUG_DUMP
    WebRtc_ReadBuffer(
        aecpc->far_pre_buf_s16, (void**)&farend_ptr, newFarend, PART_LEN2);
    WebRtc_WriteBuffer(
        WebRtcAec_far_time_buf(aecpc->aec), &farend_ptr[PART_LEN], 1);
    WebRtc_MoveReadPtr(aecpc->far_pre_buf_s16, -PART_LEN);
#endif
  }

  return retVal;
}

int32_t WebRtcAec_Process(void* aecInst,
                          const int16_t* nearend,
                          const int16_t* nearendH,
                          int16_t* out,
                          int16_t* outH,
                          int16_t nrOfSamples,
                          int16_t msInSndCardBuf,
                          int32_t skew) {
  aecpc_t* aecpc = aecInst;
  int32_t retVal = 0;
  if (nearend == NULL) {
    aecpc->lastError = AEC_NULL_POINTER_ERROR;
    return -1;
  }

  if (out == NULL) {
    aecpc->lastError = AEC_NULL_POINTER_ERROR;
    return -1;
  }

  if (aecpc->initFlag != initCheck) {
    aecpc->lastError = AEC_UNINITIALIZED_ERROR;
    return -1;
  }

  
  if (nrOfSamples != 80 && nrOfSamples != 160) {
    aecpc->lastError = AEC_BAD_PARAMETER_ERROR;
    return -1;
  }

  
  if (aecpc->sampFreq == 32000 && nearendH == NULL) {
    aecpc->lastError = AEC_NULL_POINTER_ERROR;
    return -1;
  }

  if (msInSndCardBuf < 0) {
    msInSndCardBuf = 0;
    aecpc->lastError = AEC_BAD_PARAMETER_WARNING;
    retVal = -1;
  } else if (msInSndCardBuf > kMaxTrustedDelayMs) {
    
    aecpc->lastError = AEC_BAD_PARAMETER_WARNING;
    retVal = -1;
  }

  
  if (WebRtcAec_delay_correction_enabled(aecpc->aec)) {
    ProcessExtended(
        aecpc, nearend, nearendH, out, outH, nrOfSamples, msInSndCardBuf, skew);
  } else {
    if (ProcessNormal(aecpc,
                      nearend,
                      nearendH,
                      out,
                      outH,
                      nrOfSamples,
                      msInSndCardBuf,
                      skew) != 0) {
      retVal = -1;
    }
  }

#ifdef WEBRTC_AEC_DEBUG_DUMP
  {
    int16_t far_buf_size_ms = (int16_t)(WebRtcAec_system_delay(aecpc->aec) /
                                        (sampMsNb * aecpc->rate_factor));
    (void)fwrite(&far_buf_size_ms, 2, 1, aecpc->bufFile);
    (void)fwrite(
        &aecpc->knownDelay, sizeof(aecpc->knownDelay), 1, aecpc->delayFile);
  }
#endif

  return retVal;
}

int WebRtcAec_set_config(void* handle, AecConfig config) {
  aecpc_t* self = (aecpc_t*)handle;
  if (self->initFlag != initCheck) {
    self->lastError = AEC_UNINITIALIZED_ERROR;
    return -1;
  }

  if (config.skewMode != kAecFalse && config.skewMode != kAecTrue) {
    self->lastError = AEC_BAD_PARAMETER_ERROR;
    return -1;
  }
  self->skewMode = config.skewMode;

  if (config.nlpMode != kAecNlpConservative &&
      config.nlpMode != kAecNlpModerate &&
      config.nlpMode != kAecNlpAggressive) {
    self->lastError = AEC_BAD_PARAMETER_ERROR;
    return -1;
  }

  if (config.metricsMode != kAecFalse && config.metricsMode != kAecTrue) {
    self->lastError = AEC_BAD_PARAMETER_ERROR;
    return -1;
  }

  if (config.delay_logging != kAecFalse && config.delay_logging != kAecTrue) {
    self->lastError = AEC_BAD_PARAMETER_ERROR;
    return -1;
  }

  WebRtcAec_SetConfigCore(
      self->aec, config.nlpMode, config.metricsMode, config.delay_logging);
  return 0;
}

int WebRtcAec_get_echo_status(void* handle, int* status) {
  aecpc_t* self = (aecpc_t*)handle;
  if (status == NULL) {
    self->lastError = AEC_NULL_POINTER_ERROR;
    return -1;
  }
  if (self->initFlag != initCheck) {
    self->lastError = AEC_UNINITIALIZED_ERROR;
    return -1;
  }

  *status = WebRtcAec_echo_state(self->aec);

  return 0;
}

int WebRtcAec_GetMetrics(void* handle, AecMetrics* metrics) {
  const float kUpWeight = 0.7f;
  float dtmp;
  int stmp;
  aecpc_t* self = (aecpc_t*)handle;
  Stats erl;
  Stats erle;
  Stats a_nlp;

  if (handle == NULL) {
    return -1;
  }
  if (metrics == NULL) {
    self->lastError = AEC_NULL_POINTER_ERROR;
    return -1;
  }
  if (self->initFlag != initCheck) {
    self->lastError = AEC_UNINITIALIZED_ERROR;
    return -1;
  }

  WebRtcAec_GetEchoStats(self->aec, &erl, &erle, &a_nlp);

  
  metrics->erl.instant = (int)erl.instant;

  if ((erl.himean > kOffsetLevel) && (erl.average > kOffsetLevel)) {
    
    dtmp = kUpWeight * erl.himean + (1 - kUpWeight) * erl.average;
    metrics->erl.average = (int)dtmp;
  } else {
    metrics->erl.average = kOffsetLevel;
  }

  metrics->erl.max = (int)erl.max;

  if (erl.min < (kOffsetLevel * (-1))) {
    metrics->erl.min = (int)erl.min;
  } else {
    metrics->erl.min = kOffsetLevel;
  }

  
  metrics->erle.instant = (int)erle.instant;

  if ((erle.himean > kOffsetLevel) && (erle.average > kOffsetLevel)) {
    
    dtmp = kUpWeight * erle.himean + (1 - kUpWeight) * erle.average;
    metrics->erle.average = (int)dtmp;
  } else {
    metrics->erle.average = kOffsetLevel;
  }

  metrics->erle.max = (int)erle.max;

  if (erle.min < (kOffsetLevel * (-1))) {
    metrics->erle.min = (int)erle.min;
  } else {
    metrics->erle.min = kOffsetLevel;
  }

  
  if ((metrics->erl.average > kOffsetLevel) &&
      (metrics->erle.average > kOffsetLevel)) {
    stmp = metrics->erl.average + metrics->erle.average;
  } else {
    stmp = kOffsetLevel;
  }
  metrics->rerl.average = stmp;

  
  metrics->rerl.instant = stmp;
  metrics->rerl.max = stmp;
  metrics->rerl.min = stmp;

  
  metrics->aNlp.instant = (int)a_nlp.instant;

  if ((a_nlp.himean > kOffsetLevel) && (a_nlp.average > kOffsetLevel)) {
    
    dtmp = kUpWeight * a_nlp.himean + (1 - kUpWeight) * a_nlp.average;
    metrics->aNlp.average = (int)dtmp;
  } else {
    metrics->aNlp.average = kOffsetLevel;
  }

  metrics->aNlp.max = (int)a_nlp.max;

  if (a_nlp.min < (kOffsetLevel * (-1))) {
    metrics->aNlp.min = (int)a_nlp.min;
  } else {
    metrics->aNlp.min = kOffsetLevel;
  }

  return 0;
}

int WebRtcAec_GetDelayMetrics(void* handle, int* median, int* std) {
  aecpc_t* self = handle;
  if (median == NULL) {
    self->lastError = AEC_NULL_POINTER_ERROR;
    return -1;
  }
  if (std == NULL) {
    self->lastError = AEC_NULL_POINTER_ERROR;
    return -1;
  }
  if (self->initFlag != initCheck) {
    self->lastError = AEC_UNINITIALIZED_ERROR;
    return -1;
  }
  if (WebRtcAec_GetDelayMetricsCore(self->aec, median, std) == -1) {
    
    self->lastError = AEC_UNSUPPORTED_FUNCTION_ERROR;
    return -1;
  }

  return 0;
}

int32_t WebRtcAec_get_error_code(void* aecInst) {
  aecpc_t* aecpc = aecInst;
  return aecpc->lastError;
}

AecCore* WebRtcAec_aec_core(void* handle) {
  if (!handle) {
    return NULL;
  }
  return ((aecpc_t*)handle)->aec;
}

static int ProcessNormal(aecpc_t* aecpc,
                         const int16_t* nearend,
                         const int16_t* nearendH,
                         int16_t* out,
                         int16_t* outH,
                         int16_t nrOfSamples,
                         int16_t msInSndCardBuf,
                         int32_t skew) {
  int retVal = 0;
  short i;
  short nBlocks10ms;
  short nFrames;
  
  const float minSkewEst = -0.5f;
  const float maxSkewEst = 1.0f;

  msInSndCardBuf =
      msInSndCardBuf > kMaxTrustedDelayMs ? kMaxTrustedDelayMs : msInSndCardBuf;
  
  msInSndCardBuf += 10;
  aecpc->msInSndCardBuf = msInSndCardBuf;

  if (aecpc->skewMode == kAecTrue) {
    if (aecpc->skewFrCtr < 25) {
      aecpc->skewFrCtr++;
    } else {
      retVal = WebRtcAec_GetSkew(aecpc->resampler, skew, &aecpc->skew);
      if (retVal == -1) {
        aecpc->skew = 0;
        aecpc->lastError = AEC_BAD_PARAMETER_WARNING;
      }

      aecpc->skew /= aecpc->sampFactor * nrOfSamples;

      if (aecpc->skew < 1.0e-3 && aecpc->skew > -1.0e-3) {
        aecpc->resample = kAecFalse;
      } else {
        aecpc->resample = kAecTrue;
      }

      if (aecpc->skew < minSkewEst) {
        aecpc->skew = minSkewEst;
      } else if (aecpc->skew > maxSkewEst) {
        aecpc->skew = maxSkewEst;
      }

#ifdef WEBRTC_AEC_DEBUG_DUMP
      (void)fwrite(&aecpc->skew, sizeof(aecpc->skew), 1, aecpc->skewFile);
#endif
    }
  }

  nFrames = nrOfSamples / FRAME_LEN;
  nBlocks10ms = nFrames / aecpc->rate_factor;

  if (aecpc->startup_phase) {
    
    if (nearend != out) {
      memcpy(out, nearend, sizeof(short) * nrOfSamples);
    }
    if (nearendH != outH) {
      memcpy(outH, nearendH, sizeof(short) * nrOfSamples);
    }

    
    

    
    if (aecpc->checkBuffSize) {
      aecpc->checkBufSizeCtr++;
      
      
      
      
      
      if (aecpc->counter == 0) {
        aecpc->firstVal = aecpc->msInSndCardBuf;
        aecpc->sum = 0;
      }

      if (abs(aecpc->firstVal - aecpc->msInSndCardBuf) <
          WEBRTC_SPL_MAX(0.2 * aecpc->msInSndCardBuf, sampMsNb)) {
        aecpc->sum += aecpc->msInSndCardBuf;
        aecpc->counter++;
      } else {
        aecpc->counter = 0;
      }

      if (aecpc->counter * nBlocks10ms >= 6) {
        
        
        
        aecpc->bufSizeStart =
            WEBRTC_SPL_MIN((3 * aecpc->sum * aecpc->rate_factor * 8) /
                               (4 * aecpc->counter * PART_LEN),
                           kMaxBufSizeStart);
        
        aecpc->checkBuffSize = 0;
      }

      if (aecpc->checkBufSizeCtr * nBlocks10ms > 50) {
        
        
        aecpc->bufSizeStart = WEBRTC_SPL_MIN(
            (aecpc->msInSndCardBuf * aecpc->rate_factor * 3) / 40,
            kMaxBufSizeStart);
        aecpc->checkBuffSize = 0;
      }
    }

    
    if (!aecpc->checkBuffSize) {
      
      
      
      
      int overhead_elements =
          WebRtcAec_system_delay(aecpc->aec) / PART_LEN - aecpc->bufSizeStart;
      if (overhead_elements == 0) {
        
        aecpc->startup_phase = 0;
      } else if (overhead_elements > 0) {
        
        
        
        
        
        WebRtcAec_MoveFarReadPtr(aecpc->aec, overhead_elements);

        
        aecpc->startup_phase = 0;
      }
    }
  } else {
    
    EstBufDelayNormal(aecpc);

    
    for (i = 0; i < nFrames; i++) {
      
      WebRtcAec_ProcessFrame(aecpc->aec,
                             &nearend[FRAME_LEN * i],
                             &nearendH[FRAME_LEN * i],
                             aecpc->knownDelay,
                             &out[FRAME_LEN * i],
                             &outH[FRAME_LEN * i]);
      
      
      
    }
  }

  return retVal;
}

static void ProcessExtended(aecpc_t* self,
                            const int16_t* near,
                            const int16_t* near_high,
                            int16_t* out,
                            int16_t* out_high,
                            int16_t num_samples,
                            int16_t reported_delay_ms,
                            int32_t skew) {
  int i;
  const int num_frames = num_samples / FRAME_LEN;
#if defined(WEBRTC_UNTRUSTED_DELAY)
  const int delay_diff_offset = kDelayDiffOffsetSamples;
  reported_delay_ms = kFixedDelayMs;
#else
  
  const int delay_diff_offset = 0;
  
  
  
  reported_delay_ms = reported_delay_ms < kMinTrustedDelayMs
                          ? kMinTrustedDelayMs
                          : reported_delay_ms;
  
  
  
  
  reported_delay_ms = reported_delay_ms >= kMaxTrustedDelayMs
                          ? kFixedDelayMs
                          : reported_delay_ms;
#endif
  self->msInSndCardBuf = reported_delay_ms;

  if (!self->farend_started) {
    
    if (near != out) {
      memcpy(out, near, sizeof(short) * num_samples);
    }
    if (near_high != out_high) {
      memcpy(out_high, near_high, sizeof(short) * num_samples);
    }
    return;
  }
  if (self->startup_phase) {
    
    
    
    
    int startup_size_ms =
        reported_delay_ms < kFixedDelayMs ? kFixedDelayMs : reported_delay_ms;
    int overhead_elements = (WebRtcAec_system_delay(self->aec) -
                             startup_size_ms / 2 * self->rate_factor * 8) /
                            PART_LEN;
    WebRtcAec_MoveFarReadPtr(self->aec, overhead_elements);
    self->startup_phase = 0;
  }

  EstBufDelayExtended(self);

  {
    
    
    
    const int adjusted_known_delay =
        WEBRTC_SPL_MAX(0, self->knownDelay + delay_diff_offset);

    for (i = 0; i < num_frames; ++i) {
      WebRtcAec_ProcessFrame(self->aec,
                             &near[FRAME_LEN * i],
                             &near_high[FRAME_LEN * i],
                             adjusted_known_delay,
                             &out[FRAME_LEN * i],
                             &out_high[FRAME_LEN * i]);
    }
  }
}

static void EstBufDelayNormal(aecpc_t* aecpc) {
  int nSampSndCard = aecpc->msInSndCardBuf * sampMsNb * aecpc->rate_factor;
  int current_delay = nSampSndCard - WebRtcAec_system_delay(aecpc->aec);
  int delay_difference = 0;

  
  
  
  
  

  
  current_delay += FRAME_LEN * aecpc->rate_factor;

  
  if (aecpc->skewMode == kAecTrue && aecpc->resample == kAecTrue) {
    current_delay -= kResamplingDelay;
  }

  
  if (current_delay < PART_LEN) {
    current_delay += WebRtcAec_MoveFarReadPtr(aecpc->aec, 1) * PART_LEN;
  }

  
  
  aecpc->filtDelay = aecpc->filtDelay < 0 ? 0 : aecpc->filtDelay;
  aecpc->filtDelay =
      WEBRTC_SPL_MAX(0, (short)(0.8 * aecpc->filtDelay + 0.2 * current_delay));

  delay_difference = aecpc->filtDelay - aecpc->knownDelay;
  if (delay_difference > 224) {
    if (aecpc->lastDelayDiff < 96) {
      aecpc->timeForDelayChange = 0;
    } else {
      aecpc->timeForDelayChange++;
    }
  } else if (delay_difference < 96 && aecpc->knownDelay > 0) {
    if (aecpc->lastDelayDiff > 224) {
      aecpc->timeForDelayChange = 0;
    } else {
      aecpc->timeForDelayChange++;
    }
  } else {
    aecpc->timeForDelayChange = 0;
  }
  aecpc->lastDelayDiff = delay_difference;

  if (aecpc->timeForDelayChange > 25) {
    aecpc->knownDelay = WEBRTC_SPL_MAX((int)aecpc->filtDelay - 160, 0);
  }
}

static void EstBufDelayExtended(aecpc_t* self) {
  int reported_delay = self->msInSndCardBuf * sampMsNb * self->rate_factor;
  int current_delay = reported_delay - WebRtcAec_system_delay(self->aec);
  int delay_difference = 0;

  
  
  
  
  

  
  current_delay += FRAME_LEN * self->rate_factor;

  
  if (self->skewMode == kAecTrue && self->resample == kAecTrue) {
    current_delay -= kResamplingDelay;
  }

  
  if (current_delay < PART_LEN) {
    current_delay += WebRtcAec_MoveFarReadPtr(self->aec, 2) * PART_LEN;
  }

  if (self->filtDelay == -1) {
    self->filtDelay = WEBRTC_SPL_MAX(0, 0.5 * current_delay);
  } else {
    self->filtDelay = WEBRTC_SPL_MAX(
        0, (short)(0.95 * self->filtDelay + 0.05 * current_delay));
  }

  delay_difference = self->filtDelay - self->knownDelay;
  if (delay_difference > 384) {
    if (self->lastDelayDiff < 128) {
      self->timeForDelayChange = 0;
    } else {
      self->timeForDelayChange++;
    }
  } else if (delay_difference < 128 && self->knownDelay > 0) {
    if (self->lastDelayDiff > 384) {
      self->timeForDelayChange = 0;
    } else {
      self->timeForDelayChange++;
    }
  } else {
    self->timeForDelayChange = 0;
  }
  self->lastDelayDiff = delay_difference;

  if (self->timeForDelayChange > 25) {
    self->knownDelay = WEBRTC_SPL_MAX((int)self->filtDelay - 256, 0);
  }
}
