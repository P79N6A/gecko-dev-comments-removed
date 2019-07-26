













#include "typedefs.h"

#ifndef WEBRTC_NETEQ_INTERNAL_H
#define WEBRTC_NETEQ_INTERNAL_H

#ifdef __cplusplus 
extern "C"
{
#endif

typedef struct
{
    uint8_t payloadType;
    uint16_t sequenceNumber;
    uint32_t timeStamp;
    uint32_t SSRC;
    uint8_t markerBit;
} WebRtcNetEQ_RTPInfo;

















int WebRtcNetEQ_RecInRTPStruct(void *inst, WebRtcNetEQ_RTPInfo *rtpInfo,
                               const uint8_t *payloadPtr, int16_t payloadLenBytes,
                               uint32_t timeRec);











int WebRtcNetEQ_GetMasterSlaveInfoSize();
























int WebRtcNetEQ_RecOutMasterSlave(void *inst, int16_t *pw16_outData,
                                  int16_t *pw16_len, void *msInfo,
                                  int16_t isMaster);

typedef struct
{
    uint16_t currentBufferSize;         
    uint16_t preferredBufferSize;       
    uint16_t jitterPeaksFound;          

    uint16_t currentPacketLossRate;     
    uint16_t currentDiscardRate;        
    uint16_t currentExpandRate;         


    uint16_t currentPreemptiveRate;     

    uint16_t currentAccelerateRate;     

    int32_t clockDriftPPM;              

    int addedSamples;                   

} WebRtcNetEQ_NetworkStatistics;





int WebRtcNetEQ_GetNetworkStatistics(void *inst, WebRtcNetEQ_NetworkStatistics *stats);


typedef struct {
  
  int accelerate_bgn_samples;

  
  int accelerate_normal_samples;

  
  int expand_bgn_sampels;

  
  int expand_normal_samples;

  

  int preemptive_expand_bgn_samples;

  

  int preemptive_expand_normal_samples;

  

  int merge_expand_bgn_samples;

  

  int merge_expand_normal_samples;
} WebRtcNetEQ_ProcessingActivity;







void WebRtcNetEQ_GetProcessingActivity(void* inst,
                                       WebRtcNetEQ_ProcessingActivity* stat);








int WebRtcNetEQ_GetRawFrameWaitingTimes(void *inst,
                                        int max_length,
                                        int* waiting_times_ms);













typedef int (*WebRtcNetEQ_VADInitFunction)(void *VAD_inst);
typedef int (*WebRtcNetEQ_VADSetmodeFunction)(void *VAD_inst, int mode);
typedef int (*WebRtcNetEQ_VADFunction)(void *VAD_inst, int fs,
    int16_t *frame, int frameLen);

























int WebRtcNetEQ_SetVADInstance(void *NetEQ_inst, void *VAD_inst,
                               WebRtcNetEQ_VADInitFunction initFunction,
                               WebRtcNetEQ_VADSetmodeFunction setmodeFunction,
                               WebRtcNetEQ_VADFunction VADFunction);


















int WebRtcNetEQ_SetVADMode(void *NetEQ_inst, int mode);


















int WebRtcNetEQ_RecOutNoDecode(void *inst, int16_t *pw16_outData,
                               int16_t *pw16_len);

















int WebRtcNetEQ_FlushBuffers(void *inst);
















void WebRtcNetEQ_EnableAVSync(void* inst, int enable);


















int WebRtcNetEQ_RecInSyncRTP(void* inst,
                             WebRtcNetEQ_RTPInfo* rtp_info,
                             uint32_t receive_timestamp);






int WebRtcNetEQ_SetMinimumDelay(void *inst, int minimum_delay_ms);






int WebRtcNetEQ_SetMaximumDelay(void *inst, int maximum_delay_ms);





int WebRtcNetEQ_GetRequiredDelayMs(const void* inst);

#ifdef __cplusplus
}
#endif

#endif
