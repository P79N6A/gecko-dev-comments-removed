









#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_INTERFACE_ISACFIX_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_INTERFACE_ISACFIX_H_




#include "webrtc/typedefs.h"

typedef struct {
  void *dummy;
} ISACFIX_MainStruct;


#if defined(__cplusplus)
extern "C" {
#endif










  int16_t WebRtcIsacfix_AssignSize(int *sizeinbytes);

  














  int16_t WebRtcIsacfix_Assign(ISACFIX_MainStruct **inst,
                                     void *ISACFIX_inst_Addr);

  












  int16_t WebRtcIsacfix_Create(ISACFIX_MainStruct **ISAC_main_inst);


  











  int16_t WebRtcIsacfix_Free(ISACFIX_MainStruct *ISAC_main_inst);


  

















  int16_t WebRtcIsacfix_EncoderInit(ISACFIX_MainStruct *ISAC_main_inst,
                                    int16_t  CodingMode);


  




















  int16_t WebRtcIsacfix_Encode(ISACFIX_MainStruct *ISAC_main_inst,
                               const int16_t *speechIn,
                               uint8_t* encoded);



  
























#ifdef WEBRTC_ISAC_FIX_NB_CALLS_ENABLED
  int16_t WebRtcIsacfix_EncodeNb(ISACFIX_MainStruct *ISAC_main_inst,
                                 const int16_t *speechIn,
                                 int16_t *encoded);
#endif 



  












  int16_t WebRtcIsacfix_DecoderInit(ISACFIX_MainStruct *ISAC_main_inst);


  
















  int16_t WebRtcIsacfix_UpdateBwEstimate1(ISACFIX_MainStruct *ISAC_main_inst,
                                          const uint8_t* encoded,
                                          int32_t  packet_size,
                                          uint16_t rtp_seq_number,
                                          uint32_t arr_ts);

  


















  int16_t WebRtcIsacfix_UpdateBwEstimate(ISACFIX_MainStruct *ISAC_main_inst,
                                         const uint8_t* encoded,
                                         int32_t          packet_size,
                                         uint16_t         rtp_seq_number,
                                         uint32_t         send_ts,
                                         uint32_t         arr_ts);

  


















  int16_t WebRtcIsacfix_Decode(ISACFIX_MainStruct *ISAC_main_inst,
                               const uint8_t* encoded,
                               int16_t len,
                               int16_t *decoded,
                               int16_t *speechType);


  




















#ifdef WEBRTC_ISAC_FIX_NB_CALLS_ENABLED
  int16_t WebRtcIsacfix_DecodeNb(ISACFIX_MainStruct *ISAC_main_inst,
                                 const uint16_t *encoded,
                                 int16_t len,
                                 int16_t *decoded,
                                 int16_t *speechType);
#endif 


  




















#ifdef WEBRTC_ISAC_FIX_NB_CALLS_ENABLED
  int16_t WebRtcIsacfix_DecodePlcNb(ISACFIX_MainStruct *ISAC_main_inst,
                                    int16_t *decoded,
                                    int16_t noOfLostFrames);
#endif 




  



















  int16_t WebRtcIsacfix_DecodePlc(ISACFIX_MainStruct *ISAC_main_inst,
                                  int16_t *decoded,
                                  int16_t noOfLostFrames );


  













  int16_t WebRtcIsacfix_ReadFrameLen(const uint8_t* encoded,
                                     int encoded_len_bytes,
                                     int16_t* frameLength);

  















  int16_t WebRtcIsacfix_Control(ISACFIX_MainStruct *ISAC_main_inst,
                                int16_t          rate,
                                int16_t          framesize);



  





















  int16_t WebRtcIsacfix_ControlBwe(ISACFIX_MainStruct *ISAC_main_inst,
                                   int16_t rateBPS,
                                   int16_t frameSizeMs,
                                   int16_t enforceFrameSize);



  









  void WebRtcIsacfix_version(char *version);


  













  int16_t WebRtcIsacfix_GetErrorCode(ISACFIX_MainStruct *ISAC_main_inst);


  











  int32_t WebRtcIsacfix_GetUplinkBw(ISACFIX_MainStruct *ISAC_main_inst);


  

















  int16_t WebRtcIsacfix_SetMaxPayloadSize(ISACFIX_MainStruct *ISAC_main_inst,
                                          int16_t maxPayloadBytes);


  






















  int16_t WebRtcIsacfix_SetMaxRate(ISACFIX_MainStruct *ISAC_main_inst,
                                   int32_t maxRate);

  











  int16_t WebRtcIsacfix_CreateInternal(ISACFIX_MainStruct *ISAC_main_inst);


  











  int16_t WebRtcIsacfix_FreeInternal(ISACFIX_MainStruct *ISAC_main_inst);


  


















  int16_t WebRtcIsacfix_GetNewBitStream(ISACFIX_MainStruct *ISAC_main_inst,
                                        int16_t          bweIndex,
                                        float              scale,
                                        uint8_t* encoded);


  













  int16_t WebRtcIsacfix_GetDownLinkBwIndex(ISACFIX_MainStruct* ISAC_main_inst,
                                           int16_t*     rateIndex);


  











  int16_t WebRtcIsacfix_UpdateUplinkBw(ISACFIX_MainStruct* ISAC_main_inst,
                                       int16_t     rateIndex);


  













  int16_t WebRtcIsacfix_ReadBwIndex(const uint8_t* encoded,
                                    int encoded_len_bytes,
                                    int16_t* rateIndex);


  










  int16_t WebRtcIsacfix_GetNewFrameLen(ISACFIX_MainStruct *ISAC_main_inst);


#if defined(__cplusplus)
}
#endif



#endif
