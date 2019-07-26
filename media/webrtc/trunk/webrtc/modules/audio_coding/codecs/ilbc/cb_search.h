

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_CB_SEARCH_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_CB_SEARCH_H_

void WebRtcIlbcfix_CbSearch(
    iLBC_Enc_Inst_t *iLBCenc_inst,
    
    WebRtc_Word16 *index,  
    WebRtc_Word16 *gain_index, 
    WebRtc_Word16 *intarget, 
    WebRtc_Word16 *decResidual,
    WebRtc_Word16 lMem,  
    WebRtc_Word16 lTarget,  
    WebRtc_Word16 *weightDenum,
    WebRtc_Word16 block  
                            );

#endif
