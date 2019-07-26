











#include <stdlib.h>
#include <string.h>
#include "g722_interface.h"
#include "g722_enc_dec.h"
#include "typedefs.h"


int16_t WebRtcG722_CreateEncoder(G722EncInst **G722enc_inst)
{
    *G722enc_inst=(G722EncInst*)malloc(sizeof(g722_encode_state_t));
    if (*G722enc_inst!=NULL) {
      return(0);
    } else {
      return(-1);
    }
}

int16_t WebRtcG722_EncoderInit(G722EncInst *G722enc_inst)
{
    
    
    G722enc_inst = (G722EncInst *) WebRtc_g722_encode_init(
        (g722_encode_state_t*) G722enc_inst, 64000, 2);
    if (G722enc_inst == NULL) {
        return -1;
    } else {
        return 0;
    }
}

int16_t WebRtcG722_FreeEncoder(G722EncInst *G722enc_inst)
{
    
    return WebRtc_g722_encode_release((g722_encode_state_t*) G722enc_inst);
}

int16_t WebRtcG722_Encode(G722EncInst *G722enc_inst,
                          int16_t *speechIn,
                          int16_t len,
                          int16_t *encoded)
{
    unsigned char *codechar = (unsigned char*) encoded;
    
    return WebRtc_g722_encode((g722_encode_state_t*) G722enc_inst,
                       codechar, speechIn, len);
}

int16_t WebRtcG722_CreateDecoder(G722DecInst **G722dec_inst)
{
    *G722dec_inst=(G722DecInst*)malloc(sizeof(g722_decode_state_t));
    if (*G722dec_inst!=NULL) {
      return(0);
    } else {
      return(-1);
    }
}

int16_t WebRtcG722_DecoderInit(G722DecInst *G722dec_inst)
{
    
    
    G722dec_inst = (G722DecInst *) WebRtc_g722_decode_init(
        (g722_decode_state_t*) G722dec_inst, 64000, 2);
    if (G722dec_inst == NULL) {
        return -1;
    } else {
        return 0;
    }
}

int16_t WebRtcG722_FreeDecoder(G722DecInst *G722dec_inst)
{
    
    return WebRtc_g722_decode_release((g722_decode_state_t*) G722dec_inst);
}

int16_t WebRtcG722_Decode(G722DecInst *G722dec_inst,
                          int16_t *encoded,
                          int16_t len,
                          int16_t *decoded,
                          int16_t *speechType)
{
    
    *speechType=G722_WEBRTC_SPEECH;
    return WebRtc_g722_decode((g722_decode_state_t*) G722dec_inst,
                              decoded, (uint8_t*) encoded, len);
}

int16_t WebRtcG722_Version(char *versionStr, short len)
{
    
    char version[30] = "2.0.0\n";
    if (strlen(version) < (unsigned int)len)
    {
        strcpy(versionStr, version);
        return 0;
    }
    else
    {
        return -1;
    }
}

