

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_CB_MEM_ENERGY_CALC_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_CB_MEM_ENERGY_CALC_H_

void WebRtcIlbcfix_CbMemEnergyCalc(
    int32_t energy,   
    int16_t range,   
    int16_t *ppi,   
    int16_t *ppo,   
    int16_t *energyW16,  
    int16_t *energyShifts, 
    int16_t scale,   
    int16_t base_size  
                                   );

#endif
