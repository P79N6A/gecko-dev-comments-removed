













#include "typedefs.h"

#ifndef WEBRTC_NETEQ_INTERNAL_H
#define WEBRTC_NETEQ_INTERNAL_H

#ifdef __cplusplus 
extern "C"
{
#endif

typedef struct
{
    WebRtc_UWord8 payloadType;
    WebRtc_UWord16 sequenceNumber;
    WebRtc_UWord32 timeStamp;
    WebRtc_UWord32 SSRC;
    WebRtc_UWord8 markerBit;
} WebRtcNetEQ_RTPInfo;

















int WebRtcNetEQ_RecInRTPStruct(void *inst, WebRtcNetEQ_RTPInfo *rtpInfo,
                               const WebRtc_UWord8 *payloadPtr, WebRtc_Word16 payloadLenBytes,
                               WebRtc_UWord32 timeRec);











int WebRtcNetEQ_GetMasterSlaveInfoSize();
























int WebRtcNetEQ_RecOutMasterSlave(void *inst, WebRtc_Word16 *pw16_outData,
                                  WebRtc_Word16 *pw16_len, void *msInfo,
                                  WebRtc_Word16 isMaster);

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








int WebRtcNetEQ_GetRawFrameWaitingTimes(void *inst,
                                        int max_length,
                                        int* waiting_times_ms);













typedef int (*WebRtcNetEQ_VADInitFunction)(void *VAD_inst);
typedef int (*WebRtcNetEQ_VADSetmodeFunction)(void *VAD_inst, int mode);
typedef int (*WebRtcNetEQ_VADFunction)(void *VAD_inst, int fs,
    WebRtc_Word16 *frame, int frameLen);

























int WebRtcNetEQ_SetVADInstance(void *NetEQ_inst, void *VAD_inst,
                               WebRtcNetEQ_VADInitFunction initFunction,
                               WebRtcNetEQ_VADSetmodeFunction setmodeFunction,
                               WebRtcNetEQ_VADFunction VADFunction);


















int WebRtcNetEQ_SetVADMode(void *NetEQ_inst, int mode);


















int WebRtcNetEQ_RecOutNoDecode(void *inst, WebRtc_Word16 *pw16_outData,
                               WebRtc_Word16 *pw16_len);

















int WebRtcNetEQ_FlushBuffers(void *inst);

#ifdef __cplusplus
}
#endif

#endif
