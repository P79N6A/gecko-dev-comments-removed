

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_CB_MEM_ENERGY_AUGMENTATION_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_CB_MEM_ENERGY_AUGMENTATION_H_

void WebRtcIlbcfix_CbMemEnergyAugmentation(
    int16_t *interpSamples, 
    int16_t *CBmem,   
    int16_t scale,   
    int16_t base_size,  
    int16_t *energyW16,  
    int16_t *energyShifts 
                                           );

#endif
