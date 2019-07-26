









#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_INTERFACE_ISAC_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_INTERFACE_ISAC_H_




#include "webrtc/typedefs.h"

typedef struct WebRtcISACStruct    ISACStruct;

#if defined(__cplusplus)
extern "C" {
#endif


















  int16_t WebRtcIsac_AssignSize(
      int* sizeinbytes);


  














  int16_t WebRtcIsac_Assign(
      ISACStruct** ISAC_main_inst,
      void*        ISAC_inst_Addr);


  












  int16_t WebRtcIsac_Create(
      ISACStruct** ISAC_main_inst);


  











  int16_t WebRtcIsac_Free(
      ISACStruct* ISAC_main_inst);


  


















  int16_t WebRtcIsac_EncoderInit(
      ISACStruct* ISAC_main_inst,
      int16_t CodingMode);


  























  int16_t WebRtcIsac_Encode(
      ISACStruct*        ISAC_main_inst,
      const int16_t* speechIn,
      int16_t*       encoded);


  












  int16_t WebRtcIsac_DecoderInit(
      ISACStruct* ISAC_main_inst);


  

















  int16_t WebRtcIsac_UpdateBwEstimate(
      ISACStruct*         ISAC_main_inst,
      const uint16_t* encoded,
      int32_t         packet_size,
      uint16_t        rtp_seq_number,
      uint32_t        send_ts,
      uint32_t        arr_ts);


  



















  int16_t WebRtcIsac_Decode(
      ISACStruct*           ISAC_main_inst,
      const uint16_t* encoded,
      int16_t         len,
      int16_t*        decoded,
      int16_t*        speechType);


  


















  int16_t WebRtcIsac_DecodePlc(
      ISACStruct*  ISAC_main_inst,
      int16_t* decoded,
      int16_t  noOfLostFrames);


  


















  int16_t WebRtcIsac_Control(
      ISACStruct*   ISAC_main_inst,
      int32_t rate,
      int16_t framesize);


  
























  int16_t WebRtcIsac_ControlBwe(
      ISACStruct* ISAC_main_inst,
      int32_t rateBPS,
      int16_t frameSizeMs,
      int16_t enforceFrameSize);


  












  int16_t WebRtcIsac_ReadFrameLen(
      ISACStruct*          ISAC_main_inst,
      const int16_t* encoded,
      int16_t*       frameLength);


  









  void WebRtcIsac_version(
      char *version);


  













  int16_t WebRtcIsac_GetErrorCode(
      ISACStruct* ISAC_main_inst);


  























  int16_t WebRtcIsac_GetUplinkBw(
      ISACStruct*    ISAC_main_inst,
      int32_t* bottleneck);


  


































  int16_t WebRtcIsac_SetMaxPayloadSize(
      ISACStruct* ISAC_main_inst,
      int16_t maxPayloadBytes);


  






































  int16_t WebRtcIsac_SetMaxRate(
      ISACStruct* ISAC_main_inst,
      int32_t maxRate);


  










  uint16_t WebRtcIsac_DecSampRate(ISACStruct* ISAC_main_inst);


  









  uint16_t WebRtcIsac_EncSampRate(ISACStruct* ISAC_main_inst);


  













  int16_t WebRtcIsac_SetDecSampRate(ISACStruct* ISAC_main_inst,
                                          uint16_t samp_rate_hz);


  















  int16_t WebRtcIsac_SetEncSampRate(ISACStruct* ISAC_main_inst,
                                          uint16_t sample_rate_hz);



  


































  int16_t WebRtcIsac_GetNewBitStream(
      ISACStruct*    ISAC_main_inst,
      int16_t  bweIndex,
      int16_t  jitterInfo,
      int32_t  rate,
      int16_t* encoded,
      int16_t  isRCU);



  













  int16_t WebRtcIsac_GetDownLinkBwIndex(
      ISACStruct*  ISAC_main_inst,
      int16_t* bweIndex,
      int16_t* jitterInfo);


  











  int16_t WebRtcIsac_UpdateUplinkBw(
      ISACStruct* ISAC_main_inst,
      int16_t bweIndex);


  













  int16_t WebRtcIsac_ReadBwIndex(
      const int16_t* encoded,
      int16_t*       bweIndex);



  















  int16_t WebRtcIsac_GetNewFrameLen(
      ISACStruct* ISAC_main_inst);


  




















  int16_t WebRtcIsac_GetRedPayload(
      ISACStruct*    ISAC_main_inst,
      int16_t* encoded);


  


















  int16_t WebRtcIsac_DecodeRcu(
      ISACStruct*           ISAC_main_inst,
      const uint16_t* encoded,
      int16_t         len,
      int16_t*        decoded,
      int16_t*        speechType);


#if defined(__cplusplus)
}
#endif



#endif
