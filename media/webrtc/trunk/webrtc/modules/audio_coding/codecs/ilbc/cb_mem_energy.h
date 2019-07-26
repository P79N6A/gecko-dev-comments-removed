

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_CB_MEM_ENERGY_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_CB_MEM_ENERGY_H_

void WebRtcIlbcfix_CbMemEnergy(
    int16_t range,
    int16_t *CB,   
    int16_t *filteredCB,  
    int16_t lMem,   
    int16_t lTarget,   
    int16_t *energyW16,  
    int16_t *energyShifts, 
    int16_t scale,   
    int16_t base_size  
                               );

#endif
