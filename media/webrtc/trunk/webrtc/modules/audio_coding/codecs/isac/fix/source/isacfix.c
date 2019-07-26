
















#include "modules/audio_coding/codecs/isac/fix/interface/isacfix.h"

#include <stdlib.h>

#include "modules/audio_coding/codecs/isac/fix/source/bandwidth_estimator.h"
#include "modules/audio_coding/codecs/isac/fix/source/codec.h"
#include "modules/audio_coding/codecs/isac/fix/source/entropy_coding.h"
#include "modules/audio_coding/codecs/isac/fix/source/filterbank_internal.h"
#include "modules/audio_coding/codecs/isac/fix/source/lpc_masking_model.h"
#include "modules/audio_coding/codecs/isac/fix/source/structs.h"
#include "system_wrappers/interface/cpu_features_wrapper.h"


FilterMaLoopFix WebRtcIsacfix_FilterMaLoopFix;
Spec2Time WebRtcIsacfix_Spec2Time;
Time2Spec WebRtcIsacfix_Time2Spec;
MatrixProduct1 WebRtcIsacfix_MatrixProduct1;
MatrixProduct2 WebRtcIsacfix_MatrixProduct2;









int16_t WebRtcIsacfix_AssignSize(int *sizeinbytes) {
  *sizeinbytes=sizeof(ISACFIX_SubStruct)*2/sizeof(int16_t);
  return(0);
}










int16_t WebRtcIsacfix_Assign(ISACFIX_MainStruct **inst, void *ISACFIX_inst_Addr) {
  if (ISACFIX_inst_Addr!=NULL) {
    *inst = (ISACFIX_MainStruct*)ISACFIX_inst_Addr;
    (*(ISACFIX_SubStruct**)inst)->errorcode = 0;
    (*(ISACFIX_SubStruct**)inst)->initflag = 0;
    (*(ISACFIX_SubStruct**)inst)->ISACenc_obj.SaveEnc_ptr = NULL;
    return(0);
  } else {
    return(-1);
  }
}


#ifndef ISACFIX_NO_DYNAMIC_MEM














int16_t WebRtcIsacfix_Create(ISACFIX_MainStruct **ISAC_main_inst)
{
  ISACFIX_SubStruct *tempo;
  tempo = malloc(1 * sizeof(ISACFIX_SubStruct));
  *ISAC_main_inst = (ISACFIX_MainStruct *)tempo;
  if (*ISAC_main_inst!=NULL) {
    (*(ISACFIX_SubStruct**)ISAC_main_inst)->errorcode = 0;
    (*(ISACFIX_SubStruct**)ISAC_main_inst)->initflag = 0;
    (*(ISACFIX_SubStruct**)ISAC_main_inst)->ISACenc_obj.SaveEnc_ptr = NULL;
    WebRtcSpl_Init();
    return(0);
  } else {
    return(-1);
  }
}














int16_t WebRtcIsacfix_CreateInternal(ISACFIX_MainStruct *ISAC_main_inst)
{
  ISACFIX_SubStruct *ISAC_inst;

  
  ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;

  
  ISAC_inst->ISACenc_obj.SaveEnc_ptr = malloc(1 * sizeof(ISAC_SaveEncData_t));

  if (ISAC_inst->ISACenc_obj.SaveEnc_ptr!=NULL) {
    return(0);
  } else {
    return(-1);
  }
}


#endif















int16_t WebRtcIsacfix_Free(ISACFIX_MainStruct *ISAC_main_inst)
{
  free(ISAC_main_inst);
  return(0);
}













int16_t WebRtcIsacfix_FreeInternal(ISACFIX_MainStruct *ISAC_main_inst)
{
  ISACFIX_SubStruct *ISAC_inst;

  
  ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;

  
  free(ISAC_inst->ISACenc_obj.SaveEnc_ptr);

  return(0);
}







#if (defined WEBRTC_DETECT_ARM_NEON || defined WEBRTC_ARCH_ARM_NEON)
static void WebRtcIsacfix_InitNeon(void) {
  WebRtcIsacfix_AutocorrFix = WebRtcIsacfix_AutocorrNeon;
  WebRtcIsacfix_FilterMaLoopFix = WebRtcIsacfix_FilterMaLoopNeon;
  WebRtcIsacfix_Spec2Time = WebRtcIsacfix_Spec2TimeNeon;
  WebRtcIsacfix_Time2Spec = WebRtcIsacfix_Time2SpecNeon;
  WebRtcIsacfix_CalculateResidualEnergy =
      WebRtcIsacfix_CalculateResidualEnergyNeon;
  WebRtcIsacfix_AllpassFilter2FixDec16 =
      WebRtcIsacfix_AllpassFilter2FixDec16Neon;
  WebRtcIsacfix_MatrixProduct1 = WebRtcIsacfix_MatrixProduct1Neon;
  WebRtcIsacfix_MatrixProduct2 = WebRtcIsacfix_MatrixProduct2Neon;
}
#endif



















int16_t WebRtcIsacfix_EncoderInit(ISACFIX_MainStruct *ISAC_main_inst,
                                  int16_t  CodingMode)
{
  int k;
  int16_t statusInit;
  ISACFIX_SubStruct *ISAC_inst;

  statusInit = 0;
  
  ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;

  
  ISAC_inst->initflag |= 2;

  if (CodingMode == 0)
    
    ISAC_inst->ISACenc_obj.new_framelength  = INITIAL_FRAMESAMPLES;
  else if (CodingMode == 1)
    
    ISAC_inst->ISACenc_obj.new_framelength = 480;    
  else {
    ISAC_inst->errorcode = ISAC_DISALLOWED_CODING_MODE;
    statusInit = -1;
  }

  ISAC_inst->CodingMode = CodingMode;

  WebRtcIsacfix_InitMaskingEnc(&ISAC_inst->ISACenc_obj.maskfiltstr_obj);
  WebRtcIsacfix_InitPreFilterbank(&ISAC_inst->ISACenc_obj.prefiltbankstr_obj);
  WebRtcIsacfix_InitPitchFilter(&ISAC_inst->ISACenc_obj.pitchfiltstr_obj);
  WebRtcIsacfix_InitPitchAnalysis(&ISAC_inst->ISACenc_obj.pitchanalysisstr_obj);


  WebRtcIsacfix_InitBandwidthEstimator(&ISAC_inst->bwestimator_obj);
  WebRtcIsacfix_InitRateModel(&ISAC_inst->ISACenc_obj.rate_data_obj);


  ISAC_inst->ISACenc_obj.buffer_index   = 0;
  ISAC_inst->ISACenc_obj.frame_nb    = 0;
  ISAC_inst->ISACenc_obj.BottleNeck      = 32000; 
  ISAC_inst->ISACenc_obj.MaxDelay    = 10;    
  ISAC_inst->ISACenc_obj.current_framesamples = 0;
  ISAC_inst->ISACenc_obj.s2nr     = 0;
  ISAC_inst->ISACenc_obj.MaxBits    = 0;
  ISAC_inst->ISACenc_obj.bitstr_seed   = 4447;
  ISAC_inst->ISACenc_obj.payloadLimitBytes30  = STREAM_MAXW16_30MS << 1;
  ISAC_inst->ISACenc_obj.payloadLimitBytes60  = STREAM_MAXW16_60MS << 1;
  ISAC_inst->ISACenc_obj.maxPayloadBytes      = STREAM_MAXW16_60MS << 1;
  ISAC_inst->ISACenc_obj.maxRateInBytes       = STREAM_MAXW16_30MS << 1;
  ISAC_inst->ISACenc_obj.enforceFrameSize     = 0;

  
  for (k=0; k<STREAM_MAXW16_60MS; k++){
    ISAC_inst->ISACenc_obj.bitstr_obj.stream[k] = 0;
  }

#ifdef WEBRTC_ISAC_FIX_NB_CALLS_ENABLED
  WebRtcIsacfix_InitPostFilterbank(&ISAC_inst->ISACenc_obj.interpolatorstr_obj);
#endif

  
  WebRtcIsacfix_AutocorrFix = WebRtcIsacfix_AutocorrC;
  WebRtcIsacfix_FilterMaLoopFix = WebRtcIsacfix_FilterMaLoopC;
  WebRtcIsacfix_CalculateResidualEnergy =
      WebRtcIsacfix_CalculateResidualEnergyC;
  WebRtcIsacfix_AllpassFilter2FixDec16 = WebRtcIsacfix_AllpassFilter2FixDec16C;
  WebRtcIsacfix_Time2Spec = WebRtcIsacfix_Time2SpecC;
  WebRtcIsacfix_Spec2Time = WebRtcIsacfix_Spec2TimeC;
  WebRtcIsacfix_MatrixProduct1 = WebRtcIsacfix_MatrixProduct1C;
  WebRtcIsacfix_MatrixProduct2 = WebRtcIsacfix_MatrixProduct2C ;

#ifdef WEBRTC_DETECT_ARM_NEON
  if ((WebRtc_GetCPUFeaturesARM() & kCPUFeatureNEON) != 0) {
    WebRtcIsacfix_InitNeon();
  }
#elif defined(WEBRTC_ARCH_ARM_NEON)
  WebRtcIsacfix_InitNeon();
#endif

  return statusInit;
}























int16_t WebRtcIsacfix_Encode(ISACFIX_MainStruct *ISAC_main_inst,
                             const int16_t    *speechIn,
                             int16_t          *encoded)
{
  ISACFIX_SubStruct *ISAC_inst;
  int16_t stream_len;
#ifndef WEBRTC_ARCH_BIG_ENDIAN
  int k;
#endif

  
  ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;


  
  if ((ISAC_inst->initflag & 2) != 2) {
    ISAC_inst->errorcode = ISAC_ENCODER_NOT_INITIATED;
    return (-1);
  }

  stream_len = WebRtcIsacfix_EncodeImpl((int16_t*)speechIn,
                                        &ISAC_inst->ISACenc_obj,
                                        &ISAC_inst->bwestimator_obj,
                                        ISAC_inst->CodingMode);
  if (stream_len<0) {
    ISAC_inst->errorcode = - stream_len;
    return -1;
  }


  
#ifndef WEBRTC_ARCH_BIG_ENDIAN
  for (k=0;k<(stream_len+1)>>1;k++) {
    encoded[k] = (int16_t)( ( (uint16_t)(ISAC_inst->ISACenc_obj.bitstr_obj).stream[k] >> 8 )
                                  | (((ISAC_inst->ISACenc_obj.bitstr_obj).stream[k] & 0x00FF) << 8));
  }

#else
  WEBRTC_SPL_MEMCPY_W16(encoded, (ISAC_inst->ISACenc_obj.bitstr_obj).stream, (stream_len + 1)>>1);
#endif



  return stream_len;

}




























#ifdef WEBRTC_ISAC_FIX_NB_CALLS_ENABLED
int16_t WebRtcIsacfix_EncodeNb(ISACFIX_MainStruct *ISAC_main_inst,
                               const int16_t    *speechIn,
                               int16_t          *encoded)
{
  ISACFIX_SubStruct *ISAC_inst;
  int16_t stream_len;
  int16_t speechInWB[FRAMESAMPLES_10ms];
  int16_t Vector_Word16_1[FRAMESAMPLES_10ms/2];
  int16_t Vector_Word16_2[FRAMESAMPLES_10ms/2];

  int k;


  
  ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;


  
  if ((ISAC_inst->initflag & 2) != 2) {
    ISAC_inst->errorcode = ISAC_ENCODER_NOT_INITIATED;
    return (-1);
  }


  

  
  for (k=0;k<FRAMESAMPLES_10ms/2;k++) {
    Vector_Word16_1[k] = speechIn[k] + 1;
    Vector_Word16_2[k] = speechIn[k];
  }
  WebRtcIsacfix_FilterAndCombine2(Vector_Word16_1, Vector_Word16_2, speechInWB, &ISAC_inst->ISACenc_obj.interpolatorstr_obj, FRAMESAMPLES_10ms);


  
  stream_len = WebRtcIsacfix_EncodeImpl((int16_t*)speechInWB,
                                        &ISAC_inst->ISACenc_obj,
                                        &ISAC_inst->bwestimator_obj,
                                        ISAC_inst->CodingMode);
  if (stream_len<0) {
    ISAC_inst->errorcode = - stream_len;
    return -1;
  }


  
#ifndef WEBRTC_ARCH_BIG_ENDIAN
  for (k=0;k<(stream_len+1)>>1;k++) {
    encoded[k] = (int16_t)(((uint16_t)(ISAC_inst->ISACenc_obj.bitstr_obj).stream[k] >> 8)
                                 | (((ISAC_inst->ISACenc_obj.bitstr_obj).stream[k] & 0x00FF) << 8));
  }

#else
  WEBRTC_SPL_MEMCPY_W16(encoded, (ISAC_inst->ISACenc_obj.bitstr_obj).stream, (stream_len + 1)>>1);
#endif



  return stream_len;
}
#endif  





















int16_t WebRtcIsacfix_GetNewBitStream(ISACFIX_MainStruct *ISAC_main_inst,
                                      int16_t      bweIndex,
                                      float              scale,
                                      int16_t        *encoded)
{
  ISACFIX_SubStruct *ISAC_inst;
  int16_t stream_len;
#ifndef WEBRTC_ARCH_BIG_ENDIAN
  int k;
#endif

  
  ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;


  
  if ((ISAC_inst->initflag & 2) != 2) {
    ISAC_inst->errorcode = ISAC_ENCODER_NOT_INITIATED;
    return (-1);
  }

  stream_len = WebRtcIsacfix_EncodeStoredData(&ISAC_inst->ISACenc_obj,
                                              bweIndex,
                                              scale);
  if (stream_len<0) {
    ISAC_inst->errorcode = - stream_len;
    return -1;
  }

#ifndef WEBRTC_ARCH_BIG_ENDIAN
  for (k=0;k<(stream_len+1)>>1;k++) {
    encoded[k] = (int16_t)( ( (uint16_t)(ISAC_inst->ISACenc_obj.bitstr_obj).stream[k] >> 8 )
                                  | (((ISAC_inst->ISACenc_obj.bitstr_obj).stream[k] & 0x00FF) << 8));
  }

#else
  WEBRTC_SPL_MEMCPY_W16(encoded, (ISAC_inst->ISACenc_obj.bitstr_obj).stream, (stream_len + 1)>>1);
#endif

  return stream_len;

}
















int16_t WebRtcIsacfix_DecoderInit(ISACFIX_MainStruct *ISAC_main_inst)
{
  ISACFIX_SubStruct *ISAC_inst;

  
  ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;

  
  ISAC_inst->initflag |= 1;

  WebRtcIsacfix_InitMaskingDec(&ISAC_inst->ISACdec_obj.maskfiltstr_obj);
  WebRtcIsacfix_InitPostFilterbank(&ISAC_inst->ISACdec_obj.postfiltbankstr_obj);
  WebRtcIsacfix_InitPitchFilter(&ISAC_inst->ISACdec_obj.pitchfiltstr_obj);

  
  WebRtcIsacfix_InitPlc( &ISAC_inst->ISACdec_obj.plcstr_obj );


#ifdef WEBRTC_ISAC_FIX_NB_CALLS_ENABLED
  WebRtcIsacfix_InitPreFilterbank(&ISAC_inst->ISACdec_obj.decimatorstr_obj);
#endif

  return 0;
}



















int16_t WebRtcIsacfix_UpdateBwEstimate1(ISACFIX_MainStruct *ISAC_main_inst,
                                        const uint16_t   *encoded,
                                        int32_t          packet_size,
                                        uint16_t         rtp_seq_number,
                                        uint32_t         arr_ts)
{
  ISACFIX_SubStruct *ISAC_inst;
  Bitstr_dec streamdata;
  uint16_t partOfStream[5];
#ifndef WEBRTC_ARCH_BIG_ENDIAN
  int k;
#endif
  int16_t err;

  
  streamdata.stream = (uint16_t *)partOfStream;

  
  ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;

  
  if (packet_size <= 0) {
    
    ISAC_inst->errorcode = ISAC_EMPTY_PACKET;
    return -1;
  } else if (packet_size > (STREAM_MAXW16<<1)) {
    
    ISAC_inst->errorcode = ISAC_LENGTH_MISMATCH;
    return -1;
  }

  
  if ((ISAC_inst->initflag & 1) != 1) {
    ISAC_inst->errorcode = ISAC_DECODER_NOT_INITIATED;
    return (-1);
  }

  streamdata.W_upper = 0xFFFFFFFF;
  streamdata.streamval = 0;
  streamdata.stream_index = 0;
  streamdata.full = 1;

#ifndef WEBRTC_ARCH_BIG_ENDIAN
  for (k=0; k<5; k++) {
    streamdata.stream[k] = (uint16_t) (((uint16_t)encoded[k] >> 8)|((encoded[k] & 0xFF)<<8));
  }
#else
  memcpy(streamdata.stream, encoded, 5);
#endif

  err = WebRtcIsacfix_EstimateBandwidth(&ISAC_inst->bwestimator_obj,
                                        &streamdata,
                                        packet_size,
                                        rtp_seq_number,
                                        0,
                                        arr_ts);


  if (err < 0)
  {
    
    ISAC_inst->errorcode = -err;
    return -1;
  }


  return 0;
}



















int16_t WebRtcIsacfix_UpdateBwEstimate(ISACFIX_MainStruct *ISAC_main_inst,
                                       const uint16_t   *encoded,
                                       int32_t          packet_size,
                                       uint16_t         rtp_seq_number,
                                       uint32_t         send_ts,
                                       uint32_t         arr_ts)
{
  ISACFIX_SubStruct *ISAC_inst;
  Bitstr_dec streamdata;
  uint16_t partOfStream[5];
#ifndef WEBRTC_ARCH_BIG_ENDIAN
  int k;
#endif
  int16_t err;

  
  streamdata.stream = (uint16_t *)partOfStream;

  
  ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;

  
  if (packet_size <= 0) {
    
    ISAC_inst->errorcode = ISAC_EMPTY_PACKET;
    return -1;
  } else if (packet_size > (STREAM_MAXW16<<1)) {
    
    ISAC_inst->errorcode = ISAC_LENGTH_MISMATCH;
    return -1;
  }

  
  if ((ISAC_inst->initflag & 1) != 1) {
    ISAC_inst->errorcode = ISAC_DECODER_NOT_INITIATED;
    return (-1);
  }

  streamdata.W_upper = 0xFFFFFFFF;
  streamdata.streamval = 0;
  streamdata.stream_index = 0;
  streamdata.full = 1;

#ifndef WEBRTC_ARCH_BIG_ENDIAN
  for (k=0; k<5; k++) {
    streamdata.stream[k] = (uint16_t) ((encoded[k] >> 8)|((encoded[k] & 0xFF)<<8));
  }
#else
  memcpy(streamdata.stream, encoded, 5);
#endif

  err = WebRtcIsacfix_EstimateBandwidth(&ISAC_inst->bwestimator_obj,
                                        &streamdata,
                                        packet_size,
                                        rtp_seq_number,
                                        send_ts,
                                        arr_ts);

  if (err < 0)
  {
    
    ISAC_inst->errorcode = -err;
    return -1;
  }


  return 0;
}





















int16_t WebRtcIsacfix_Decode(ISACFIX_MainStruct *ISAC_main_inst,
                             const uint16_t   *encoded,
                             int16_t          len,
                             int16_t          *decoded,
                             int16_t     *speechType)
{
  ISACFIX_SubStruct *ISAC_inst;
  
  
  int16_t     number_of_samples;
#ifndef WEBRTC_ARCH_BIG_ENDIAN
  int k;
#endif
  int16_t declen = 0;

  
  ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;

  
  if ((ISAC_inst->initflag & 1) != 1) {
    ISAC_inst->errorcode = ISAC_DECODER_NOT_INITIATED;
    return (-1);
  }

  
  if (len <= 0) {
    
    ISAC_inst->errorcode = ISAC_EMPTY_PACKET;
    return -1;
  } else if (len > (STREAM_MAXW16<<1)) {
    
    ISAC_inst->errorcode = ISAC_LENGTH_MISMATCH;
    return -1;
  }

  (ISAC_inst->ISACdec_obj.bitstr_obj).stream = (uint16_t *)encoded;

  
#ifndef WEBRTC_ARCH_BIG_ENDIAN
  for (k=0; k<(len>>1); k++) {
    (ISAC_inst->ISACdec_obj.bitstr_obj).stream[k] = (uint16_t) ((encoded[k] >> 8)|((encoded[k] & 0xFF)<<8));
  }
  if (len & 0x0001)
    (ISAC_inst->ISACdec_obj.bitstr_obj).stream[k] = (uint16_t) ((encoded[k] & 0xFF)<<8);
#endif

  
  *speechType=1;

  declen = WebRtcIsacfix_DecodeImpl(decoded,&ISAC_inst->ISACdec_obj, &number_of_samples);

  if (declen < 0) {
    
    ISAC_inst->errorcode = -declen;
    memset(decoded, 0, sizeof(int16_t) * MAX_FRAMESAMPLES);
    return -1;
  }

  

  if (declen & 0x0001) {
    if (len != declen && len != declen + (((ISAC_inst->ISACdec_obj.bitstr_obj).stream[declen>>1]) & 0x00FF) ) {
      ISAC_inst->errorcode = ISAC_LENGTH_MISMATCH;
      memset(decoded, 0, sizeof(int16_t) * number_of_samples);
      return -1;
    }
  } else {
    if (len != declen && len != declen + (((ISAC_inst->ISACdec_obj.bitstr_obj).stream[declen>>1]) >> 8) ) {
      ISAC_inst->errorcode = ISAC_LENGTH_MISMATCH;
      memset(decoded, 0, sizeof(int16_t) * number_of_samples);
      return -1;
    }
  }

  return number_of_samples;
}


























#ifdef WEBRTC_ISAC_FIX_NB_CALLS_ENABLED
int16_t WebRtcIsacfix_DecodeNb(ISACFIX_MainStruct *ISAC_main_inst,
                               const uint16_t   *encoded,
                               int16_t          len,
                               int16_t          *decoded,
                               int16_t    *speechType)
{
  ISACFIX_SubStruct *ISAC_inst;
  
  
  int16_t     number_of_samples;
#ifndef WEBRTC_ARCH_BIG_ENDIAN
  int k;
#endif
  int16_t declen = 0;
  int16_t dummy[FRAMESAMPLES/2];


  
  ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;

  
  if ((ISAC_inst->initflag & 1) != 1) {
    ISAC_inst->errorcode = ISAC_DECODER_NOT_INITIATED;
    return (-1);
  }

  if (len == 0)
  {  

    ISAC_inst->errorcode = ISAC_EMPTY_PACKET;
    return -1;
  }

  (ISAC_inst->ISACdec_obj.bitstr_obj).stream = (uint16_t *)encoded;

  
#ifndef WEBRTC_ARCH_BIG_ENDIAN
  for (k=0; k<(len>>1); k++) {
    (ISAC_inst->ISACdec_obj.bitstr_obj).stream[k] = (uint16_t) ((encoded[k] >> 8)|((encoded[k] & 0xFF)<<8));
  }
  if (len & 0x0001)
    (ISAC_inst->ISACdec_obj.bitstr_obj).stream[k] = (uint16_t) ((encoded[k] & 0xFF)<<8);
#endif

  
  *speechType=1;

  declen = WebRtcIsacfix_DecodeImpl(decoded,&ISAC_inst->ISACdec_obj, &number_of_samples);

  if (declen < 0) {
    
    ISAC_inst->errorcode = -declen;
    memset(decoded, 0, sizeof(int16_t) * FRAMESAMPLES);
    return -1;
  }

  

  if (declen & 0x0001) {
    if (len != declen && len != declen + (((ISAC_inst->ISACdec_obj.bitstr_obj).stream[declen>>1]) & 0x00FF) ) {
      ISAC_inst->errorcode = ISAC_LENGTH_MISMATCH;
      memset(decoded, 0, sizeof(int16_t) * number_of_samples);
      return -1;
    }
  } else {
    if (len != declen && len != declen + (((ISAC_inst->ISACdec_obj.bitstr_obj).stream[declen>>1]) >> 8) ) {
      ISAC_inst->errorcode = ISAC_LENGTH_MISMATCH;
      memset(decoded, 0, sizeof(int16_t) * number_of_samples);
      return -1;
    }
  }

  WebRtcIsacfix_SplitAndFilter2(decoded, decoded, dummy, &ISAC_inst->ISACdec_obj.decimatorstr_obj);

  if (number_of_samples>FRAMESAMPLES) {
    WebRtcIsacfix_SplitAndFilter2(decoded + FRAMESAMPLES, decoded + FRAMESAMPLES/2,
                                  dummy, &ISAC_inst->ISACdec_obj.decimatorstr_obj);
  }

  return number_of_samples/2;
}
#endif 






















#ifdef WEBRTC_ISAC_FIX_NB_CALLS_ENABLED
int16_t WebRtcIsacfix_DecodePlcNb(ISACFIX_MainStruct *ISAC_main_inst,
                                  int16_t          *decoded,
                                  int16_t noOfLostFrames )
{
  int16_t no_of_samples, declen, k, ok;
  int16_t outframeNB[FRAMESAMPLES];
  int16_t outframeWB[FRAMESAMPLES];
  int16_t dummy[FRAMESAMPLES/2];


  ISACFIX_SubStruct *ISAC_inst;
  
  ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;

  
  if (noOfLostFrames > 2){
    noOfLostFrames = 2;
  }

  k = 0;
  declen = 0;
  while( noOfLostFrames > 0 )
  {
    ok = WebRtcIsacfix_DecodePlcImpl( outframeWB, &ISAC_inst->ISACdec_obj, &no_of_samples );
    if(ok)
      return -1;

    WebRtcIsacfix_SplitAndFilter2(outframeWB, &(outframeNB[k*240]), dummy, &ISAC_inst->ISACdec_obj.decimatorstr_obj);

    declen += no_of_samples;
    noOfLostFrames--;
    k++;
  }

  declen>>=1;

  for (k=0;k<declen;k++) {
    decoded[k] = outframeNB[k];
  }

  return declen;
}
#endif 























int16_t WebRtcIsacfix_DecodePlc(ISACFIX_MainStruct *ISAC_main_inst,
                                int16_t          *decoded,
                                int16_t noOfLostFrames)
{

  int16_t no_of_samples, declen, k, ok;
  int16_t outframe16[MAX_FRAMESAMPLES];

  ISACFIX_SubStruct *ISAC_inst;
  
  ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;

  
  if (noOfLostFrames > 2) {
    noOfLostFrames = 2;
  }
  k = 0;
  declen = 0;
  while( noOfLostFrames > 0 )
  {
    ok = WebRtcIsacfix_DecodePlcImpl( &(outframe16[k*480]), &ISAC_inst->ISACdec_obj, &no_of_samples );
    if(ok)
      return -1;
    declen += no_of_samples;
    noOfLostFrames--;
    k++;
  }

  for (k=0;k<declen;k++) {
    decoded[k] = outframe16[k];
  }

  return declen;
}


















int16_t WebRtcIsacfix_Control(ISACFIX_MainStruct *ISAC_main_inst,
                              int16_t          rate,
                              int16_t          framesize)
{
  ISACFIX_SubStruct *ISAC_inst;
  
  ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;

  if (ISAC_inst->CodingMode == 0)
  {
    
    ISAC_inst->errorcode = ISAC_MODE_MISMATCH;
    return -1;
  }


  if (rate >= 10000 && rate <= 32000)
    ISAC_inst->ISACenc_obj.BottleNeck = rate;
  else {
    ISAC_inst->errorcode = ISAC_DISALLOWED_BOTTLENECK;
    return -1;
  }



  if (framesize  == 30 || framesize == 60)
    ISAC_inst->ISACenc_obj.new_framelength = (FS/1000) * framesize;
  else {
    ISAC_inst->errorcode = ISAC_DISALLOWED_FRAME_LENGTH;
    return -1;
  }

  return 0;
}

























int16_t WebRtcIsacfix_ControlBwe(ISACFIX_MainStruct *ISAC_main_inst,
                                 int16_t rateBPS,
                                 int16_t frameSizeMs,
                                 int16_t enforceFrameSize)
{
  ISACFIX_SubStruct *ISAC_inst;
  
  ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;

  
  if ((ISAC_inst->initflag & 2) != 2) {
    ISAC_inst->errorcode = ISAC_ENCODER_NOT_INITIATED;
    return (-1);
  }

  
  if (ISAC_inst->CodingMode != 0) {
    ISAC_inst->errorcode = ISAC_MODE_MISMATCH;
    return (-1);
  }

  
  
  ISAC_inst->ISACenc_obj.enforceFrameSize = (enforceFrameSize != 0)? 1:0;

  
  
  if ((rateBPS >= 10000) && (rateBPS <= 32000)) {
    ISAC_inst->bwestimator_obj.sendBwAvg = (((uint32_t)rateBPS) << 7);
  } else if (rateBPS != 0) {
    ISAC_inst->errorcode = ISAC_DISALLOWED_BOTTLENECK;
    return -1;
  }

  
  if ((frameSizeMs  == 30) || (frameSizeMs == 60)) {
    ISAC_inst->ISACenc_obj.new_framelength = (FS/1000) * frameSizeMs;
  } else {
    ISAC_inst->errorcode = ISAC_DISALLOWED_FRAME_LENGTH;
    return -1;
  }

  return 0;
}



















int16_t WebRtcIsacfix_GetDownLinkBwIndex(ISACFIX_MainStruct* ISAC_main_inst,
                                         int16_t*     rateIndex)
{
  ISACFIX_SubStruct *ISAC_inst;

  
  ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;

  
  *rateIndex = WebRtcIsacfix_GetDownlinkBwIndexImpl(&ISAC_inst->bwestimator_obj);

  return 0;
}














int16_t WebRtcIsacfix_UpdateUplinkBw(ISACFIX_MainStruct* ISAC_main_inst,
                                     int16_t     rateIndex)
{
  int16_t err = 0;
  ISACFIX_SubStruct *ISAC_inst;

  
  ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;

  
  err = WebRtcIsacfix_UpdateUplinkBwRec(&ISAC_inst->bwestimator_obj, rateIndex);
  if (err < 0) {
    ISAC_inst->errorcode = -err;
    return (-1);
  }

  return 0;
}














int16_t WebRtcIsacfix_ReadFrameLen(const int16_t* encoded,
                                   int16_t* frameLength)
{
  Bitstr_dec streamdata;
  uint16_t partOfStream[5];
#ifndef WEBRTC_ARCH_BIG_ENDIAN
  int k;
#endif
  int16_t err;

  
  streamdata.stream = (uint16_t *)partOfStream;

  streamdata.W_upper = 0xFFFFFFFF;
  streamdata.streamval = 0;
  streamdata.stream_index = 0;
  streamdata.full = 1;

#ifndef WEBRTC_ARCH_BIG_ENDIAN
  for (k=0; k<5; k++) {
    streamdata.stream[k] = (uint16_t) (((uint16_t)encoded[k] >> 8)|((encoded[k] & 0xFF)<<8));
  }
#else
  memcpy(streamdata.stream, encoded, 5);
#endif

  
  err = WebRtcIsacfix_DecodeFrameLen(&streamdata, frameLength);
  if (err<0)  
    return err;

  return 0;
}
















int16_t WebRtcIsacfix_ReadBwIndex(const int16_t* encoded,
                                  int16_t* rateIndex)
{
  Bitstr_dec streamdata;
  uint16_t partOfStream[5];
#ifndef WEBRTC_ARCH_BIG_ENDIAN
  int k;
#endif
  int16_t err;

  
  streamdata.stream = (uint16_t *)partOfStream;

  streamdata.W_upper = 0xFFFFFFFF;
  streamdata.streamval = 0;
  streamdata.stream_index = 0;
  streamdata.full = 1;

#ifndef WEBRTC_ARCH_BIG_ENDIAN
  for (k=0; k<5; k++) {
    streamdata.stream[k] = (uint16_t) (((uint16_t)encoded[k] >> 8)|((encoded[k] & 0xFF)<<8));
  }
#else
  memcpy(streamdata.stream, encoded, 5);
#endif

  
  err = WebRtcIsacfix_DecodeFrameLen(&streamdata, rateIndex);
  if (err<0)  
    return err;

  
  err = WebRtcIsacfix_DecodeSendBandwidth(&streamdata, rateIndex);
  if (err<0)  
    return err;

  return 0;
}


















int16_t WebRtcIsacfix_GetErrorCode(ISACFIX_MainStruct *ISAC_main_inst)
{
  ISACFIX_SubStruct *ISAC_inst;
  
  ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;

  return ISAC_inst->errorcode;
}














int32_t WebRtcIsacfix_GetUplinkBw(ISACFIX_MainStruct *ISAC_main_inst)
{
  ISACFIX_SubStruct *ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;
  BwEstimatorstr * bw = (BwEstimatorstr*)&(ISAC_inst->bwestimator_obj);

  return (int32_t) WebRtcIsacfix_GetUplinkBandwidth(bw);
}












int16_t WebRtcIsacfix_GetNewFrameLen(ISACFIX_MainStruct *ISAC_main_inst)
{
  ISACFIX_SubStruct *ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;
  return ISAC_inst->ISACenc_obj.new_framelength;
}




















int16_t WebRtcIsacfix_SetMaxPayloadSize(ISACFIX_MainStruct *ISAC_main_inst,
                                        int16_t maxPayloadBytes)
{
  ISACFIX_SubStruct *ISAC_inst;

  
  ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;

  if((maxPayloadBytes < 100) || (maxPayloadBytes > 400))
  {
    
    return -1;
  }
  else
  {
    

    ISAC_inst->ISACenc_obj.maxPayloadBytes = maxPayloadBytes;

    
    if (maxPayloadBytes < ISAC_inst->ISACenc_obj.maxRateInBytes) {
      ISAC_inst->ISACenc_obj.payloadLimitBytes30 = maxPayloadBytes;
    } else {
      ISAC_inst->ISACenc_obj.payloadLimitBytes30 = ISAC_inst->ISACenc_obj.maxRateInBytes;
    }

    if ( maxPayloadBytes < (ISAC_inst->ISACenc_obj.maxRateInBytes << 1)) {
      ISAC_inst->ISACenc_obj.payloadLimitBytes60 = maxPayloadBytes;
    } else {
      ISAC_inst->ISACenc_obj.payloadLimitBytes60 = (ISAC_inst->ISACenc_obj.maxRateInBytes << 1);
    }
  }
  return 0;
}

























int16_t WebRtcIsacfix_SetMaxRate(ISACFIX_MainStruct *ISAC_main_inst,
                                 int32_t maxRate)
{
  ISACFIX_SubStruct *ISAC_inst;
  int16_t maxRateInBytes;

  
  ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;

  if((maxRate < 32000) || (maxRate > 53400))
  {
    
    return -1;
  }
  else
  {
    



    maxRateInBytes = (int16_t)( WebRtcSpl_DivW32W16ResW16(WEBRTC_SPL_MUL(maxRate, 3), 800) );

    
    ISAC_inst->ISACenc_obj.maxRateInBytes = maxRateInBytes;

    

    if (maxRateInBytes < ISAC_inst->ISACenc_obj.maxPayloadBytes) {
      ISAC_inst->ISACenc_obj.payloadLimitBytes30 = maxRateInBytes;
    } else {
      ISAC_inst->ISACenc_obj.payloadLimitBytes30 = ISAC_inst->ISACenc_obj.maxPayloadBytes;
    }

    

    if ( (maxRateInBytes << 1) < ISAC_inst->ISACenc_obj.maxPayloadBytes) {
      ISAC_inst->ISACenc_obj.payloadLimitBytes60 = (maxRateInBytes << 1);
    } else {
      ISAC_inst->ISACenc_obj.payloadLimitBytes60 = ISAC_inst->ISACenc_obj.maxPayloadBytes;
    }
  }

  return 0;
}













void WebRtcIsacfix_version(char *version)
{
  strcpy(version, "3.6.0");
}
