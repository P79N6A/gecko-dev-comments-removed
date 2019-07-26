

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_CB_SEARCH_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_CB_SEARCH_H_

void WebRtcIlbcfix_CbSearch(
    iLBC_Enc_Inst_t *iLBCenc_inst,
    
    int16_t *index,  
    int16_t *gain_index, 
    int16_t *intarget, 
    int16_t *decResidual,
    int16_t lMem,  
    int16_t lTarget,  
    int16_t *weightDenum,
    int16_t block  
                            );

#endif
