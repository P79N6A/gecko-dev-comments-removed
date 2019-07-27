









#ifndef MODULES_AUDIO_CODING_CODECS_G722_MAIN_INTERFACE_G722_INTERFACE_H_
#define MODULES_AUDIO_CODING_CODECS_G722_MAIN_INTERFACE_G722_INTERFACE_H_

#include "webrtc/typedefs.h"





typedef struct WebRtcG722EncInst    G722EncInst;
typedef struct WebRtcG722DecInst    G722DecInst;





#define G722_WEBRTC_SPEECH     1
#define G722_WEBRTC_CNG        2

#ifdef __cplusplus
extern "C" {
#endif













int16_t WebRtcG722_CreateEncoder(G722EncInst **G722enc_inst);















int16_t WebRtcG722_EncoderInit(G722EncInst *G722enc_inst);













int16_t WebRtcG722_FreeEncoder(G722EncInst *G722enc_inst);





















int16_t WebRtcG722_Encode(G722EncInst *G722enc_inst,
                          int16_t *speechIn,
                          int16_t len,
                          int16_t *encoded);













int16_t WebRtcG722_CreateDecoder(G722DecInst **G722dec_inst);















int16_t WebRtcG722_DecoderInit(G722DecInst *G722dec_inst);














int16_t WebRtcG722_FreeDecoder(G722DecInst *G722dec_inst);























int16_t WebRtcG722_Decode(G722DecInst *G722dec_inst,
                          int16_t *encoded,
                          int16_t len,
                          int16_t *decoded,
                          int16_t *speechType);







int16_t WebRtcG722_Version(char *versionStr, short len);


#ifdef __cplusplus
}
#endif


#endif
