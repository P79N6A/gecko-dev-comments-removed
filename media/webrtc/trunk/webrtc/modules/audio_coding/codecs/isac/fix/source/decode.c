
















#include <string.h>

#include "bandwidth_estimator.h"
#include "codec.h"
#include "entropy_coding.h"
#include "pitch_estimator.h"
#include "settings.h"
#include "structs.h"




int16_t WebRtcIsacfix_DecodeImpl(int16_t       *signal_out16,
                                 ISACFIX_DecInst_t *ISACdec_obj,
                                 int16_t       *current_framesamples)
{
  int k;
  int err;
  int16_t BWno;
  int16_t len = 0;

  int16_t model;


  int16_t Vector_Word16_1[FRAMESAMPLES/2];
  int16_t Vector_Word16_2[FRAMESAMPLES/2];

  int32_t Vector_Word32_1[FRAMESAMPLES/2];
  int32_t Vector_Word32_2[FRAMESAMPLES/2];

  int16_t lofilt_coefQ15[ORDERLO*SUBFRAMES]; 
  int16_t hifilt_coefQ15[ORDERHI*SUBFRAMES]; 
  int32_t gain_lo_hiQ17[2*SUBFRAMES];

  int16_t PitchLags_Q7[PITCH_SUBFRAMES];
  int16_t PitchGains_Q12[PITCH_SUBFRAMES];
  int16_t AvgPitchGain_Q12;

  int16_t tmp_1, tmp_2;
  int32_t tmp32a;
  int16_t gainQ13;


  int16_t frame_nb; 
  int16_t frame_mode; 
  static const int16_t kProcessedSamples = 480; 

  
  int16_t overlapWin[ 240 ];

  (ISACdec_obj->bitstr_obj).W_upper = 0xFFFFFFFF;
  (ISACdec_obj->bitstr_obj).streamval = 0;
  (ISACdec_obj->bitstr_obj).stream_index = 0;
  (ISACdec_obj->bitstr_obj).full = 1;


  
  err = WebRtcIsacfix_DecodeFrameLen(&ISACdec_obj->bitstr_obj, current_framesamples);
  if (err<0)  
    return err;

  frame_mode = *current_framesamples / MAX_FRAMESAMPLES;  

  err = WebRtcIsacfix_DecodeSendBandwidth(&ISACdec_obj->bitstr_obj, &BWno);
  if (err<0)  
    return err;

  

  for (frame_nb = 0; frame_nb <= frame_mode; frame_nb++) {

    
    err = WebRtcIsacfix_DecodePitchGain(&(ISACdec_obj->bitstr_obj), PitchGains_Q12);
    if (err<0)  
      return err;

    err = WebRtcIsacfix_DecodePitchLag(&ISACdec_obj->bitstr_obj, PitchGains_Q12, PitchLags_Q7);
    if (err<0)  
      return err;

    AvgPitchGain_Q12 = (int16_t)(((int32_t)PitchGains_Q12[0] + PitchGains_Q12[1] + PitchGains_Q12[2] + PitchGains_Q12[3])>>2);

    
    err = WebRtcIsacfix_DecodeLpc(gain_lo_hiQ17, lofilt_coefQ15, hifilt_coefQ15,
                                  &ISACdec_obj->bitstr_obj, &model);

    if (err<0)  
      return err;

    
    len = WebRtcIsacfix_DecodeSpec(&ISACdec_obj->bitstr_obj, Vector_Word16_1, Vector_Word16_2, AvgPitchGain_Q12);
    if (len < 0)  
      return len;

    
    WebRtcIsacfix_Spec2Time(Vector_Word16_1, Vector_Word16_2, Vector_Word32_1, Vector_Word32_2);

    for (k=0; k<FRAMESAMPLES/2; k++) {
      
      Vector_Word16_1[k] = (int16_t)((Vector_Word32_1[k] + 64) >> 7);
    }

    
    if( (ISACdec_obj->plcstr_obj).used == PLC_WAS_USED )
    {
      (ISACdec_obj->plcstr_obj).used = PLC_NOT_USED;
      if( (ISACdec_obj->plcstr_obj).B < 1000 )
      {
        (ISACdec_obj->plcstr_obj).decayCoeffPriodic = 4000;
      }

      ISACdec_obj->plcstr_obj.decayCoeffPriodic = WEBRTC_SPL_WORD16_MAX;    
      ISACdec_obj->plcstr_obj.decayCoeffNoise = WEBRTC_SPL_WORD16_MAX;    
      ISACdec_obj->plcstr_obj.pitchCycles = 0;

      PitchGains_Q12[0] = (int16_t)WEBRTC_SPL_MUL_16_16_RSFT(PitchGains_Q12[0], 700, 10 );

      
      WebRtcSpl_GetHanningWindow( overlapWin, RECOVERY_OVERLAP );
      for( k = 0; k < RECOVERY_OVERLAP; k++ )
        Vector_Word16_1[k] = WebRtcSpl_AddSatW16(
            (int16_t)WEBRTC_SPL_MUL_16_16_RSFT( (ISACdec_obj->plcstr_obj).overlapLP[k], overlapWin[RECOVERY_OVERLAP - k - 1], 14),
            (int16_t)WEBRTC_SPL_MUL_16_16_RSFT( Vector_Word16_1[k], overlapWin[k], 14) );



    }

    
    if( frame_nb == frame_mode )
    {
      
      WEBRTC_SPL_MEMCPY_W16( (ISACdec_obj->plcstr_obj).lofilt_coefQ15, &lofilt_coefQ15[(SUBFRAMES-1)*ORDERLO], ORDERLO );
      WEBRTC_SPL_MEMCPY_W16( (ISACdec_obj->plcstr_obj).hifilt_coefQ15, &hifilt_coefQ15[(SUBFRAMES-1)*ORDERHI], ORDERHI );
      (ISACdec_obj->plcstr_obj).gain_lo_hiQ17[0] = gain_lo_hiQ17[(SUBFRAMES-1) * 2];
      (ISACdec_obj->plcstr_obj).gain_lo_hiQ17[1] = gain_lo_hiQ17[(SUBFRAMES-1) * 2 + 1];

      
      (ISACdec_obj->plcstr_obj).AvgPitchGain_Q12 = PitchGains_Q12[3];
      (ISACdec_obj->plcstr_obj).lastPitchGain_Q12 = PitchGains_Q12[3];
      (ISACdec_obj->plcstr_obj).lastPitchLag_Q7 = PitchLags_Q7[3];

      if( PitchLags_Q7[3] < 3000 )
        (ISACdec_obj->plcstr_obj).lastPitchLag_Q7 += PitchLags_Q7[3];

      WEBRTC_SPL_MEMCPY_W16( (ISACdec_obj->plcstr_obj).prevPitchInvIn, Vector_Word16_1, FRAMESAMPLES/2 );

    }
    

    
    WebRtcIsacfix_PitchFilter(Vector_Word16_1, Vector_Word16_2, &ISACdec_obj->pitchfiltstr_obj, PitchLags_Q7, PitchGains_Q12, 4);

    if( frame_nb == frame_mode )
    {
      WEBRTC_SPL_MEMCPY_W16( (ISACdec_obj->plcstr_obj).prevPitchInvOut, &(Vector_Word16_2[FRAMESAMPLES/2 - (PITCH_MAX_LAG + 10)]), PITCH_MAX_LAG );
    }


    
    
    tmp32a = WEBRTC_SPL_MUL_16_16_RSFT(AvgPitchGain_Q12, 29, 0); 
    gainQ13 = (int16_t)((262144 - tmp32a) >> 5);  

    for (k = 0; k < FRAMESAMPLES/2; k++)
    {
      Vector_Word32_1[k] = (int32_t) WEBRTC_SPL_LSHIFT_W32(WEBRTC_SPL_MUL_16_16(Vector_Word16_2[k], gainQ13), 3); 
    }


    
    WebRtcIsacfix_NormLatticeFilterAr(ORDERLO, (ISACdec_obj->maskfiltstr_obj).PostStateLoGQ0,
                                      Vector_Word32_1, lofilt_coefQ15, gain_lo_hiQ17, 0, Vector_Word16_1);

    
    for (k = 0; k < FRAMESAMPLES/2; k++)
      Vector_Word32_1[k]    = WEBRTC_SPL_LSHIFT_W32(Vector_Word32_2[k], 9); 

    for( k = 0; k < PITCH_MAX_LAG + 10; k++ )
      (ISACdec_obj->plcstr_obj).prevHP[k] = Vector_Word32_1[FRAMESAMPLES/2 - (PITCH_MAX_LAG + 10) + k];


    WebRtcIsacfix_NormLatticeFilterAr(ORDERHI, (ISACdec_obj->maskfiltstr_obj).PostStateHiGQ0,
                                      Vector_Word32_1, hifilt_coefQ15, gain_lo_hiQ17, 1, Vector_Word16_2);

    

    
    for (k=0;k<FRAMESAMPLES/2;k++) {
      tmp_1 = (int16_t)WebRtcSpl_SatW32ToW16(((int32_t)Vector_Word16_1[k]+Vector_Word16_2[k] + 1)); 
      tmp_2 = (int16_t)WebRtcSpl_SatW32ToW16(((int32_t)Vector_Word16_1[k]-Vector_Word16_2[k])); 
      Vector_Word16_1[k] = tmp_1;
      Vector_Word16_2[k] = tmp_2;
    }

    WebRtcIsacfix_FilterAndCombine1(Vector_Word16_1,
                                    Vector_Word16_2,
                                    signal_out16 + frame_nb * kProcessedSamples,
                                    &ISACdec_obj->postfiltbankstr_obj);

  }
  return len;
}
