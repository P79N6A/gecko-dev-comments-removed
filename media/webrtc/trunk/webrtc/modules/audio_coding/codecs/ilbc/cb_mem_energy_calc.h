

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_CB_MEM_ENERGY_CALC_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_CB_MEM_ENERGY_CALC_H_

void WebRtcIlbcfix_CbMemEnergyCalc(
    WebRtc_Word32 energy,   
    WebRtc_Word16 range,   
    WebRtc_Word16 *ppi,   
    WebRtc_Word16 *ppo,   
    WebRtc_Word16 *energyW16,  
    WebRtc_Word16 *energyShifts, 
    WebRtc_Word16 scale,   
    WebRtc_Word16 base_size  
                                   );

#endif
