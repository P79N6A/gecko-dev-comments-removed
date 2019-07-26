

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_BANDWIDTH_ESTIMATOR_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_BANDWIDTH_ESTIMATOR_H_

#include "structs.h"













int32_t WebRtcIsacfix_InitBandwidthEstimator(BwEstimatorstr *bwest_str);























int32_t WebRtcIsacfix_UpdateUplinkBwImpl(BwEstimatorstr       *bwest_str,
                                         const uint16_t        rtp_number,
                                         const int16_t         frameSize,
                                         const uint32_t        send_ts,
                                         const uint32_t        arr_ts,
                                         const int16_t         pksize,
                                         const uint16_t        Index);


int16_t WebRtcIsacfix_UpdateUplinkBwRec(BwEstimatorstr *bwest_str,
                                        const int16_t Index);













uint16_t WebRtcIsacfix_GetDownlinkBwIndexImpl(BwEstimatorstr *bwest_str);


uint16_t WebRtcIsacfix_GetDownlinkBandwidth(const BwEstimatorstr *bwest_str);


int16_t WebRtcIsacfix_GetUplinkBandwidth(const BwEstimatorstr *bwest_str);


int16_t WebRtcIsacfix_GetDownlinkMaxDelay(const BwEstimatorstr *bwest_str);


int16_t WebRtcIsacfix_GetUplinkMaxDelay(const BwEstimatorstr *bwest_str);





uint16_t WebRtcIsacfix_GetMinBytes(RateModel *State,
                                   int16_t StreamSize,     
                                   const int16_t FrameLen,    
                                   const int16_t BottleNeck,        
                                   const int16_t DelayBuildUp);     




void WebRtcIsacfix_UpdateRateModel(RateModel *State,
                                   int16_t StreamSize,    
                                   const int16_t FrameSamples,  
                                   const int16_t BottleNeck);       


void WebRtcIsacfix_InitRateModel(RateModel *State);


int16_t WebRtcIsacfix_GetNewFrameLength(int16_t bottle_neck, int16_t current_framelength);



int16_t WebRtcIsacfix_GetSnr(int16_t bottle_neck, int16_t framesamples);


#endif 
