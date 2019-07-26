









#ifndef MODULES_AUDIO_CODING_CODECS_G722_MAIN_INTERFACE_G722_INTERFACE_H_
#define MODULES_AUDIO_CODING_CODECS_G722_MAIN_INTERFACE_G722_INTERFACE_H_

#include "typedefs.h"





typedef struct WebRtcG722EncInst    G722EncInst;
typedef struct WebRtcG722DecInst    G722DecInst;





#define G722_WEBRTC_SPEECH     1
#define G722_WEBRTC_CNG        2

#ifdef __cplusplus
extern "C" {
#endif













WebRtc_Word16 WebRtcG722_CreateEncoder(G722EncInst **G722enc_inst);















WebRtc_Word16 WebRtcG722_EncoderInit(G722EncInst *G722enc_inst);













WebRtc_Word16 WebRtcG722_FreeEncoder(G722EncInst *G722enc_inst);





















WebRtc_Word16 WebRtcG722_Encode(G722EncInst *G722enc_inst,
                                WebRtc_Word16 *speechIn,
                                WebRtc_Word16 len,
                                WebRtc_Word16 *encoded);













WebRtc_Word16 WebRtcG722_CreateDecoder(G722DecInst **G722dec_inst);















WebRtc_Word16 WebRtcG722_DecoderInit(G722DecInst *G722dec_inst);














WebRtc_Word16 WebRtcG722_FreeDecoder(G722DecInst *G722dec_inst);























WebRtc_Word16 WebRtcG722_Decode(G722DecInst *G722dec_inst,
                                WebRtc_Word16 *encoded,
                                WebRtc_Word16 len,
                                WebRtc_Word16 *decoded,
                                WebRtc_Word16 *speechType);







WebRtc_Word16 WebRtcG722_Version(char *versionStr, short len);


#ifdef __cplusplus
}
#endif


#endif
