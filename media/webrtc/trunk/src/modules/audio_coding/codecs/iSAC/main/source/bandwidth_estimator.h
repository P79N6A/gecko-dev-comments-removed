

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_BANDWIDTH_ESTIMATOR_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_BANDWIDTH_ESTIMATOR_H_

#include "structs.h"
#include "settings.h"


#define MIN_ISAC_BW     10000
#define MIN_ISAC_BW_LB  10000
#define MIN_ISAC_BW_UB  25000

#define MAX_ISAC_BW     56000
#define MAX_ISAC_BW_UB  32000
#define MAX_ISAC_BW_LB  32000

#define MIN_ISAC_MD     5
#define MAX_ISAC_MD     25



#define HEADER_SIZE        35


#define INIT_FRAME_LEN_WB  60
#define INIT_FRAME_LEN_SWB 30



#define INIT_BN_EST_WB     20e3f
#define INIT_BN_EST_SWB    56e3f



#define INIT_HDR_RATE_WB                                                \
  ((float)HEADER_SIZE * 8.0f * 1000.0f / (float)INIT_FRAME_LEN_WB)
#define INIT_HDR_RATE_SWB                                               \
  ((float)HEADER_SIZE * 8.0f * 1000.0f / (float)INIT_FRAME_LEN_SWB)


#define BURST_LEN       3


#define BURST_INTERVAL  500


#define INIT_BURST_LEN  5


#define INIT_RATE_WB       INIT_BN_EST_WB
#define INIT_RATE_SWB      INIT_BN_EST_SWB


#if defined(__cplusplus)
extern "C" {
#endif




  WebRtc_Word32 WebRtcIsac_InitBandwidthEstimator(
      BwEstimatorstr*           bwest_str,
      enum IsacSamplingRate encoderSampRate,
      enum IsacSamplingRate decoderSampRate);

  
  
  
  
  
  
  
  
  
  WebRtc_Word16 WebRtcIsac_UpdateBandwidthEstimator(
      BwEstimatorstr*    bwest_str,
      const WebRtc_UWord16 rtp_number,
      const WebRtc_Word32  frame_length,
      const WebRtc_UWord32 send_ts,
      const WebRtc_UWord32 arr_ts,
      const WebRtc_Word32  pksize);

  
  WebRtc_Word16 WebRtcIsac_UpdateUplinkBwImpl(
      BwEstimatorstr*           bwest_str,
      WebRtc_Word16               Index,
      enum IsacSamplingRate encoderSamplingFreq);

  
  WebRtc_UWord16 WebRtcIsac_GetDownlinkBwJitIndexImpl(
      BwEstimatorstr*           bwest_str,
      WebRtc_Word16*              bottleneckIndex,
      WebRtc_Word16*              jitterInfo,
      enum IsacSamplingRate decoderSamplingFreq);

  
  WebRtc_Word32 WebRtcIsac_GetDownlinkBandwidth(
      const BwEstimatorstr *bwest_str);

  
  WebRtc_Word32 WebRtcIsac_GetDownlinkMaxDelay(
      const BwEstimatorstr *bwest_str);

  
  void WebRtcIsac_GetUplinkBandwidth(
      const BwEstimatorstr* bwest_str,
      WebRtc_Word32*          bitRate);

  
  WebRtc_Word32 WebRtcIsac_GetUplinkMaxDelay(
      const BwEstimatorstr *bwest_str);


  



  int WebRtcIsac_GetMinBytes(
      RateModel*         State,
      int                StreamSize,    
      const int          FrameLen,      
      const double       BottleNeck,    
      const double       DelayBuildUp,  
      enum ISACBandwidth bandwidth
      );

  


  void WebRtcIsac_UpdateRateModel(
      RateModel*   State,
      int          StreamSize,                
      const int    FrameSamples,        
      const double BottleNeck);       


  void WebRtcIsac_InitRateModel(
      RateModel *State);

  
  int WebRtcIsac_GetNewFrameLength(
      double bottle_neck,
      int    current_framelength);

  
  double WebRtcIsac_GetSnr(
      double bottle_neck,
      int    new_framelength);


  WebRtc_Word16 WebRtcIsac_UpdateUplinkJitter(
      BwEstimatorstr*              bwest_str,
      WebRtc_Word32                  index);

#if defined(__cplusplus)
}
#endif


#endif
