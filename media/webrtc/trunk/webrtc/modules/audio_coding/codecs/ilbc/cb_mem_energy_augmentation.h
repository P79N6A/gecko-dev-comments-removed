

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_CB_MEM_ENERGY_AUGMENTATION_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_CB_MEM_ENERGY_AUGMENTATION_H_

void WebRtcIlbcfix_CbMemEnergyAugmentation(
    WebRtc_Word16 *interpSamples, 
    WebRtc_Word16 *CBmem,   
    WebRtc_Word16 scale,   
    WebRtc_Word16 base_size,  
    WebRtc_Word16 *energyW16,  
    WebRtc_Word16 *energyShifts 
                                           );

#endif
