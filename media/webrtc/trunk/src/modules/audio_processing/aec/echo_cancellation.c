












#include "echo_cancellation.h"

#include <math.h>
#ifdef WEBRTC_AEC_DEBUG_DUMP
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>

#include "aec_core.h"
#include "aec_resampler.h"
#include "common_audio/signal_processing/include/signal_processing_library.h"
#include "modules/audio_processing/aec/echo_cancellation_internal.h"
#include "ring_buffer.h"
#include "typedefs.h"





#define MAX_RESAMP_LEN (5 * FRAME_LEN)

static const int kMaxBufSizeStart = 62;  
static const int sampMsNb = 8; 


static const float targetSupp[3] = {-6.9f, -11.5f, -18.4f};
static const float minOverDrive[3] = {1.0f, 2.0f, 5.0f};
static const int initCheck = 42;

#ifdef WEBRTC_AEC_DEBUG_DUMP
static int instance_count = 0;
#endif



static int EstBufDelay(aecpc_t *aecInst);

WebRtc_Word32 WebRtcAec_Create(void **aecInst)
{
    aecpc_t *aecpc;
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
    
    
    
    if (WebRtc_CreateBuffer(&aecpc->far_pre_buf,
                            PART_LEN2 + kResamplerBufferSize,
                            sizeof(float)) == -1) {
        WebRtcAec_Free(aecpc);
        aecpc = NULL;
        return -1;
    }

    aecpc->initFlag = 0;
    aecpc->lastError = 0;

#ifdef WEBRTC_AEC_DEBUG_DUMP
    if (WebRtc_CreateBuffer(&aecpc->far_pre_buf_s16,
                            PART_LEN2 + kResamplerBufferSize,
                            sizeof(int16_t)) == -1) {
        WebRtcAec_Free(aecpc);
        aecpc = NULL;
        return -1;
    }
    {
      char filename[64];
      sprintf(filename, "aec_far%d.pcm", instance_count);
      aecpc->aec->farFile = fopen(filename, "wb");
      sprintf(filename, "aec_near%d.pcm", instance_count);
      aecpc->aec->nearFile = fopen(filename, "wb");
      sprintf(filename, "aec_out%d.pcm", instance_count);
      aecpc->aec->outFile = fopen(filename, "wb");
      sprintf(filename, "aec_out_linear%d.pcm", instance_count);
      aecpc->aec->outLinearFile = fopen(filename, "wb");
      sprintf(filename, "aec_buf%d.dat", instance_count);
      aecpc->bufFile = fopen(filename, "wb");
      sprintf(filename, "aec_skew%d.dat", instance_count);
      aecpc->skewFile = fopen(filename, "wb");
      sprintf(filename, "aec_delay%d.dat", instance_count);
      aecpc->delayFile = fopen(filename, "wb");
      instance_count++;
    }
#endif

    return 0;
}

WebRtc_Word32 WebRtcAec_Free(void *aecInst)
{
    aecpc_t *aecpc = aecInst;

    if (aecpc == NULL) {
        return -1;
    }

    WebRtc_FreeBuffer(aecpc->far_pre_buf);

#ifdef WEBRTC_AEC_DEBUG_DUMP
    WebRtc_FreeBuffer(aecpc->far_pre_buf_s16);
    fclose(aecpc->aec->farFile);
    fclose(aecpc->aec->nearFile);
    fclose(aecpc->aec->outFile);
    fclose(aecpc->aec->outLinearFile);
    fclose(aecpc->bufFile);
    fclose(aecpc->skewFile);
    fclose(aecpc->delayFile);
#endif

    WebRtcAec_FreeAec(aecpc->aec);
    WebRtcAec_FreeResampler(aecpc->resampler);
    free(aecpc);

    return 0;
}

WebRtc_Word32 WebRtcAec_Init(void *aecInst, WebRtc_Word32 sampFreq, WebRtc_Word32 scSampFreq)
{
    aecpc_t *aecpc = aecInst;
    AecConfig aecConfig;

    if (aecpc == NULL) {
        return -1;
    }

    if (sampFreq != 8000 && sampFreq != 16000  && sampFreq != 32000) {
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
    }
    else {
        aecpc->splitSampFreq = sampFreq;
    }

    aecpc->skewFrCtr = 0;
    aecpc->activity = 0;

    aecpc->delayCtr = 0;

    aecpc->sum = 0;
    aecpc->counter = 0;
    aecpc->checkBuffSize = 1;
    aecpc->firstVal = 0;

    aecpc->ECstartup = 1;
    aecpc->bufSizeStart = 0;
    aecpc->checkBufSizeCtr = 0;
    aecpc->filtDelay = 0;
    aecpc->timeForDelayChange = 0;
    aecpc->knownDelay = 0;
    aecpc->lastDelayDiff = 0;

    aecpc->skew = 0;
    aecpc->resample = kAecFalse;
    aecpc->highSkewCtr = 0;
    aecpc->sampFactor = (aecpc->scSampFreq * 1.0f) / aecpc->splitSampFreq;

    
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


WebRtc_Word32 WebRtcAec_BufferFarend(void *aecInst, const WebRtc_Word16 *farend,
    WebRtc_Word16 nrOfSamples)
{
    aecpc_t *aecpc = aecInst;
    WebRtc_Word32 retVal = 0;
    int newNrOfSamples = (int) nrOfSamples;
    short newFarend[MAX_RESAMP_LEN];
    const int16_t* farend_ptr = farend;
    float tmp_farend[MAX_RESAMP_LEN];
    const float* farend_float = tmp_farend;
    float skew;
    int i = 0;

    if (aecpc == NULL) {
        return -1;
    }

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
        
        WebRtcAec_ResampleLinear(aecpc->resampler, farend, nrOfSamples, skew,
                                 newFarend, &newNrOfSamples);
        farend_ptr = (const int16_t*) newFarend;
    }

    aecpc->aec->system_delay += newNrOfSamples;

#ifdef WEBRTC_AEC_DEBUG_DUMP
    WebRtc_WriteBuffer(aecpc->far_pre_buf_s16, farend_ptr,
                       (size_t) newNrOfSamples);
#endif
    
    for (i = 0; i < newNrOfSamples; i++) {
      tmp_farend[i] = (float) farend_ptr[i];
    }
    WebRtc_WriteBuffer(aecpc->far_pre_buf, farend_float,
                       (size_t) newNrOfSamples);

    
    while (WebRtc_available_read(aecpc->far_pre_buf) >= PART_LEN2) {
      
      WebRtc_ReadBuffer(aecpc->far_pre_buf, (void**) &farend_float, tmp_farend,
                        PART_LEN2);

      WebRtcAec_BufferFarendPartition(aecpc->aec, farend_float);

      
      WebRtc_MoveReadPtr(aecpc->far_pre_buf, -PART_LEN);
#ifdef WEBRTC_AEC_DEBUG_DUMP
      WebRtc_ReadBuffer(aecpc->far_pre_buf_s16, (void**) &farend_ptr, newFarend,
                        PART_LEN2);
      WebRtc_WriteBuffer(aecpc->aec->far_time_buf, &farend_ptr[PART_LEN], 1);
      WebRtc_MoveReadPtr(aecpc->far_pre_buf_s16, -PART_LEN);
#endif
    }

    return retVal;
}

WebRtc_Word32 WebRtcAec_Process(void *aecInst, const WebRtc_Word16 *nearend,
    const WebRtc_Word16 *nearendH, WebRtc_Word16 *out, WebRtc_Word16 *outH,
    WebRtc_Word16 nrOfSamples, WebRtc_Word16 msInSndCardBuf, WebRtc_Word32 skew)
{
    aecpc_t *aecpc = aecInst;
    WebRtc_Word32 retVal = 0;
    short i;
    short nBlocks10ms;
    short nFrames;
    
    const float minSkewEst = -0.5f;
    const float maxSkewEst = 1.0f;

    if (aecpc == NULL) {
        return -1;
    }

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
    }
    else if (msInSndCardBuf > 500) {
        msInSndCardBuf = 500;
        aecpc->lastError = AEC_BAD_PARAMETER_WARNING;
        retVal = -1;
    }
    
    msInSndCardBuf += 10;
    aecpc->msInSndCardBuf = msInSndCardBuf;

    if (aecpc->skewMode == kAecTrue) {
        if (aecpc->skewFrCtr < 25) {
            aecpc->skewFrCtr++;
        }
        else {
            retVal = WebRtcAec_GetSkew(aecpc->resampler, skew, &aecpc->skew);
            if (retVal == -1) {
                aecpc->skew = 0;
                aecpc->lastError = AEC_BAD_PARAMETER_WARNING;
            }

            aecpc->skew /= aecpc->sampFactor*nrOfSamples;

            if (aecpc->skew < 1.0e-3 && aecpc->skew > -1.0e-3) {
                aecpc->resample = kAecFalse;
            }
            else {
                aecpc->resample = kAecTrue;
            }

            if (aecpc->skew < minSkewEst) {
                aecpc->skew = minSkewEst;
            }
            else if (aecpc->skew > maxSkewEst) {
                aecpc->skew = maxSkewEst;
            }

#ifdef WEBRTC_AEC_DEBUG_DUMP
            (void)fwrite(&aecpc->skew, sizeof(aecpc->skew), 1, aecpc->skewFile);
#endif
        }
    }

    nFrames = nrOfSamples / FRAME_LEN;
    nBlocks10ms = nFrames / aecpc->aec->mult;

    if (aecpc->ECstartup) {
        if (nearend != out) {
            
            memcpy(out, nearend, sizeof(short) * nrOfSamples);
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
            }
            else {
                aecpc->counter = 0;
            }

            if (aecpc->counter * nBlocks10ms >= 6) {
                
                
                
                aecpc->bufSizeStart = WEBRTC_SPL_MIN((3 * aecpc->sum *
                  aecpc->aec->mult * 8) / (4 * aecpc->counter * PART_LEN),
                  kMaxBufSizeStart);
                
                aecpc->checkBuffSize = 0;
            }

            if (aecpc->checkBufSizeCtr * nBlocks10ms > 50) {
                
                
                aecpc->bufSizeStart = WEBRTC_SPL_MIN((aecpc->msInSndCardBuf *
                    aecpc->aec->mult * 3) / 40, kMaxBufSizeStart);
                aecpc->checkBuffSize = 0;
            }
        }

        
        if (!aecpc->checkBuffSize) {
            
            
            
            
            int overhead_elements = aecpc->aec->system_delay / PART_LEN -
                aecpc->bufSizeStart;
            if (overhead_elements == 0) {
                
                aecpc->ECstartup = 0;
            } else if (overhead_elements > 0) {
                
                
                
                
                
                WebRtcAec_MoveFarReadPtr(aecpc->aec, overhead_elements);

                
                aecpc->ECstartup = 0;
            }
        }
    } else {
        

        int out_elements = 0;

        EstBufDelay(aecpc);

        
        for (i = 0; i < nFrames; i++) {
            int16_t* out_ptr = NULL;
            int16_t out_tmp[FRAME_LEN];

            
            WebRtcAec_ProcessFrame(aecpc->aec,
                                   &nearend[FRAME_LEN * i],
                                   &nearendH[FRAME_LEN * i],
                                   aecpc->knownDelay);
            
            
            

            
            
            out_elements = (int) WebRtc_available_read(aecpc->aec->outFrBuf);
            if (out_elements < FRAME_LEN) {
                WebRtc_MoveReadPtr(aecpc->aec->outFrBuf,
                                   out_elements - FRAME_LEN);
                if (aecpc->sampFreq == 32000) {
                    WebRtc_MoveReadPtr(aecpc->aec->outFrBufH,
                                       out_elements - FRAME_LEN);
                }
            }

            
            WebRtc_ReadBuffer(aecpc->aec->outFrBuf, (void**) &out_ptr,
                              out_tmp, FRAME_LEN);
            memcpy(&out[FRAME_LEN * i], out_ptr, sizeof(int16_t) * FRAME_LEN);
            
            if (aecpc->sampFreq == 32000) {
                WebRtc_ReadBuffer(aecpc->aec->outFrBufH, (void**) &out_ptr,
                                  out_tmp, FRAME_LEN);
                memcpy(&outH[FRAME_LEN * i], out_ptr,
                       sizeof(int16_t) * FRAME_LEN);
            }
        }
    }

#ifdef WEBRTC_AEC_DEBUG_DUMP
    {
        int16_t far_buf_size_ms = (int16_t)(aecpc->aec->system_delay /
            (sampMsNb * aecpc->aec->mult));
        (void)fwrite(&far_buf_size_ms, 2, 1, aecpc->bufFile);
        (void)fwrite(&aecpc->knownDelay, sizeof(aecpc->knownDelay), 1,
                     aecpc->delayFile);
    }
#endif

    return retVal;
}

WebRtc_Word32 WebRtcAec_set_config(void *aecInst, AecConfig config)
{
    aecpc_t *aecpc = aecInst;

    if (aecpc == NULL) {
        return -1;
    }

    if (aecpc->initFlag != initCheck) {
        aecpc->lastError = AEC_UNINITIALIZED_ERROR;
        return -1;
    }

    if (config.skewMode != kAecFalse && config.skewMode != kAecTrue) {
        aecpc->lastError = AEC_BAD_PARAMETER_ERROR;
        return -1;
    }
    aecpc->skewMode = config.skewMode;

    if (config.nlpMode != kAecNlpConservative && config.nlpMode !=
            kAecNlpModerate && config.nlpMode != kAecNlpAggressive) {
        aecpc->lastError = AEC_BAD_PARAMETER_ERROR;
        return -1;
    }
    aecpc->nlpMode = config.nlpMode;
    aecpc->aec->targetSupp = targetSupp[aecpc->nlpMode];
    aecpc->aec->minOverDrive = minOverDrive[aecpc->nlpMode];

    if (config.metricsMode != kAecFalse && config.metricsMode != kAecTrue) {
        aecpc->lastError = AEC_BAD_PARAMETER_ERROR;
        return -1;
    }
    aecpc->aec->metricsMode = config.metricsMode;
    if (aecpc->aec->metricsMode == kAecTrue) {
        WebRtcAec_InitMetrics(aecpc->aec);
    }

  if (config.delay_logging != kAecFalse && config.delay_logging != kAecTrue) {
    aecpc->lastError = AEC_BAD_PARAMETER_ERROR;
    return -1;
  }
  aecpc->aec->delay_logging_enabled = config.delay_logging;
  if (aecpc->aec->delay_logging_enabled == kAecTrue) {
    memset(aecpc->aec->delay_histogram, 0, sizeof(aecpc->aec->delay_histogram));
  }

    return 0;
}

WebRtc_Word32 WebRtcAec_get_config(void *aecInst, AecConfig *config)
{
    aecpc_t *aecpc = aecInst;

    if (aecpc == NULL) {
        return -1;
    }

    if (config == NULL) {
        aecpc->lastError = AEC_NULL_POINTER_ERROR;
        return -1;
    }

    if (aecpc->initFlag != initCheck) {
        aecpc->lastError = AEC_UNINITIALIZED_ERROR;
        return -1;
    }

    config->nlpMode = aecpc->nlpMode;
    config->skewMode = aecpc->skewMode;
    config->metricsMode = aecpc->aec->metricsMode;
    config->delay_logging = aecpc->aec->delay_logging_enabled;

    return 0;
}

WebRtc_Word32 WebRtcAec_get_echo_status(void *aecInst, WebRtc_Word16 *status)
{
    aecpc_t *aecpc = aecInst;

    if (aecpc == NULL) {
        return -1;
    }

    if (status == NULL) {
        aecpc->lastError = AEC_NULL_POINTER_ERROR;
        return -1;
    }

    if (aecpc->initFlag != initCheck) {
        aecpc->lastError = AEC_UNINITIALIZED_ERROR;
        return -1;
    }

    *status = aecpc->aec->echoState;

    return 0;
}

WebRtc_Word32 WebRtcAec_GetMetrics(void *aecInst, AecMetrics *metrics)
{
    const float upweight = 0.7f;
    float dtmp;
    short stmp;
    aecpc_t *aecpc = aecInst;

    if (aecpc == NULL) {
        return -1;
    }

    if (metrics == NULL) {
        aecpc->lastError = AEC_NULL_POINTER_ERROR;
        return -1;
    }

    if (aecpc->initFlag != initCheck) {
        aecpc->lastError = AEC_UNINITIALIZED_ERROR;
        return -1;
    }

    
    metrics->erl.instant = (short) aecpc->aec->erl.instant;

    if ((aecpc->aec->erl.himean > offsetLevel) && (aecpc->aec->erl.average > offsetLevel)) {
    
        dtmp = upweight * aecpc->aec->erl.himean + (1 - upweight) * aecpc->aec->erl.average;
        metrics->erl.average = (short) dtmp;
    }
    else {
        metrics->erl.average = offsetLevel;
    }

    metrics->erl.max = (short) aecpc->aec->erl.max;

    if (aecpc->aec->erl.min < (offsetLevel * (-1))) {
        metrics->erl.min = (short) aecpc->aec->erl.min;
    }
    else {
        metrics->erl.min = offsetLevel;
    }

    
    metrics->erle.instant = (short) aecpc->aec->erle.instant;

    if ((aecpc->aec->erle.himean > offsetLevel) && (aecpc->aec->erle.average > offsetLevel)) {
        
        dtmp =  upweight * aecpc->aec->erle.himean + (1 - upweight) * aecpc->aec->erle.average;
        metrics->erle.average = (short) dtmp;
    }
    else {
        metrics->erle.average = offsetLevel;
    }

    metrics->erle.max = (short) aecpc->aec->erle.max;

    if (aecpc->aec->erle.min < (offsetLevel * (-1))) {
        metrics->erle.min = (short) aecpc->aec->erle.min;
    } else {
        metrics->erle.min = offsetLevel;
    }

    
    if ((metrics->erl.average > offsetLevel) && (metrics->erle.average > offsetLevel)) {
        stmp = metrics->erl.average + metrics->erle.average;
    }
    else {
        stmp = offsetLevel;
    }
    metrics->rerl.average = stmp;

    
    metrics->rerl.instant = stmp;
    metrics->rerl.max = stmp;
    metrics->rerl.min = stmp;

    
    metrics->aNlp.instant = (short) aecpc->aec->aNlp.instant;

    if ((aecpc->aec->aNlp.himean > offsetLevel) && (aecpc->aec->aNlp.average > offsetLevel)) {
        
        dtmp =  upweight * aecpc->aec->aNlp.himean + (1 - upweight) * aecpc->aec->aNlp.average;
        metrics->aNlp.average = (short) dtmp;
    }
    else {
        metrics->aNlp.average = offsetLevel;
    }

    metrics->aNlp.max = (short) aecpc->aec->aNlp.max;

    if (aecpc->aec->aNlp.min < (offsetLevel * (-1))) {
        metrics->aNlp.min = (short) aecpc->aec->aNlp.min;
    }
    else {
        metrics->aNlp.min = offsetLevel;
    }

    return 0;
}

int WebRtcAec_GetDelayMetrics(void* handle, int* median, int* std) {
  aecpc_t* self = handle;
  int i = 0;
  int delay_values = 0;
  int num_delay_values = 0;
  int my_median = 0;
  const int kMsPerBlock = (PART_LEN * 1000) / self->splitSampFreq;
  float l1_norm = 0;

  if (handle == NULL) {
    return -1;
  }
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
  if (self->aec->delay_logging_enabled == 0) {
    
    self->lastError = AEC_UNSUPPORTED_FUNCTION_ERROR;
    return -1;
  }

  
  for (i = 0; i < kHistorySizeBlocks; i++) {
    num_delay_values += self->aec->delay_histogram[i];
  }
  if (num_delay_values == 0) {
    
    
    
    *median = -1;
    *std = -1;
    return 0;
  }

  delay_values = num_delay_values >> 1; 
  
  for (i = 0; i < kHistorySizeBlocks; i++) {
    delay_values -= self->aec->delay_histogram[i];
    if (delay_values < 0) {
      my_median = i;
      break;
    }
  }
  
  *median = (my_median - kLookaheadBlocks) * kMsPerBlock;

  
  for (i = 0; i < kHistorySizeBlocks; i++) {
    l1_norm += (float) (fabs(i - my_median) * self->aec->delay_histogram[i]);
  }
  *std = (int) (l1_norm / (float) num_delay_values + 0.5f) * kMsPerBlock;

  
  memset(self->aec->delay_histogram, 0, sizeof(self->aec->delay_histogram));

  return 0;
}

WebRtc_Word32 WebRtcAec_get_error_code(void *aecInst)
{
    aecpc_t *aecpc = aecInst;

    if (aecpc == NULL) {
        return -1;
    }

    return aecpc->lastError;
}

static int EstBufDelay(aecpc_t* aecpc) {
  int nSampSndCard = aecpc->msInSndCardBuf * sampMsNb * aecpc->aec->mult;
  int current_delay = nSampSndCard - aecpc->aec->system_delay;
  int delay_difference = 0;

  
  
  
  
  

  
  current_delay += FRAME_LEN * aecpc->aec->mult;

  
  if (aecpc->skewMode == kAecTrue && aecpc->resample == kAecTrue) {
    current_delay -= kResamplingDelay;
  }

  
  if (current_delay < PART_LEN) {
    current_delay += WebRtcAec_MoveFarReadPtr(aecpc->aec, 1) * PART_LEN;
  }

  aecpc->filtDelay = WEBRTC_SPL_MAX(0, (short) (0.8 * aecpc->filtDelay +
          0.2 * current_delay));

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
    aecpc->knownDelay = WEBRTC_SPL_MAX((int) aecpc->filtDelay - 160, 0);
  }

  return 0;
}
