









#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_INTERFACE_ISAC_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_INTERFACE_ISAC_H_




#include "typedefs.h"

typedef struct WebRtcISACStruct    ISACStruct;

#if defined(__cplusplus)
extern "C" {
#endif


















  WebRtc_Word16 WebRtcIsac_AssignSize(
      int* sizeinbytes);


  














  WebRtc_Word16 WebRtcIsac_Assign(
      ISACStruct** ISAC_main_inst,
      void*        ISAC_inst_Addr);


  












  WebRtc_Word16 WebRtcIsac_Create(
      ISACStruct** ISAC_main_inst);


  











  WebRtc_Word16 WebRtcIsac_Free(
      ISACStruct* ISAC_main_inst);


  


















  WebRtc_Word16 WebRtcIsac_EncoderInit(
      ISACStruct* ISAC_main_inst,
      WebRtc_Word16 CodingMode);


  























  WebRtc_Word16 WebRtcIsac_Encode(
      ISACStruct*        ISAC_main_inst,
      const WebRtc_Word16* speechIn,
      WebRtc_Word16*       encoded);


  












  WebRtc_Word16 WebRtcIsac_DecoderInit(
      ISACStruct* ISAC_main_inst);


  

















  WebRtc_Word16 WebRtcIsac_UpdateBwEstimate(
      ISACStruct*         ISAC_main_inst,
      const WebRtc_UWord16* encoded,
      WebRtc_Word32         packet_size,
      WebRtc_UWord16        rtp_seq_number,
      WebRtc_UWord32        send_ts,
      WebRtc_UWord32        arr_ts);


  



















  WebRtc_Word16 WebRtcIsac_Decode(
      ISACStruct*           ISAC_main_inst,
      const WebRtc_UWord16* encoded,
      WebRtc_Word16         len,
      WebRtc_Word16*        decoded,
      WebRtc_Word16*        speechType);


  


















  WebRtc_Word16 WebRtcIsac_DecodePlc(
      ISACStruct*  ISAC_main_inst,
      WebRtc_Word16* decoded,
      WebRtc_Word16  noOfLostFrames);


  


















  WebRtc_Word16 WebRtcIsac_Control(
      ISACStruct*   ISAC_main_inst,
      WebRtc_Word32 rate,
      WebRtc_Word16 framesize);


  
























  WebRtc_Word16 WebRtcIsac_ControlBwe(
      ISACStruct* ISAC_main_inst,
      WebRtc_Word32 rateBPS,
      WebRtc_Word16 frameSizeMs,
      WebRtc_Word16 enforceFrameSize);


  












  WebRtc_Word16 WebRtcIsac_ReadFrameLen(
      ISACStruct*          ISAC_main_inst,
      const WebRtc_Word16* encoded,
      WebRtc_Word16*       frameLength);


  









  void WebRtcIsac_version(
      char *version);


  













  WebRtc_Word16 WebRtcIsac_GetErrorCode(
      ISACStruct* ISAC_main_inst);


  























  WebRtc_Word16 WebRtcIsac_GetUplinkBw(
      ISACStruct*    ISAC_main_inst,
      WebRtc_Word32* bottleneck);


  


































  WebRtc_Word16 WebRtcIsac_SetMaxPayloadSize(
      ISACStruct* ISAC_main_inst,
      WebRtc_Word16 maxPayloadBytes);


  






































  WebRtc_Word16 WebRtcIsac_SetMaxRate(
      ISACStruct* ISAC_main_inst,
      WebRtc_Word32 maxRate);


  










  WebRtc_UWord16 WebRtcIsac_DecSampRate(ISACStruct* ISAC_main_inst);


  









  WebRtc_UWord16 WebRtcIsac_EncSampRate(ISACStruct* ISAC_main_inst);


  













  WebRtc_Word16 WebRtcIsac_SetDecSampRate(ISACStruct* ISAC_main_inst,
                                          WebRtc_UWord16 samp_rate_hz);


  















  WebRtc_Word16 WebRtcIsac_SetEncSampRate(ISACStruct* ISAC_main_inst,
                                          WebRtc_UWord16 sample_rate_hz);



  


































  WebRtc_Word16 WebRtcIsac_GetNewBitStream(
      ISACStruct*    ISAC_main_inst,
      WebRtc_Word16  bweIndex,
      WebRtc_Word16  jitterInfo,
      WebRtc_Word32  rate,
      WebRtc_Word16* encoded,
      WebRtc_Word16  isRCU);



  













  WebRtc_Word16 WebRtcIsac_GetDownLinkBwIndex(
      ISACStruct*  ISAC_main_inst,
      WebRtc_Word16* bweIndex,
      WebRtc_Word16* jitterInfo);


  











  WebRtc_Word16 WebRtcIsac_UpdateUplinkBw(
      ISACStruct* ISAC_main_inst,
      WebRtc_Word16 bweIndex);


  













  WebRtc_Word16 WebRtcIsac_ReadBwIndex(
      const WebRtc_Word16* encoded,
      WebRtc_Word16*       bweIndex);



  















  WebRtc_Word16 WebRtcIsac_GetNewFrameLen(
      ISACStruct* ISAC_main_inst);


  




















  WebRtc_Word16 WebRtcIsac_GetRedPayload(
      ISACStruct*    ISAC_main_inst,
      WebRtc_Word16* encoded);


  


















  WebRtc_Word16 WebRtcIsac_DecodeRcu(
      ISACStruct*           ISAC_main_inst,
      const WebRtc_UWord16* encoded,
      WebRtc_Word16         len,
      WebRtc_Word16*        decoded,
      WebRtc_Word16*        speechType);


#if defined(__cplusplus)
}
#endif



#endif
