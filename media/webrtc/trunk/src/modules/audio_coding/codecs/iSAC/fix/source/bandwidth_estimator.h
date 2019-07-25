

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_BANDWIDTH_ESTIMATOR_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_BANDWIDTH_ESTIMATOR_H_

#include "structs.h"













WebRtc_Word32 WebRtcIsacfix_InitBandwidthEstimator(BwEstimatorstr *bwest_str);























WebRtc_Word32 WebRtcIsacfix_UpdateUplinkBwImpl(BwEstimatorstr            *bwest_str,
                                               const WebRtc_UWord16        rtp_number,
                                               const WebRtc_Word16         frameSize,
                                               const WebRtc_UWord32    send_ts,
                                               const WebRtc_UWord32        arr_ts,
                                               const WebRtc_Word16         pksize,
                                               const WebRtc_UWord16        Index);


WebRtc_Word16 WebRtcIsacfix_UpdateUplinkBwRec(BwEstimatorstr *bwest_str,
                                              const WebRtc_Word16 Index);













WebRtc_UWord16 WebRtcIsacfix_GetDownlinkBwIndexImpl(BwEstimatorstr *bwest_str);


WebRtc_UWord16 WebRtcIsacfix_GetDownlinkBandwidth(const BwEstimatorstr *bwest_str);


WebRtc_Word16 WebRtcIsacfix_GetUplinkBandwidth(const BwEstimatorstr *bwest_str);


WebRtc_Word16 WebRtcIsacfix_GetDownlinkMaxDelay(const BwEstimatorstr *bwest_str);


WebRtc_Word16 WebRtcIsacfix_GetUplinkMaxDelay(const BwEstimatorstr *bwest_str);





WebRtc_UWord16 WebRtcIsacfix_GetMinBytes(RateModel *State,
                                         WebRtc_Word16 StreamSize,     
                                         const WebRtc_Word16 FrameLen,    
                                         const WebRtc_Word16 BottleNeck,        
                                         const WebRtc_Word16 DelayBuildUp);     




void WebRtcIsacfix_UpdateRateModel(RateModel *State,
                                   WebRtc_Word16 StreamSize,    
                                   const WebRtc_Word16 FrameSamples,  
                                   const WebRtc_Word16 BottleNeck);       


void WebRtcIsacfix_InitRateModel(RateModel *State);


WebRtc_Word16 WebRtcIsacfix_GetNewFrameLength(WebRtc_Word16 bottle_neck, WebRtc_Word16 current_framelength);



WebRtc_Word16 WebRtcIsacfix_GetSnr(WebRtc_Word16 bottle_neck, WebRtc_Word16 framesamples);


#endif 
