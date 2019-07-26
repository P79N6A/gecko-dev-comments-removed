




















#include "codec.h"
#include "entropy_coding.h"
#include "pitch_estimator.h"
#include "bandwidth_estimator.h"
#include "structs.h"
#include "settings.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>






int WebRtcIsac_DecodeLb(float* signal_out, ISACLBDecStruct* ISACdecLB_obj,
                        WebRtc_Word16* current_framesamples,
                        WebRtc_Word16 isRCUPayload) {
  int k;
  int len, err;
  WebRtc_Word16 bandwidthInd;

  float LP_dec_float[FRAMESAMPLES_HALF];
  float HP_dec_float[FRAMESAMPLES_HALF];

  double LPw[FRAMESAMPLES_HALF];
  double HPw[FRAMESAMPLES_HALF];
  double LPw_pf[FRAMESAMPLES_HALF];

  double lo_filt_coef[(ORDERLO + 1)*SUBFRAMES];
  double hi_filt_coef[(ORDERHI + 1)*SUBFRAMES];

  double real_f[FRAMESAMPLES_HALF];
  double imag_f[FRAMESAMPLES_HALF];

  double PitchLags[4];
  double PitchGains[4];
  double AvgPitchGain;
  WebRtc_Word16 PitchGains_Q12[4];
  WebRtc_Word16 AvgPitchGain_Q12;

  float gain;

  int frame_nb; 
  int frame_mode; 
  

  WebRtcIsac_ResetBitstream(&(ISACdecLB_obj->bitstr_obj));

  len = 0;

  

  err = WebRtcIsac_DecodeFrameLen(&ISACdecLB_obj->bitstr_obj,
                                  current_framesamples);
  if (err < 0) {
    return err;
  }

  


  frame_mode = *current_framesamples / MAX_FRAMESAMPLES;

  err = WebRtcIsac_DecodeSendBW(&ISACdecLB_obj->bitstr_obj, &bandwidthInd);
  if (err < 0) {
    return err;
  }

  

  for (frame_nb = 0; frame_nb <= frame_mode; frame_nb++) {
    
    err = WebRtcIsac_DecodePitchGain(&ISACdecLB_obj->bitstr_obj,
                                     PitchGains_Q12);
    if (err < 0) {
      return err;
    }

    err = WebRtcIsac_DecodePitchLag(&ISACdecLB_obj->bitstr_obj, PitchGains_Q12,
                                    PitchLags);
    if (err < 0) {
      return err;
    }

    AvgPitchGain_Q12 = (PitchGains_Q12[0] + PitchGains_Q12[1] +
        PitchGains_Q12[2] + PitchGains_Q12[3]) >> 2;

    
    err = WebRtcIsac_DecodeLpc(&ISACdecLB_obj->bitstr_obj, lo_filt_coef,
                               hi_filt_coef);
    if (err < 0) {
      return err;
    }
    
    len = WebRtcIsac_DecodeSpec(&ISACdecLB_obj->bitstr_obj, AvgPitchGain_Q12,
                                kIsacLowerBand, real_f, imag_f);
    if (len < 0) {
      return len;
    }

    
    WebRtcIsac_Spec2time(real_f, imag_f, LPw, HPw,
                         &ISACdecLB_obj->fftstr_obj);

    
    for (k = 0; k < 4; k++) {
      PitchGains[k] = ((float)PitchGains_Q12[k]) / 4096;
    }
    if (isRCUPayload) {
      for (k = 0; k < 240; k++) {
        LPw[k] *= RCU_TRANSCODING_SCALE_INVERSE;
        HPw[k] *= RCU_TRANSCODING_SCALE_INVERSE;
      }
    }

    
    WebRtcIsac_PitchfilterPost(LPw, LPw_pf, &ISACdecLB_obj->pitchfiltstr_obj,
                               PitchLags, PitchGains);
    
    AvgPitchGain = ((float)AvgPitchGain_Q12) / 4096;
    gain = 1.0f - 0.45f * (float)AvgPitchGain;

    for (k = 0; k < FRAMESAMPLES_HALF; k++) {
      
      LPw_pf[k] *= gain;
    }

    if (isRCUPayload) {
      for (k = 0; k < FRAMESAMPLES_HALF; k++) {
        
        LPw_pf[k] *= RCU_TRANSCODING_SCALE;
        HPw[k] *= RCU_TRANSCODING_SCALE;
      }
    }
    
    WebRtcIsac_NormLatticeFilterAr(
        ORDERLO, ISACdecLB_obj->maskfiltstr_obj.PostStateLoF,
        (ISACdecLB_obj->maskfiltstr_obj).PostStateLoG, LPw_pf, lo_filt_coef,
        LP_dec_float);
    WebRtcIsac_NormLatticeFilterAr(
        ORDERHI, ISACdecLB_obj->maskfiltstr_obj.PostStateHiF,
        (ISACdecLB_obj->maskfiltstr_obj).PostStateHiG, HPw, hi_filt_coef,
        HP_dec_float);

    
    WebRtcIsac_FilterAndCombineFloat(LP_dec_float, HP_dec_float,
                                     signal_out + frame_nb * FRAMESAMPLES,
                                     &ISACdecLB_obj->postfiltbankstr_obj);
  }
  return len;
}









int WebRtcIsac_DecodeUb16(float* signal_out, ISACUBDecStruct* ISACdecUB_obj,
                          WebRtc_Word16 isRCUPayload) {
  int len, err;

  double halfFrameFirst[FRAMESAMPLES_HALF];
  double halfFrameSecond[FRAMESAMPLES_HALF];

  double percepFilterParam[(UB_LPC_ORDER + 1) * (SUBFRAMES << 1) +
                           (UB_LPC_ORDER + 1)];

  double real_f[FRAMESAMPLES_HALF];
  double imag_f[FRAMESAMPLES_HALF];
  const WebRtc_Word16 kAveragePitchGain = 0; 
  len = 0;

  
  memset(percepFilterParam, 0, sizeof(percepFilterParam));
  err = WebRtcIsac_DecodeInterpolLpcUb(&ISACdecUB_obj->bitstr_obj,
                                       percepFilterParam, isac16kHz);
  if (err < 0) {
    return err;
  }

  
  len = WebRtcIsac_DecodeSpec(&ISACdecUB_obj->bitstr_obj, kAveragePitchGain,
                              kIsacUpperBand16, real_f, imag_f);
  if (len < 0) {
    return len;
  }
  if (isRCUPayload) {
    int n;
    for (n = 0; n < 240; n++) {
      real_f[n] *= RCU_TRANSCODING_SCALE_UB_INVERSE;
      imag_f[n] *= RCU_TRANSCODING_SCALE_UB_INVERSE;
    }
  }
  
  WebRtcIsac_Spec2time(real_f, imag_f, halfFrameFirst, halfFrameSecond,
                       &ISACdecUB_obj->fftstr_obj);

  
  WebRtcIsac_NormLatticeFilterAr(
      UB_LPC_ORDER, ISACdecUB_obj->maskfiltstr_obj.PostStateLoF,
      (ISACdecUB_obj->maskfiltstr_obj).PostStateLoG, halfFrameFirst,
      &percepFilterParam[(UB_LPC_ORDER + 1)], signal_out);

  WebRtcIsac_NormLatticeFilterAr(
      UB_LPC_ORDER, ISACdecUB_obj->maskfiltstr_obj.PostStateLoF,
      (ISACdecUB_obj->maskfiltstr_obj).PostStateLoG, halfFrameSecond,
      &percepFilterParam[(UB_LPC_ORDER + 1) * SUBFRAMES + (UB_LPC_ORDER + 1)],
      &signal_out[FRAMESAMPLES_HALF]);

  return len;
}










int WebRtcIsac_DecodeUb12(float* signal_out, ISACUBDecStruct* ISACdecUB_obj,
                      WebRtc_Word16 isRCUPayload) {
  int len, err;

  float LP_dec_float[FRAMESAMPLES_HALF];
  float HP_dec_float[FRAMESAMPLES_HALF];

  double LPw[FRAMESAMPLES_HALF];
  double HPw[FRAMESAMPLES_HALF];

  double percepFilterParam[(UB_LPC_ORDER + 1)*SUBFRAMES];

  double real_f[FRAMESAMPLES_HALF];
  double imag_f[FRAMESAMPLES_HALF];
  const WebRtc_Word16 kAveragePitchGain = 0; 
  len = 0;

  
  err = WebRtcIsac_DecodeInterpolLpcUb(&ISACdecUB_obj->bitstr_obj,
                                       percepFilterParam, isac12kHz);
  if (err < 0) {
    return err;
  }

  
  len = WebRtcIsac_DecodeSpec(&ISACdecUB_obj->bitstr_obj, kAveragePitchGain,
                              kIsacUpperBand12, real_f, imag_f);
  if (len < 0) {
    return len;
  }

  if (isRCUPayload) {
    int n;
    for (n = 0; n < 240; n++) {
      real_f[n] *= RCU_TRANSCODING_SCALE_UB_INVERSE;
      imag_f[n] *= RCU_TRANSCODING_SCALE_UB_INVERSE;
    }
  }
  
  WebRtcIsac_Spec2time(real_f, imag_f, LPw, HPw, &ISACdecUB_obj->fftstr_obj);
  
  WebRtcIsac_NormLatticeFilterAr(UB_LPC_ORDER,
                                 ISACdecUB_obj->maskfiltstr_obj.PostStateLoF,
                                 (ISACdecUB_obj->maskfiltstr_obj).PostStateLoG,
                                 LPw, percepFilterParam, LP_dec_float);
  
  memset(HP_dec_float, 0, sizeof(float) * (FRAMESAMPLES_HALF));
  
  WebRtcIsac_FilterAndCombineFloat(HP_dec_float, LP_dec_float, signal_out,
                                   &ISACdecUB_obj->postfiltbankstr_obj);
  return len;
}
