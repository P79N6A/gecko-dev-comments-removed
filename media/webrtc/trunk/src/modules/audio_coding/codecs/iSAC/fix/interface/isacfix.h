









#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_INTERFACE_ISACFIX_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_INTERFACE_ISACFIX_H_




#include "typedefs.h"


typedef struct {
  void *dummy;
} ISACFIX_MainStruct;


#if defined(__cplusplus)
extern "C" {
#endif










  WebRtc_Word16 WebRtcIsacfix_AssignSize(int *sizeinbytes);

  














  WebRtc_Word16 WebRtcIsacfix_Assign(ISACFIX_MainStruct **inst,
                                     void *ISACFIX_inst_Addr);

  












  WebRtc_Word16 WebRtcIsacfix_Create(ISACFIX_MainStruct **ISAC_main_inst);


  











  WebRtc_Word16 WebRtcIsacfix_Free(ISACFIX_MainStruct *ISAC_main_inst);


  

















  WebRtc_Word16 WebRtcIsacfix_EncoderInit(ISACFIX_MainStruct *ISAC_main_inst,
                                          WebRtc_Word16  CodingMode);


  




















  WebRtc_Word16 WebRtcIsacfix_Encode(ISACFIX_MainStruct *ISAC_main_inst,
                                     const WebRtc_Word16 *speechIn,
                                     WebRtc_Word16 *encoded);



  
























#ifdef WEBRTC_ISAC_FIX_NB_CALLS_ENABLED
  WebRtc_Word16 WebRtcIsacfix_EncodeNb(ISACFIX_MainStruct *ISAC_main_inst,
                                       const WebRtc_Word16 *speechIn,
                                       WebRtc_Word16 *encoded);
#endif 



  












  WebRtc_Word16 WebRtcIsacfix_DecoderInit(ISACFIX_MainStruct *ISAC_main_inst);


  
















  WebRtc_Word16 WebRtcIsacfix_UpdateBwEstimate1(ISACFIX_MainStruct *ISAC_main_inst,
                                                const WebRtc_UWord16 *encoded,
                                                WebRtc_Word32  packet_size,
                                                WebRtc_UWord16 rtp_seq_number,
                                                WebRtc_UWord32 arr_ts);

  


















  WebRtc_Word16 WebRtcIsacfix_UpdateBwEstimate(ISACFIX_MainStruct *ISAC_main_inst,
                                               const WebRtc_UWord16   *encoded,
                                               WebRtc_Word32          packet_size,
                                               WebRtc_UWord16         rtp_seq_number,
                                               WebRtc_UWord32         send_ts,
                                               WebRtc_UWord32         arr_ts);

  


















  WebRtc_Word16 WebRtcIsacfix_Decode(ISACFIX_MainStruct *ISAC_main_inst,
                                     const WebRtc_UWord16 *encoded,
                                     WebRtc_Word16 len,
                                     WebRtc_Word16 *decoded,
                                     WebRtc_Word16 *speechType);


  




















#ifdef WEBRTC_ISAC_FIX_NB_CALLS_ENABLED
  WebRtc_Word16 WebRtcIsacfix_DecodeNb(ISACFIX_MainStruct *ISAC_main_inst,
                                       const WebRtc_UWord16 *encoded,
                                       WebRtc_Word16 len,
                                       WebRtc_Word16 *decoded,
                                       WebRtc_Word16 *speechType);
#endif 


  




















#ifdef WEBRTC_ISAC_FIX_NB_CALLS_ENABLED
  WebRtc_Word16 WebRtcIsacfix_DecodePlcNb(ISACFIX_MainStruct *ISAC_main_inst,
                                          WebRtc_Word16 *decoded,
                                          WebRtc_Word16 noOfLostFrames );
#endif 




  



















  WebRtc_Word16 WebRtcIsacfix_DecodePlc(ISACFIX_MainStruct *ISAC_main_inst,
                                        WebRtc_Word16 *decoded,
                                        WebRtc_Word16 noOfLostFrames );


  












  WebRtc_Word16 WebRtcIsacfix_ReadFrameLen(const WebRtc_Word16* encoded,
                                           WebRtc_Word16* frameLength);

  















  WebRtc_Word16 WebRtcIsacfix_Control(ISACFIX_MainStruct *ISAC_main_inst,
                                      WebRtc_Word16          rate,
                                      WebRtc_Word16          framesize);



  





















  WebRtc_Word16 WebRtcIsacfix_ControlBwe(ISACFIX_MainStruct *ISAC_main_inst,
                                         WebRtc_Word16 rateBPS,
                                         WebRtc_Word16 frameSizeMs,
                                         WebRtc_Word16 enforceFrameSize);



  









  void WebRtcIsacfix_version(char *version);


  













  WebRtc_Word16 WebRtcIsacfix_GetErrorCode(ISACFIX_MainStruct *ISAC_main_inst);


  











  WebRtc_Word32 WebRtcIsacfix_GetUplinkBw(ISACFIX_MainStruct *ISAC_main_inst);


  

















  WebRtc_Word16 WebRtcIsacfix_SetMaxPayloadSize(ISACFIX_MainStruct *ISAC_main_inst,
                                                WebRtc_Word16 maxPayloadBytes);


  






















  WebRtc_Word16 WebRtcIsacfix_SetMaxRate(ISACFIX_MainStruct *ISAC_main_inst,
                                         WebRtc_Word32 maxRate);

  











  WebRtc_Word16 WebRtcIsacfix_CreateInternal(ISACFIX_MainStruct *ISAC_main_inst);


  











  WebRtc_Word16 WebRtcIsacfix_FreeInternal(ISACFIX_MainStruct *ISAC_main_inst);


  


















  WebRtc_Word16 WebRtcIsacfix_GetNewBitStream(ISACFIX_MainStruct *ISAC_main_inst,
                                              WebRtc_Word16          bweIndex,
                                              float              scale,
                                              WebRtc_Word16        *encoded);


  













  WebRtc_Word16 WebRtcIsacfix_GetDownLinkBwIndex(ISACFIX_MainStruct* ISAC_main_inst,
                                                 WebRtc_Word16*     rateIndex);


  











  WebRtc_Word16 WebRtcIsacfix_UpdateUplinkBw(ISACFIX_MainStruct* ISAC_main_inst,
                                             WebRtc_Word16     rateIndex);


  












  WebRtc_Word16 WebRtcIsacfix_ReadBwIndex(const WebRtc_Word16* encoded,
                                          WebRtc_Word16* rateIndex);


  










  WebRtc_Word16 WebRtcIsacfix_GetNewFrameLen(ISACFIX_MainStruct *ISAC_main_inst);


#if defined(__cplusplus)
}
#endif



#endif
