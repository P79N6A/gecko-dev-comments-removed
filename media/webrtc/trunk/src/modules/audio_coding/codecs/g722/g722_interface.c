











#include <stdlib.h>
#include <string.h>
#include "g722_interface.h"
#include "g722_enc_dec.h"
#include "typedefs.h"


WebRtc_Word16 WebRtcG722_CreateEncoder(G722EncInst **G722enc_inst)
{
    *G722enc_inst=(G722EncInst*)malloc(sizeof(g722_encode_state_t));
    if (*G722enc_inst!=NULL) {
      return(0);
    } else {
      return(-1);
    }
}

WebRtc_Word16 WebRtcG722_EncoderInit(G722EncInst *G722enc_inst)
{
    
    
    G722enc_inst = (G722EncInst *) WebRtc_g722_encode_init(
        (g722_encode_state_t*) G722enc_inst, 64000, 2);
    if (G722enc_inst == NULL) {
        return -1;
    } else {
        return 0;
    }
}

WebRtc_Word16 WebRtcG722_FreeEncoder(G722EncInst *G722enc_inst)
{
    
    return WebRtc_g722_encode_release((g722_encode_state_t*) G722enc_inst);
}

WebRtc_Word16 WebRtcG722_Encode(G722EncInst *G722enc_inst,
                                WebRtc_Word16 *speechIn,
                                WebRtc_Word16 len,
                                WebRtc_Word16 *encoded)
{
    unsigned char *codechar = (unsigned char*) encoded;
    
    return WebRtc_g722_encode((g722_encode_state_t*) G722enc_inst,
                       codechar, speechIn, len);
}

WebRtc_Word16 WebRtcG722_CreateDecoder(G722DecInst **G722dec_inst)
{
    *G722dec_inst=(G722DecInst*)malloc(sizeof(g722_decode_state_t));
    if (*G722dec_inst!=NULL) {
      return(0);
    } else {
      return(-1);
    }
}

WebRtc_Word16 WebRtcG722_DecoderInit(G722DecInst *G722dec_inst)
{
    
    
    G722dec_inst = (G722DecInst *) WebRtc_g722_decode_init(
        (g722_decode_state_t*) G722dec_inst, 64000, 2);
    if (G722dec_inst == NULL) {
        return -1;
    } else {
        return 0;
    }
}

WebRtc_Word16 WebRtcG722_FreeDecoder(G722DecInst *G722dec_inst)
{
    
    return WebRtc_g722_decode_release((g722_decode_state_t*) G722dec_inst);
}

WebRtc_Word16 WebRtcG722_Decode(G722DecInst *G722dec_inst,
                                WebRtc_Word16 *encoded,
                                WebRtc_Word16 len,
                                WebRtc_Word16 *decoded,
                                WebRtc_Word16 *speechType)
{
    
    *speechType=G722_WEBRTC_SPEECH;
    return WebRtc_g722_decode((g722_decode_state_t*) G722dec_inst,
                              decoded, (WebRtc_UWord8*) encoded, len);
}

WebRtc_Word16 WebRtcG722_Version(char *versionStr, short len)
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

