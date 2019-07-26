

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_PACK_BITS_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_PACK_BITS_H_

#include "defines.h"





void WebRtcIlbcfix_PackBits( 
    uint16_t *bitstream,   
    iLBC_bits *enc_bits,  
    int16_t mode     
                             );

#endif
