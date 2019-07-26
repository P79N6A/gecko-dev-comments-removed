

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_CB_MEM_ENERGY_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_CB_MEM_ENERGY_H_

void WebRtcIlbcfix_CbMemEnergy(
    WebRtc_Word16 range,
    WebRtc_Word16 *CB,   
    WebRtc_Word16 *filteredCB,  
    WebRtc_Word16 lMem,   
    WebRtc_Word16 lTarget,   
    WebRtc_Word16 *energyW16,  
    WebRtc_Word16 *energyShifts, 
    WebRtc_Word16 scale,   
    WebRtc_Word16 base_size  
                               );

#endif
