


















#include <stdlib.h>
#include <string.h>

#include "structs.h"
#include "codec.h"
#include "pitch_estimator.h"
#include "entropy_coding.h"
#include "arith_routines.h"
#include "pitch_gain_tables.h"
#include "pitch_lag_tables.h"
#include "spectrum_ar_model_tables.h"
#include "lpc_tables.h"
#include "lpc_analysis.h"
#include "bandwidth_estimator.h"
#include "lpc_shape_swb12_tables.h"
#include "lpc_shape_swb16_tables.h"
#include "lpc_gain_swb_tables.h"


#define UB_LOOKAHEAD 24































static const WebRtc_Word16 kLowerBandBitRate12[7] = {
  29000, 30000, 30000, 31000, 31000, 32000, 32000};
static const WebRtc_Word16 kUpperBandBitRate12[7] = {
  25000, 25000, 27000, 27000, 29000, 29000, 32000};


static const WebRtc_Word16 kLowerBandBitRate16[6] = {
  31000, 31000, 32000, 32000, 32000, 32000};
static const WebRtc_Word16 kUpperBandBitRate16[6] = {
  28000, 29000, 29000, 30000, 31000, 32000};



















WebRtc_Word16
WebRtcIsac_RateAllocation(
    WebRtc_Word32         inRateBitPerSec,
    double*             rateLBBitPerSec,
    double*             rateUBBitPerSec,
    enum ISACBandwidth* bandwidthKHz)
{
  WebRtc_Word16 idx;
  double idxD;
  double idxErr;
  if(inRateBitPerSec < 38000)
  {
    
    
    
    *rateLBBitPerSec = (WebRtc_Word16)((inRateBitPerSec > 32000)?
                                       32000:inRateBitPerSec);
    *rateUBBitPerSec = 0;
    *bandwidthKHz = isac8kHz;
  }
  else if((inRateBitPerSec >= 38000) && (inRateBitPerSec < 50000))
  {
    
    
    

    
    
    const double stepSizeInv = 8.5714286e-4;
    idxD = (inRateBitPerSec - 38000) * stepSizeInv;
    idx = (idxD >= 6)? 6:((WebRtc_Word16)idxD);
    idxErr = idxD - idx;
    *rateLBBitPerSec = kLowerBandBitRate12[idx];
    *rateUBBitPerSec = kUpperBandBitRate12[idx];

    if(idx < 6)
    {
      *rateLBBitPerSec += (WebRtc_Word16)(idxErr *
                                          (kLowerBandBitRate12[idx + 1] -
                                           kLowerBandBitRate12[idx]));
      *rateUBBitPerSec += (WebRtc_Word16)(idxErr *
                                          (kUpperBandBitRate12[idx + 1] -
                                           kUpperBandBitRate12[idx]));
    }

    *bandwidthKHz = isac12kHz;
  }
  else if((inRateBitPerSec >= 50000) && (inRateBitPerSec <= 56000))
  {
    
    
    

    
    
    const double stepSizeInv = 8.3333333e-4;
    idxD = (inRateBitPerSec - 50000) * stepSizeInv;
    idx = (idxD >= 5)? 5:((WebRtc_Word16)idxD);
    idxErr = idxD - idx;
    *rateLBBitPerSec = kLowerBandBitRate16[idx];
    *rateUBBitPerSec  = kUpperBandBitRate16[idx];

    if(idx < 5)
    {
      *rateLBBitPerSec += (WebRtc_Word16)(idxErr *
                                          (kLowerBandBitRate16[idx + 1] -
                                           kLowerBandBitRate16[idx]));

      *rateUBBitPerSec += (WebRtc_Word16)(idxErr *
                                          (kUpperBandBitRate16[idx + 1] -
                                           kUpperBandBitRate16[idx]));
    }

    *bandwidthKHz = isac16kHz;
  }
  else
  {
    
    return -1;
  }

  
  *rateLBBitPerSec = (*rateLBBitPerSec > 32000)? 32000:*rateLBBitPerSec;
  *rateUBBitPerSec = (*rateUBBitPerSec > 32000)? 32000:*rateUBBitPerSec;

  return 0;
}



int
WebRtcIsac_EncodeLb(
    float*           in,
    ISACLBEncStruct* ISACencLB_obj,
    WebRtc_Word16      codingMode,
    WebRtc_Word16      bottleneckIndex)
{
  int stream_length = 0;
  int err;
  int k;
  int iterCntr;

  double lofilt_coef[(ORDERLO+1)*SUBFRAMES];
  double hifilt_coef[(ORDERHI+1)*SUBFRAMES];
  float LP[FRAMESAMPLES_HALF];
  float HP[FRAMESAMPLES_HALF];

  double LP_lookahead[FRAMESAMPLES_HALF];
  double HP_lookahead[FRAMESAMPLES_HALF];
  double LP_lookahead_pf[FRAMESAMPLES_HALF + QLOOKAHEAD];
  double LPw[FRAMESAMPLES_HALF];

  double HPw[FRAMESAMPLES_HALF];
  double LPw_pf[FRAMESAMPLES_HALF];
  WebRtc_Word16 fre[FRAMESAMPLES_HALF];   
  WebRtc_Word16 fim[FRAMESAMPLES_HALF];   

  double PitchLags[4];
  double PitchGains[4];
  WebRtc_Word16 PitchGains_Q12[4];
  WebRtc_Word16 AvgPitchGain_Q12;

  int frame_mode; 
  int processed_samples, status = 0;

  double bits_gains;
  int bmodel;

  transcode_obj transcodingParam;
  double bytesLeftSpecCoding;
  WebRtc_UWord16 payloadLimitBytes;

  

  if (ISACencLB_obj->buffer_index == 0) {
    
    ISACencLB_obj->current_framesamples = ISACencLB_obj->new_framelength;
  }
  
  frame_mode = ISACencLB_obj->current_framesamples/MAX_FRAMESAMPLES;
  
  processed_samples = ISACencLB_obj->current_framesamples/(frame_mode+1);

  
  
  

  
  for (k = 0; k < FRAMESAMPLES_10ms; k++) {
    ISACencLB_obj->data_buffer_float[k + ISACencLB_obj->buffer_index] =
        in[k];
  }

  

  if (ISACencLB_obj->buffer_index + FRAMESAMPLES_10ms != processed_samples) {
    ISACencLB_obj->buffer_index += FRAMESAMPLES_10ms;
    return 0;
  }
  

  ISACencLB_obj->buffer_index = 0;

  
  

  
  

  if (frame_mode == 0 || ISACencLB_obj->frame_nb == 0 ) {
    
    
    int intVar;
    
    ISACencLB_obj->bitstr_obj.W_upper = 0xFFFFFFFF;
    ISACencLB_obj->bitstr_obj.streamval = 0;
    ISACencLB_obj->bitstr_obj.stream_index = 0;

    if((codingMode == 0) && (frame_mode == 0) &&
       (ISACencLB_obj->enforceFrameSize == 0)) {
      ISACencLB_obj->new_framelength =
          WebRtcIsac_GetNewFrameLength(ISACencLB_obj->bottleneck,
                                       ISACencLB_obj->current_framesamples);
    }

    ISACencLB_obj->s2nr = WebRtcIsac_GetSnr(
        ISACencLB_obj->bottleneck, ISACencLB_obj->current_framesamples);

    
    status = WebRtcIsac_EncodeFrameLen(
        ISACencLB_obj->current_framesamples, &ISACencLB_obj->bitstr_obj);
    if (status < 0) {
      
      return status;
    }
    
    ISACencLB_obj->SaveEnc_obj.framelength =
        ISACencLB_obj->current_framesamples;

    
    ISACencLB_obj->lastBWIdx = bottleneckIndex;
    intVar = (int)bottleneckIndex;
    WebRtcIsac_EncodeReceiveBw(&intVar, &ISACencLB_obj->bitstr_obj);
  }

  
  WebRtcIsac_SplitAndFilterFloat(ISACencLB_obj->data_buffer_float, LP, HP,
                                 LP_lookahead, HP_lookahead,    &ISACencLB_obj->prefiltbankstr_obj );

  
  WebRtcIsac_PitchAnalysis(LP_lookahead, LP_lookahead_pf,
                           &ISACencLB_obj->pitchanalysisstr_obj, PitchLags, PitchGains);

  

  
  for (k=0;k<PITCH_SUBFRAMES;k++) {
    PitchGains_Q12[k] = (WebRtc_Word16)(PitchGains[k] * 4096.0);
  }

  
  if (frame_mode == 0 || ISACencLB_obj->frame_nb == 0)
  {
    ISACencLB_obj->SaveEnc_obj.startIdx = 0;
  } else {
    ISACencLB_obj->SaveEnc_obj.startIdx = 1;
  }

  
  WebRtcIsac_EncodePitchGain(PitchGains_Q12, &ISACencLB_obj->bitstr_obj,
                             &ISACencLB_obj->SaveEnc_obj);
  WebRtcIsac_EncodePitchLag(PitchLags, PitchGains_Q12,
                            &ISACencLB_obj->bitstr_obj, &ISACencLB_obj->SaveEnc_obj);

  AvgPitchGain_Q12 = (PitchGains_Q12[0] + PitchGains_Q12[1] +
                      PitchGains_Q12[2] + PitchGains_Q12[3])>>2;

  
  WebRtcIsac_GetLpcCoefLb(LP_lookahead_pf, HP_lookahead,
                          &ISACencLB_obj->maskfiltstr_obj, ISACencLB_obj->s2nr,
                          PitchGains_Q12, lofilt_coef, hifilt_coef);

  
  WebRtcIsac_EncodeLpcLb(lofilt_coef, hifilt_coef,  &bmodel, &bits_gains,
                         &ISACencLB_obj->bitstr_obj, &ISACencLB_obj->SaveEnc_obj);

  
  for (k = 0; k < 4; k++) {
    PitchGains[k] = ((float)PitchGains_Q12[k])/4096;
  }

  
  transcodingParam.W_upper      = ISACencLB_obj->bitstr_obj.W_upper;
  transcodingParam.stream_index = ISACencLB_obj->bitstr_obj.stream_index;
  transcodingParam.streamval    = ISACencLB_obj->bitstr_obj.streamval;
  transcodingParam.stream[0]    = ISACencLB_obj->bitstr_obj.stream[
      ISACencLB_obj->bitstr_obj.stream_index - 2];
  transcodingParam.stream[1]    = ISACencLB_obj->bitstr_obj.stream[
      ISACencLB_obj->bitstr_obj.stream_index - 1];
  transcodingParam.stream[2]    = ISACencLB_obj->bitstr_obj.stream[
      ISACencLB_obj->bitstr_obj.stream_index];

  
  for(k = 0; k < SUBFRAMES; k++) {
    transcodingParam.loFiltGain[k] = lofilt_coef[(LPC_LOBAND_ORDER+1)*k];
    transcodingParam.hiFiltGain[k] = hifilt_coef[(LPC_HIBAND_ORDER+1)*k];
  }

  
  WebRtcIsac_EncodeLpcGainLb(lofilt_coef, hifilt_coef,  bmodel,
                             &ISACencLB_obj->bitstr_obj, &ISACencLB_obj->SaveEnc_obj);

  

  if((frame_mode == 1) && (ISACencLB_obj->frame_nb == 0)) {
    

    payloadLimitBytes = ISACencLB_obj->payloadLimitBytes60 >> 1;
  }
  else if (frame_mode == 0) {
    
    
    payloadLimitBytes = ISACencLB_obj->payloadLimitBytes30 - 3;
  } else {
    
    
    payloadLimitBytes = ISACencLB_obj->payloadLimitBytes60 - 3;
  }
  bytesLeftSpecCoding = payloadLimitBytes - transcodingParam.stream_index;

  
  
  WebRtcIsac_NormLatticeFilterMa(ORDERLO,
                                  ISACencLB_obj->maskfiltstr_obj.PreStateLoF,
                                  ISACencLB_obj->maskfiltstr_obj.PreStateLoG, LP, lofilt_coef, LPw);
  
  WebRtcIsac_NormLatticeFilterMa(ORDERHI,
                                  ISACencLB_obj->maskfiltstr_obj.PreStateHiF,
                                  ISACencLB_obj->maskfiltstr_obj.PreStateHiG, HP, hifilt_coef, HPw);


  
  WebRtcIsac_PitchfilterPre(LPw, LPw_pf, &ISACencLB_obj->pitchfiltstr_obj,
                             PitchLags, PitchGains);

  
  WebRtcIsac_Time2Spec(LPw_pf, HPw, fre, fim, &ISACencLB_obj->fftstr_obj);


  
  for (k = 0; k < FRAMESAMPLES_HALF; k++) {
    ISACencLB_obj->SaveEnc_obj.fre[k +
                                   ISACencLB_obj->SaveEnc_obj.startIdx*FRAMESAMPLES_HALF] = fre[k];
    ISACencLB_obj->SaveEnc_obj.fim[k +
                                   ISACencLB_obj->SaveEnc_obj.startIdx*FRAMESAMPLES_HALF] = fim[k];
  }
  ISACencLB_obj->SaveEnc_obj.AvgPitchGain[
      ISACencLB_obj->SaveEnc_obj.startIdx] = AvgPitchGain_Q12;

  
  err = WebRtcIsac_EncodeSpecLb(fre, fim, &ISACencLB_obj->bitstr_obj,
                                AvgPitchGain_Q12);
  if ((err < 0) && (err != -ISAC_DISALLOWED_BITSTREAM_LENGTH)) {
    

    if (frame_mode == 1 && ISACencLB_obj->frame_nb == 1) {
      

      ISACencLB_obj->frame_nb = 0;
    }
    return err;
  }
  iterCntr = 0;
  while((ISACencLB_obj->bitstr_obj.stream_index > payloadLimitBytes) ||
        (err == -ISAC_DISALLOWED_BITSTREAM_LENGTH)) {
    double bytesSpecCoderUsed;
    double transcodeScale;

    if(iterCntr >= MAX_PAYLOAD_LIMIT_ITERATION) {
      
      if((frame_mode == 1) && (ISACencLB_obj->frame_nb == 0)) {
        



        ISACencLB_obj->frame_nb = 1;
        return 0;
      } else if((frame_mode == 1) && (ISACencLB_obj->frame_nb == 1)) {
        ISACencLB_obj->frame_nb = 0;
      }

      if(err != -ISAC_DISALLOWED_BITSTREAM_LENGTH) {
        return -ISAC_PAYLOAD_LARGER_THAN_LIMIT;
      } else {
        return status;
      }
    }

    if(err == -ISAC_DISALLOWED_BITSTREAM_LENGTH) {
      bytesSpecCoderUsed = STREAM_SIZE_MAX;
      
      transcodeScale = bytesLeftSpecCoding / bytesSpecCoderUsed * 0.5;
    } else {
      bytesSpecCoderUsed = ISACencLB_obj->bitstr_obj.stream_index -
          transcodingParam.stream_index;
      transcodeScale = bytesLeftSpecCoding / bytesSpecCoderUsed;
    }

    

    transcodeScale *= (1.0 - (0.9 * (double)iterCntr /
                              (double)MAX_PAYLOAD_LIMIT_ITERATION));

    
    for (k = 0; k < SUBFRAMES; k++) {
      lofilt_coef[(LPC_LOBAND_ORDER+1) * k] =
          transcodingParam.loFiltGain[k] * transcodeScale;
      hifilt_coef[(LPC_HIBAND_ORDER+1) * k] =
          transcodingParam.hiFiltGain[k] * transcodeScale;
      transcodingParam.loFiltGain[k] =
          lofilt_coef[(LPC_LOBAND_ORDER+1) * k];
      transcodingParam.hiFiltGain[k] =
          hifilt_coef[(LPC_HIBAND_ORDER+1) * k];
    }

    
    for (k = 0; k < FRAMESAMPLES_HALF; k++) {
      fre[k] = (WebRtc_Word16)(fre[k] * transcodeScale);
      fim[k] = (WebRtc_Word16)(fim[k] * transcodeScale);
    }

    
    for (k = 0; k < FRAMESAMPLES_HALF; k++) {
      ISACencLB_obj->SaveEnc_obj.fre[k +
                                     ISACencLB_obj->SaveEnc_obj.startIdx * FRAMESAMPLES_HALF] =
          fre[k];
      ISACencLB_obj->SaveEnc_obj.fim[k +
                                     ISACencLB_obj->SaveEnc_obj.startIdx * FRAMESAMPLES_HALF] =
          fim[k];
    }

    
    ISACencLB_obj->bitstr_obj.W_upper = transcodingParam.W_upper;
    ISACencLB_obj->bitstr_obj.stream_index = transcodingParam.stream_index;
    ISACencLB_obj->bitstr_obj.streamval = transcodingParam.streamval;
    ISACencLB_obj->bitstr_obj.stream[transcodingParam.stream_index - 2] =
        transcodingParam.stream[0];
    ISACencLB_obj->bitstr_obj.stream[transcodingParam.stream_index - 1] =
        transcodingParam.stream[1];
    ISACencLB_obj->bitstr_obj.stream[transcodingParam.stream_index] =
        transcodingParam.stream[2];

    
    WebRtcIsac_EncodeLpcGainLb(lofilt_coef, hifilt_coef,  bmodel,
                               &ISACencLB_obj->bitstr_obj, &ISACencLB_obj->SaveEnc_obj);

    
    bytesLeftSpecCoding = payloadLimitBytes -
        transcodingParam.stream_index;

    
    err = WebRtcIsac_EncodeSpecLb(fre, fim, &ISACencLB_obj->bitstr_obj,
                                  AvgPitchGain_Q12);
    if((err < 0) && (err != -ISAC_DISALLOWED_BITSTREAM_LENGTH)) {
      

      if (frame_mode == 1 && ISACencLB_obj->frame_nb == 1) {
        

        ISACencLB_obj->frame_nb = 0;
      }
      return err;
    }
    iterCntr++;
  }

  
  
  if (frame_mode == 1)
  {
    if(ISACencLB_obj->frame_nb == 0)
    {
      ISACencLB_obj->frame_nb = 1;
      return 0;
    }
    else if(ISACencLB_obj->frame_nb == 1)
    {
      ISACencLB_obj->frame_nb = 0;
      

      if (codingMode == 0 && (ISACencLB_obj->enforceFrameSize == 0))
      {
        ISACencLB_obj->new_framelength =
            WebRtcIsac_GetNewFrameLength(ISACencLB_obj->bottleneck,
                                         ISACencLB_obj->current_framesamples);
      }
    }
  }
  else
  {
    ISACencLB_obj->frame_nb = 0;
  }

  
  stream_length = WebRtcIsac_EncTerminate(&ISACencLB_obj->bitstr_obj);

  return stream_length;
}

int
WebRtcIsac_EncodeUb16(
    float*           in,
    ISACUBEncStruct* ISACencUB_obj,
    WebRtc_Word32      jitterInfo)
{
  int err;
  int k;

  double lpcVecs[UB_LPC_ORDER * UB16_LPC_VEC_PER_FRAME];
  double percepFilterParams[(1 + UB_LPC_ORDER) * (SUBFRAMES<<1) +
                            (1 + UB_LPC_ORDER)];

  double LP_lookahead[FRAMESAMPLES];
  WebRtc_Word16 fre[FRAMESAMPLES_HALF];   
  WebRtc_Word16 fim[FRAMESAMPLES_HALF];   

  int status = 0;

  double varscale[2];
  double corr[SUBFRAMES<<1][UB_LPC_ORDER + 1];
  double lpcGains[SUBFRAMES<<1];
  transcode_obj transcodingParam;
  double bytesLeftSpecCoding;
  WebRtc_UWord16 payloadLimitBytes;
  WebRtc_UWord16 iterCntr;
  double s2nr;

  
  
  

  
  for (k = 0; k < FRAMESAMPLES_10ms; k++) {
    ISACencUB_obj->data_buffer_float[k + ISACencUB_obj->buffer_index] =
        in[k];
  }

  

  if (ISACencUB_obj->buffer_index + FRAMESAMPLES_10ms < FRAMESAMPLES) {
    ISACencUB_obj->buffer_index += FRAMESAMPLES_10ms;
    return 0;
  }

  
  

  
  

  
  ISACencUB_obj->bitstr_obj.W_upper = 0xFFFFFFFF;
  ISACencUB_obj->bitstr_obj.streamval = 0;
  ISACencUB_obj->bitstr_obj.stream_index = 0;

  
  
  WebRtcIsac_EncodeJitterInfo(jitterInfo, &ISACencUB_obj->bitstr_obj);

  status = WebRtcIsac_EncodeBandwidth(isac16kHz,
                                      &ISACencUB_obj->bitstr_obj);
  if (status < 0) {
    return status;
  }

  s2nr = WebRtcIsac_GetSnr(ISACencUB_obj->bottleneck,
                                 FRAMESAMPLES);

  memcpy(lpcVecs, ISACencUB_obj->lastLPCVec, UB_LPC_ORDER * sizeof(double));

  for (k = 0; k < FRAMESAMPLES; k++) {
    LP_lookahead[k] = ISACencUB_obj->data_buffer_float[UB_LOOKAHEAD + k];
  }

  
  WebRtcIsac_GetLpcCoefUb(LP_lookahead, &ISACencUB_obj->maskfiltstr_obj,
                            &lpcVecs[UB_LPC_ORDER], corr, varscale, isac16kHz);

  memcpy(ISACencUB_obj->lastLPCVec,
         &lpcVecs[(UB16_LPC_VEC_PER_FRAME - 1) * (UB_LPC_ORDER)],
         sizeof(double) * UB_LPC_ORDER);

  
  WebRtcIsac_EncodeLpcUB(lpcVecs, &ISACencUB_obj->bitstr_obj,
                           percepFilterParams, isac16kHz, &ISACencUB_obj->SaveEnc_obj);


  
  
  WebRtcIsac_GetLpcGain(s2nr, &percepFilterParams[UB_LPC_ORDER + 1],
                       (SUBFRAMES<<1), lpcGains, corr, varscale);

  
  transcodingParam.stream_index = ISACencUB_obj->bitstr_obj.stream_index;
  transcodingParam.W_upper      = ISACencUB_obj->bitstr_obj.W_upper;
  transcodingParam.streamval    = ISACencUB_obj->bitstr_obj.streamval;
  transcodingParam.stream[0]    = ISACencUB_obj->bitstr_obj.stream[
      ISACencUB_obj->bitstr_obj.stream_index - 2];
  transcodingParam.stream[1]    = ISACencUB_obj->bitstr_obj.stream[
      ISACencUB_obj->bitstr_obj.stream_index - 1];
  transcodingParam.stream[2]    = ISACencUB_obj->bitstr_obj.stream[
      ISACencUB_obj->bitstr_obj.stream_index];

  
  for(k = 0; k < SUBFRAMES; k++) {
    transcodingParam.loFiltGain[k] = lpcGains[k];
    transcodingParam.hiFiltGain[k] = lpcGains[SUBFRAMES + k];
  }

  
  memcpy(ISACencUB_obj->SaveEnc_obj.lpcGain, lpcGains, (SUBFRAMES << 1) * sizeof(double));

  WebRtcIsac_EncodeLpcGainUb(lpcGains, &ISACencUB_obj->bitstr_obj,
                             ISACencUB_obj->SaveEnc_obj.lpcGainIndex);
  WebRtcIsac_EncodeLpcGainUb(&lpcGains[SUBFRAMES], &ISACencUB_obj->bitstr_obj,
                             &ISACencUB_obj->SaveEnc_obj.lpcGainIndex[SUBFRAMES]);

  


  payloadLimitBytes = ISACencUB_obj->maxPayloadSizeBytes -
      ISACencUB_obj->numBytesUsed - 3;
  bytesLeftSpecCoding = payloadLimitBytes -
      ISACencUB_obj->bitstr_obj.stream_index;

  for (k = 0; k < (SUBFRAMES<<1); k++) {
    percepFilterParams[k*(UB_LPC_ORDER + 1) + (UB_LPC_ORDER + 1)] =
        lpcGains[k];
  }

  
  
  WebRtcIsac_NormLatticeFilterMa(UB_LPC_ORDER,
                                  ISACencUB_obj->maskfiltstr_obj.PreStateLoF,
                                  ISACencUB_obj->maskfiltstr_obj.PreStateLoG,
                                  &ISACencUB_obj->data_buffer_float[0],
                                  &percepFilterParams[UB_LPC_ORDER + 1],
                                  &LP_lookahead[0]);

  
  WebRtcIsac_NormLatticeFilterMa(UB_LPC_ORDER,
                                  ISACencUB_obj->maskfiltstr_obj.PreStateLoF,
                                  ISACencUB_obj->maskfiltstr_obj.PreStateLoG,
                                  &ISACencUB_obj->data_buffer_float[FRAMESAMPLES_HALF],
                                  &percepFilterParams[(UB_LPC_ORDER + 1) + SUBFRAMES *
                                                      (UB_LPC_ORDER + 1)], &LP_lookahead[FRAMESAMPLES_HALF]);

  WebRtcIsac_Time2Spec(&LP_lookahead[0], &LP_lookahead[FRAMESAMPLES_HALF],
                      fre, fim, &ISACencUB_obj->fftstr_obj);

  
  memcpy(&ISACencUB_obj->SaveEnc_obj.realFFT, fre,
         FRAMESAMPLES_HALF * sizeof(WebRtc_Word16));

  memcpy(&ISACencUB_obj->SaveEnc_obj.imagFFT, fim,
         FRAMESAMPLES_HALF * sizeof(WebRtc_Word16));

  
  
  memcpy(ISACencUB_obj->data_buffer_float,
         &ISACencUB_obj->data_buffer_float[FRAMESAMPLES],
         LB_TOTAL_DELAY_SAMPLES * sizeof(float));
  
  
  ISACencUB_obj->buffer_index = LB_TOTAL_DELAY_SAMPLES;

  
  memcpy(&ISACencUB_obj->SaveEnc_obj.bitStreamObj,
         &ISACencUB_obj->bitstr_obj, sizeof(Bitstr));

  
  err = WebRtcIsac_EncodeSpecUB16(fre, fim, &ISACencUB_obj->bitstr_obj);
  if ((err < 0) && (err != -ISAC_DISALLOWED_BITSTREAM_LENGTH)) {
    return err;
  }

  iterCntr = 0;
  while((ISACencUB_obj->bitstr_obj.stream_index > payloadLimitBytes) ||
        (err == -ISAC_DISALLOWED_BITSTREAM_LENGTH)) {
    double bytesSpecCoderUsed;
    double transcodeScale;

    if (iterCntr >= MAX_PAYLOAD_LIMIT_ITERATION) {
      
      return -ISAC_PAYLOAD_LARGER_THAN_LIMIT;
    }

    if (err == -ISAC_DISALLOWED_BITSTREAM_LENGTH) {
      bytesSpecCoderUsed = STREAM_SIZE_MAX;
      
      transcodeScale = bytesLeftSpecCoding / bytesSpecCoderUsed * 0.5;
    } else {
      bytesSpecCoderUsed = ISACencUB_obj->bitstr_obj.stream_index -
          transcodingParam.stream_index;
      transcodeScale = bytesLeftSpecCoding / bytesSpecCoderUsed;
    }

    

    transcodeScale *= (1.0 - (0.9 * (double)iterCntr/
                              (double)MAX_PAYLOAD_LIMIT_ITERATION));

    
    for (k = 0; k < SUBFRAMES; k++) {
      transcodingParam.loFiltGain[k] *= transcodeScale;
      transcodingParam.hiFiltGain[k] *= transcodeScale;
    }

    
    for (k = 0; k < FRAMESAMPLES_HALF; k++) {
      fre[k] = (WebRtc_Word16)(fre[k] * transcodeScale + 0.5);
      fim[k] = (WebRtc_Word16)(fim[k] * transcodeScale + 0.5);
    }

    
    memcpy(&ISACencUB_obj->SaveEnc_obj.realFFT, fre,
           FRAMESAMPLES_HALF * sizeof(WebRtc_Word16));

    memcpy(&ISACencUB_obj->SaveEnc_obj.imagFFT, fim,
           FRAMESAMPLES_HALF * sizeof(WebRtc_Word16));


    
    ISACencUB_obj->bitstr_obj.W_upper = transcodingParam.W_upper;

    ISACencUB_obj->bitstr_obj.stream_index = transcodingParam.stream_index;

    ISACencUB_obj->bitstr_obj.streamval = transcodingParam.streamval;

    ISACencUB_obj->bitstr_obj.stream[transcodingParam.stream_index - 2] =
        transcodingParam.stream[0];

    ISACencUB_obj->bitstr_obj.stream[transcodingParam.stream_index - 1] =
        transcodingParam.stream[1];

    ISACencUB_obj->bitstr_obj.stream[transcodingParam.stream_index] =
        transcodingParam.stream[2];

    
    memcpy(ISACencUB_obj->SaveEnc_obj.lpcGain, lpcGains,
           (SUBFRAMES << 1) * sizeof(double));

    WebRtcIsac_EncodeLpcGainUb(transcodingParam.loFiltGain,
                               &ISACencUB_obj->bitstr_obj,
                               ISACencUB_obj->SaveEnc_obj.lpcGainIndex);
    WebRtcIsac_EncodeLpcGainUb(transcodingParam.hiFiltGain,
                               &ISACencUB_obj->bitstr_obj,
                               &ISACencUB_obj->SaveEnc_obj.lpcGainIndex[SUBFRAMES]);

    
    bytesLeftSpecCoding = payloadLimitBytes -
        ISACencUB_obj->bitstr_obj.stream_index;

    
    memcpy(&ISACencUB_obj->SaveEnc_obj.bitStreamObj,
           &ISACencUB_obj->bitstr_obj, sizeof(Bitstr));

    
    err = WebRtcIsac_EncodeSpecUB16(fre, fim, &ISACencUB_obj->bitstr_obj);
    if ((err < 0) && (err != -ISAC_DISALLOWED_BITSTREAM_LENGTH)) {
      

      return err;
    }
    iterCntr++;
  }

  
  return WebRtcIsac_EncTerminate(&ISACencUB_obj->bitstr_obj);
}


int
WebRtcIsac_EncodeUb12(
    float*           in,
    ISACUBEncStruct* ISACencUB_obj,
    WebRtc_Word32      jitterInfo)
{
  int err;
  int k;
  int iterCntr;

  double lpcVecs[UB_LPC_ORDER * UB_LPC_VEC_PER_FRAME];

  double percepFilterParams[(1 + UB_LPC_ORDER) * SUBFRAMES];
  float LP[FRAMESAMPLES_HALF];
  float HP[FRAMESAMPLES_HALF];

  double LP_lookahead[FRAMESAMPLES_HALF];
  double HP_lookahead[FRAMESAMPLES_HALF];
  double LPw[FRAMESAMPLES_HALF];

  double HPw[FRAMESAMPLES_HALF];
  WebRtc_Word16 fre[FRAMESAMPLES_HALF];   
  WebRtc_Word16 fim[FRAMESAMPLES_HALF];   

  int status = 0;

  double varscale[1];

  double corr[UB_LPC_GAIN_DIM][UB_LPC_ORDER + 1];
  double lpcGains[SUBFRAMES];
  transcode_obj transcodingParam;
  double bytesLeftSpecCoding;
  WebRtc_UWord16 payloadLimitBytes;
  double s2nr;

  
  
  

  
  for (k=0; k<FRAMESAMPLES_10ms; k++) {
    ISACencUB_obj->data_buffer_float[k + ISACencUB_obj->buffer_index] =
        in[k];
  }

  

  if (ISACencUB_obj->buffer_index + FRAMESAMPLES_10ms < FRAMESAMPLES) {
    ISACencUB_obj->buffer_index += FRAMESAMPLES_10ms;
    return 0;
  }
  

  ISACencUB_obj->buffer_index = 0;

  
  

  
  

  
  ISACencUB_obj->bitstr_obj.W_upper = 0xFFFFFFFF;
  ISACencUB_obj->bitstr_obj.streamval = 0;
  ISACencUB_obj->bitstr_obj.stream_index = 0;

  
  
  WebRtcIsac_EncodeJitterInfo(jitterInfo, &ISACencUB_obj->bitstr_obj);

  status = WebRtcIsac_EncodeBandwidth(isac12kHz,
                                      &ISACencUB_obj->bitstr_obj);
  if (status < 0) {
    return status;
  }


  s2nr = WebRtcIsac_GetSnr(ISACencUB_obj->bottleneck,
                                 FRAMESAMPLES);

  
  WebRtcIsac_SplitAndFilterFloat(ISACencUB_obj->data_buffer_float, HP, LP,
                                 HP_lookahead, LP_lookahead, &ISACencUB_obj->prefiltbankstr_obj);

  
  WebRtcIsac_GetLpcCoefUb(LP_lookahead, &ISACencUB_obj->maskfiltstr_obj,
                            lpcVecs, corr, varscale, isac12kHz);

  
  WebRtcIsac_EncodeLpcUB(lpcVecs, &ISACencUB_obj->bitstr_obj,
                           percepFilterParams, isac12kHz, &ISACencUB_obj->SaveEnc_obj);

  WebRtcIsac_GetLpcGain(s2nr, percepFilterParams, SUBFRAMES, lpcGains,
                       corr, varscale);
  
  
  transcodingParam.W_upper = ISACencUB_obj->bitstr_obj.W_upper;

  transcodingParam.stream_index = ISACencUB_obj->bitstr_obj.stream_index;

  transcodingParam.streamval = ISACencUB_obj->bitstr_obj.streamval;

  transcodingParam.stream[0] = ISACencUB_obj->bitstr_obj.stream[
      ISACencUB_obj->bitstr_obj.stream_index - 2];

  transcodingParam.stream[1] = ISACencUB_obj->bitstr_obj.stream[
      ISACencUB_obj->bitstr_obj.stream_index - 1];

  transcodingParam.stream[2] = ISACencUB_obj->bitstr_obj.stream[
      ISACencUB_obj->bitstr_obj.stream_index];

  
  for(k = 0; k < SUBFRAMES; k++) {
    transcodingParam.loFiltGain[k] = lpcGains[k];
  }

  
  memcpy(ISACencUB_obj->SaveEnc_obj.lpcGain, lpcGains, SUBFRAMES *
         sizeof(double));

  WebRtcIsac_EncodeLpcGainUb(lpcGains, &ISACencUB_obj->bitstr_obj,
                             ISACencUB_obj->SaveEnc_obj.lpcGainIndex);

  for(k = 0; k < SUBFRAMES; k++) {
    percepFilterParams[k*(UB_LPC_ORDER + 1)] = lpcGains[k];
  }

  
  
  WebRtcIsac_NormLatticeFilterMa(UB_LPC_ORDER,
                                  ISACencUB_obj->maskfiltstr_obj.PreStateLoF,
                                  ISACencUB_obj->maskfiltstr_obj.PreStateLoG, LP, percepFilterParams,
                                  LPw);

  


  payloadLimitBytes = ISACencUB_obj->maxPayloadSizeBytes -
      ISACencUB_obj->numBytesUsed - 3;
  bytesLeftSpecCoding = payloadLimitBytes -
      ISACencUB_obj->bitstr_obj.stream_index;

  memset(HPw, 0, sizeof(double) * FRAMESAMPLES_HALF);

  
  WebRtcIsac_Time2Spec(LPw, HPw, fre, fim, &ISACencUB_obj->fftstr_obj);

  
  memcpy(&ISACencUB_obj->SaveEnc_obj.realFFT, fre,
         FRAMESAMPLES_HALF * sizeof(WebRtc_Word16));

  
  memcpy(&ISACencUB_obj->SaveEnc_obj.imagFFT, fim,
         FRAMESAMPLES_HALF * sizeof(WebRtc_Word16));

  
  memcpy(&ISACencUB_obj->SaveEnc_obj.bitStreamObj,
         &ISACencUB_obj->bitstr_obj, sizeof(Bitstr));

  
  err = WebRtcIsac_EncodeSpecUB12(fre, fim, &ISACencUB_obj->bitstr_obj);
  if ((err < 0) && (err != -ISAC_DISALLOWED_BITSTREAM_LENGTH)) {
    

    return err;
  }
  iterCntr = 0;
  while((ISACencUB_obj->bitstr_obj.stream_index > payloadLimitBytes) ||
        (err == -ISAC_DISALLOWED_BITSTREAM_LENGTH)) {
    double bytesSpecCoderUsed;
    double transcodeScale;

    if (iterCntr >= MAX_PAYLOAD_LIMIT_ITERATION) {
      
      return -ISAC_PAYLOAD_LARGER_THAN_LIMIT;
    }

    if (err == -ISAC_DISALLOWED_BITSTREAM_LENGTH) {
      bytesSpecCoderUsed = STREAM_SIZE_MAX;
      
      transcodeScale = bytesLeftSpecCoding / bytesSpecCoderUsed * 0.5;
    } else {
      bytesSpecCoderUsed = ISACencUB_obj->bitstr_obj.stream_index -
          transcodingParam.stream_index;
      transcodeScale = bytesLeftSpecCoding / bytesSpecCoderUsed;
    }

    

    transcodeScale *= (1.0 - (0.9 * (double)iterCntr/
                              (double)MAX_PAYLOAD_LIMIT_ITERATION));

    
    for (k = 0; k < SUBFRAMES; k++) {
      transcodingParam.loFiltGain[k] *= transcodeScale;
    }

    
    for (k = 0; k < FRAMESAMPLES_HALF; k++) {
      fre[k] = (WebRtc_Word16)(fre[k] * transcodeScale + 0.5);
      fim[k] = (WebRtc_Word16)(fim[k] * transcodeScale + 0.5);
    }

    
    memcpy(&ISACencUB_obj->SaveEnc_obj.realFFT, fre,
           FRAMESAMPLES_HALF * sizeof(WebRtc_Word16));

    
    memcpy(&ISACencUB_obj->SaveEnc_obj.imagFFT, fim,
           FRAMESAMPLES_HALF * sizeof(WebRtc_Word16));


    
    ISACencUB_obj->bitstr_obj.W_upper = transcodingParam.W_upper;

    ISACencUB_obj->bitstr_obj.stream_index = transcodingParam.stream_index;

    ISACencUB_obj->bitstr_obj.streamval = transcodingParam.streamval;

    ISACencUB_obj->bitstr_obj.stream[transcodingParam.stream_index - 2] =
        transcodingParam.stream[0];

    ISACencUB_obj->bitstr_obj.stream[transcodingParam.stream_index - 1] =
        transcodingParam.stream[1];

    ISACencUB_obj->bitstr_obj.stream[transcodingParam.stream_index] =
        transcodingParam.stream[2];

    
    memcpy(&ISACencUB_obj->SaveEnc_obj.lpcGain, lpcGains, SUBFRAMES *
           sizeof(double));

    
    
    WebRtcIsac_EncodeLpcGainUb(transcodingParam.loFiltGain,
                               &ISACencUB_obj->bitstr_obj,
                               ISACencUB_obj->SaveEnc_obj.lpcGainIndex);

    
    memcpy(&ISACencUB_obj->SaveEnc_obj.bitStreamObj,
           &ISACencUB_obj->bitstr_obj, sizeof(Bitstr));

    
    bytesLeftSpecCoding = payloadLimitBytes -
        ISACencUB_obj->bitstr_obj.stream_index;

    
    err = WebRtcIsac_EncodeSpecUB12(fre, fim,
                                      &ISACencUB_obj->bitstr_obj);
    if ((err < 0) && (err != -ISAC_DISALLOWED_BITSTREAM_LENGTH)) {
      

      return err;
    }
    iterCntr++;
  }

  
  return WebRtcIsac_EncTerminate(&ISACencUB_obj->bitstr_obj);
}











int WebRtcIsac_EncodeStoredDataLb(
    const ISAC_SaveEncData_t* ISACSavedEnc_obj,
    Bitstr*                   ISACBitStr_obj,
    int                       BWnumber,
    float                     scale)
{
  int ii;
  int status;
  int BWno = BWnumber;

  const WebRtc_UWord16 *WebRtcIsac_kQPitchGainCdf_ptr[1];
  const WebRtc_UWord16 **cdf;

  double tmpLPCcoeffs_lo[(ORDERLO+1)*SUBFRAMES*2];
  double tmpLPCcoeffs_hi[(ORDERHI+1)*SUBFRAMES*2];
  int tmpLPCindex_g[12*2];
  WebRtc_Word16 tmp_fre[FRAMESAMPLES], tmp_fim[FRAMESAMPLES];

  
  if ((BWnumber < 0) || (BWnumber > 23)) {
    return -ISAC_RANGE_ERROR_BW_ESTIMATOR;
  }

  
  ISACBitStr_obj->W_upper = 0xFFFFFFFF;
  ISACBitStr_obj->streamval = 0;
  ISACBitStr_obj->stream_index = 0;

  
  status = WebRtcIsac_EncodeFrameLen(ISACSavedEnc_obj->framelength,
                                     ISACBitStr_obj);
  if (status < 0) {
    
    return status;
  }

  
  if ((scale > 0.0) && (scale < 1.0)) {
    
    for (ii = 0;
         ii < ((ORDERLO + 1)* SUBFRAMES * (1 + ISACSavedEnc_obj->startIdx));
         ii++) {
      tmpLPCcoeffs_lo[ii] = scale *  ISACSavedEnc_obj->LPCcoeffs_lo[ii];
    }
    for (ii = 0;
         ii < ((ORDERHI + 1) * SUBFRAMES *(1 + ISACSavedEnc_obj->startIdx));
         ii++) {
      tmpLPCcoeffs_hi[ii] = scale *  ISACSavedEnc_obj->LPCcoeffs_hi[ii];
    }
    
    for (ii = 0;
         ii < (FRAMESAMPLES_HALF * (1 + ISACSavedEnc_obj->startIdx));
         ii++) {
      tmp_fre[ii] = (WebRtc_Word16)((scale) *
                                    (float)ISACSavedEnc_obj->fre[ii]) ;
      tmp_fim[ii] = (WebRtc_Word16)((scale) *
                                    (float)ISACSavedEnc_obj->fim[ii]) ;
    }
  } else {
    for (ii = 0;
         ii < (KLT_ORDER_GAIN * (1 + ISACSavedEnc_obj->startIdx));
         ii++) {
      tmpLPCindex_g[ii] =  ISACSavedEnc_obj->LPCindex_g[ii];
    }
    for (ii = 0;
         ii < (FRAMESAMPLES_HALF * (1 + ISACSavedEnc_obj->startIdx));
         ii++) {
      tmp_fre[ii] = ISACSavedEnc_obj->fre[ii];
      tmp_fim[ii] = ISACSavedEnc_obj->fim[ii];
    }
  }

  
  WebRtcIsac_EncodeReceiveBw(&BWno, ISACBitStr_obj);

  
  for (ii = 0; ii <= ISACSavedEnc_obj->startIdx; ii++) {
    
    *WebRtcIsac_kQPitchGainCdf_ptr = WebRtcIsac_kQPitchGainCdf;
    WebRtcIsac_EncHistMulti(ISACBitStr_obj,
                            &ISACSavedEnc_obj->pitchGain_index[ii], WebRtcIsac_kQPitchGainCdf_ptr, 1);

    
    
    if (ISACSavedEnc_obj->meanGain[ii] < 0.2) {
      cdf = WebRtcIsac_kQPitchLagCdfPtrLo;
    } else if (ISACSavedEnc_obj->meanGain[ii] < 0.4) {
      cdf = WebRtcIsac_kQPitchLagCdfPtrMid;
    } else {
      cdf = WebRtcIsac_kQPitchLagCdfPtrHi;
    }
    WebRtcIsac_EncHistMulti(ISACBitStr_obj,
                            &ISACSavedEnc_obj->pitchIndex[PITCH_SUBFRAMES*ii], cdf,
                            PITCH_SUBFRAMES);

    
    
    WebRtcIsac_EncHistMulti(ISACBitStr_obj,
                            &ISACSavedEnc_obj->LPCmodel[ii], WebRtcIsac_kQKltModelCdfPtr, 1);

    
    WebRtcIsac_EncHistMulti(ISACBitStr_obj,
                            &ISACSavedEnc_obj->LPCindex_s[KLT_ORDER_SHAPE*ii],
                            WebRtcIsac_kQKltCdfPtrShape[ISACSavedEnc_obj->LPCmodel[ii]],
                            KLT_ORDER_SHAPE);

    
    if (scale < 1.0) {
      WebRtcIsac_TranscodeLPCCoef(&tmpLPCcoeffs_lo[(ORDERLO+1) *
                                                   SUBFRAMES*ii], &tmpLPCcoeffs_hi[(ORDERHI+1)*SUBFRAMES*ii],
                                  ISACSavedEnc_obj->LPCmodel[ii],
                                  &tmpLPCindex_g[KLT_ORDER_GAIN * ii]);
    }

    
    WebRtcIsac_EncHistMulti(ISACBitStr_obj,
                            &tmpLPCindex_g[KLT_ORDER_GAIN*ii], WebRtcIsac_kQKltCdfPtrGain[
                                ISACSavedEnc_obj->LPCmodel[ii]], KLT_ORDER_GAIN);

    
    status = WebRtcIsac_EncodeSpecLb(&tmp_fre[ii*FRAMESAMPLES_HALF],
                                     &tmp_fim[ii*FRAMESAMPLES_HALF], ISACBitStr_obj,
                                     ISACSavedEnc_obj->AvgPitchGain[ii]);
    if (status < 0) {
      return status;
    }
  }

  
  return WebRtcIsac_EncTerminate(ISACBitStr_obj);
}




int WebRtcIsac_EncodeStoredDataUb12(
    const ISACUBSaveEncDataStruct* ISACSavedEnc_obj,
    Bitstr*                        bitStream,
    WebRtc_Word32                    jitterInfo,
    float                          scale)
{
  int n;
  int err;
  double lpcGain[SUBFRAMES];
  WebRtc_Word16 realFFT[FRAMESAMPLES_HALF];
  WebRtc_Word16 imagFFT[FRAMESAMPLES_HALF];

  
  bitStream->W_upper = 0xFFFFFFFF;
  bitStream->streamval = 0;
  bitStream->stream_index = 0;

  
  WebRtcIsac_EncodeJitterInfo(jitterInfo, bitStream);

  err = WebRtcIsac_EncodeBandwidth(isac12kHz, bitStream);
  if(err < 0)
  {
    return err;
  }

  
  WebRtcIsac_EncHistMulti(bitStream, ISACSavedEnc_obj->indexLPCShape,
                          WebRtcIsac_kLpcShapeCdfMatUb12, UB_LPC_ORDER * UB_LPC_VEC_PER_FRAME);


  
  if((scale <= 0.0) || (scale > 1.0))
  {
    scale = 1.0f;
  }

  if(scale == 1.0f)
  {
    
    WebRtcIsac_EncHistMulti(bitStream, ISACSavedEnc_obj->lpcGainIndex,
                            WebRtcIsac_kLpcGainCdfMat, UB_LPC_GAIN_DIM);
    
    err = WebRtcIsac_EncodeSpecUB12(ISACSavedEnc_obj->realFFT,
                                      ISACSavedEnc_obj->imagFFT, bitStream);
  }
  else
  {
    
    for(n = 0; n < SUBFRAMES; n++)
    {
      lpcGain[n] = scale * ISACSavedEnc_obj->lpcGain[n];
    }
    
    WebRtcIsac_StoreLpcGainUb(lpcGain, bitStream);
    for(n = 0; n < FRAMESAMPLES_HALF; n++)
    {
      realFFT[n] = (WebRtc_Word16)(scale * (float)ISACSavedEnc_obj->realFFT[n] + 0.5f);
      imagFFT[n] = (WebRtc_Word16)(scale * (float)ISACSavedEnc_obj->imagFFT[n] + 0.5f);
    }
    
    err = WebRtcIsac_EncodeSpecUB12(realFFT, imagFFT, bitStream);
  }
  if(err < 0)
  {
    
    return err;
  }

  
  return WebRtcIsac_EncTerminate(bitStream);
}


int
WebRtcIsac_EncodeStoredDataUb16(
    const ISACUBSaveEncDataStruct* ISACSavedEnc_obj,
    Bitstr*                        bitStream,
    WebRtc_Word32                    jitterInfo,
    float                          scale)
{
  int n;
  int err;
  double lpcGain[SUBFRAMES << 1];
  WebRtc_Word16 realFFT[FRAMESAMPLES_HALF];
  WebRtc_Word16 imagFFT[FRAMESAMPLES_HALF];

  
  bitStream->W_upper = 0xFFFFFFFF;
  bitStream->streamval = 0;
  bitStream->stream_index = 0;

  
  WebRtcIsac_EncodeJitterInfo(jitterInfo, bitStream);

  err = WebRtcIsac_EncodeBandwidth(isac16kHz, bitStream);
  if(err < 0)
  {
    return err;
  }

  WebRtcIsac_EncHistMulti(bitStream, ISACSavedEnc_obj->indexLPCShape,
                          WebRtcIsac_kLpcShapeCdfMatUb16, UB_LPC_ORDER * UB16_LPC_VEC_PER_FRAME);

  
  if((scale <= 0.0) || (scale > 1.0))
  {
    scale = 1.0f;
  }

  if(scale == 1.0f)
  {
    
    WebRtcIsac_EncHistMulti(bitStream, ISACSavedEnc_obj->lpcGainIndex,
                            WebRtcIsac_kLpcGainCdfMat, UB_LPC_GAIN_DIM);
    WebRtcIsac_EncHistMulti(bitStream, &ISACSavedEnc_obj->lpcGainIndex[SUBFRAMES],
                            WebRtcIsac_kLpcGainCdfMat, UB_LPC_GAIN_DIM);
    
    err = WebRtcIsac_EncodeSpecUB16(ISACSavedEnc_obj->realFFT,
                                      ISACSavedEnc_obj->imagFFT, bitStream);

  }
  else
  {
    
    for(n = 0; n < SUBFRAMES; n++)
    {
      lpcGain[n] = scale * ISACSavedEnc_obj->lpcGain[n];
      lpcGain[n + SUBFRAMES] = scale * ISACSavedEnc_obj->lpcGain[n + SUBFRAMES];
    }
    
    WebRtcIsac_StoreLpcGainUb(lpcGain, bitStream);
    WebRtcIsac_StoreLpcGainUb(&lpcGain[SUBFRAMES], bitStream);
    
    for(n = 0; n < FRAMESAMPLES_HALF; n++)
    {
      realFFT[n] = (WebRtc_Word16)(scale * (float)ISACSavedEnc_obj->realFFT[n] + 0.5f);
      imagFFT[n] = (WebRtc_Word16)(scale * (float)ISACSavedEnc_obj->imagFFT[n] + 0.5f);
    }
    
    err = WebRtcIsac_EncodeSpecUB16(realFFT, imagFFT, bitStream);
  }

  if(err < 0)
  {
    
    return err;
  }

  
  return WebRtcIsac_EncTerminate(bitStream);
}


WebRtc_Word16
WebRtcIsac_GetRedPayloadUb(
    const ISACUBSaveEncDataStruct* ISACSavedEncObj,
    Bitstr*                        bitStreamObj,
    enum ISACBandwidth             bandwidth)
{
  int n;
  WebRtc_Word16 status;
  WebRtc_Word16 realFFT[FRAMESAMPLES_HALF];
  WebRtc_Word16 imagFFT[FRAMESAMPLES_HALF];

  
  memcpy(bitStreamObj, &ISACSavedEncObj->bitStreamObj, sizeof(Bitstr));

  
  for(n = 0; n < FRAMESAMPLES_HALF; n++)
  {
    realFFT[n] = (WebRtc_Word16)((float)ISACSavedEncObj->realFFT[n] *
                                 RCU_TRANSCODING_SCALE_UB + 0.5);
    imagFFT[n] = (WebRtc_Word16)((float)ISACSavedEncObj->imagFFT[n] *
                                 RCU_TRANSCODING_SCALE_UB + 0.5);
  }

  switch(bandwidth)
  {
    case isac12kHz:
      {
        status = WebRtcIsac_EncodeSpecUB12(realFFT, imagFFT, bitStreamObj);
        break;
      }
    case isac16kHz:
      {
        status = WebRtcIsac_EncodeSpecUB16(realFFT, imagFFT, bitStreamObj);
        break;
      }
    default:
      return -1;
  }

  if(status < 0)
  {
    
    return status;
  }
  else
  {
    
    return WebRtcIsac_EncTerminate(bitStreamObj);
  }
}
