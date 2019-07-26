

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_UNPACK_BITS_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_UNPACK_BITS_H_

#include "defines.h"





WebRtc_Word16 WebRtcIlbcfix_UnpackBits( 
    const WebRtc_UWord16 *bitstream,    
    iLBC_bits *enc_bits,  
    WebRtc_Word16 mode     
                                        );

#endif
