













#ifndef MCU_H
#define MCU_H

#include "typedefs.h"

#include "codec_db.h"
#include "rtcp.h"
#include "packet_buffer.h"
#include "buffer_stats.h"
#include "neteq_statistics.h"

#ifdef NETEQ_ATEVENT_DECODE
#include "dtmf_buffer.h"
#endif

#define MAX_ONE_DESC 5 /* cannot do more than this many consecutive one-descriptor decodings */
#define MAX_LOSS_REPORT_PERIOD 60   /* number of seconds between auto-reset */

enum TsScaling
{
    kTSnoScaling = 0,
    kTSscalingTwo,
    kTSscalingTwoThirds,
    kTSscalingFourThirds
};

enum { kLenWaitingTimes = 100 };

typedef struct
{

    WebRtc_Word16 current_Codec;
    WebRtc_Word16 current_Payload;
    WebRtc_UWord32 timeStamp; 
    WebRtc_Word16 millisecondsPerCall;
    WebRtc_UWord16 timestampsPerCall; 
    WebRtc_UWord16 fs;
    WebRtc_UWord32 ssrc; 
    WebRtc_Word16 new_codec;
    WebRtc_Word16 first_packet;

    
    WebRtc_Word16 *pw16_readAddress;
    WebRtc_Word16 *pw16_writeAddress;
    void *main_inst;

    CodecDbInst_t codec_DB_inst; 


    SplitInfo_t PayloadSplit_inst; 

    WebRtcNetEQ_RTCP_t RTCP_inst; 
    PacketBuf_t PacketBuffer_inst; 
    BufstatsInst_t BufferStat_inst; 

#ifdef NETEQ_ATEVENT_DECODE
    dtmf_inst_t DTMF_inst;
#endif
    int NoOfExpandCalls;
    WebRtc_Word16 AVT_PlayoutOn;
    enum WebRtcNetEQPlayoutMode NetEqPlayoutMode;

    WebRtc_Word16 one_desc; 

    WebRtc_UWord32 lostTS; 
    WebRtc_UWord32 lastReportTS; 

    int waiting_times[kLenWaitingTimes];  
    int len_waiting_times;
    int next_waiting_time_index;

    WebRtc_UWord32 externalTS;
    WebRtc_UWord32 internalTS;
    WebRtc_Word16 TSscalingInitialized;
    enum TsScaling scalingFactor;

#ifdef NETEQ_STEREO
    int usingStereo;
#endif

} MCUInst_t;












int WebRtcNetEQ_McuReset(MCUInst_t *inst);












int WebRtcNetEQ_ResetMcuInCallStats(MCUInst_t *inst);











void WebRtcNetEQ_ResetWaitingTimeStats(MCUInst_t *inst);












void WebRtcNetEQ_StoreWaitingTime(MCUInst_t *inst, int waiting_time);












int WebRtcNetEQ_ResetMcuJitterStat(MCUInst_t *inst);















int WebRtcNetEQ_McuAddressInit(MCUInst_t *inst, void * Data2McuAddress,
                               void * Data2DspAddress, void *main_inst);













int WebRtcNetEQ_McuSetFs(MCUInst_t *inst, WebRtc_UWord16 fs_hz);












int WebRtcNetEQ_SignalMcu(MCUInst_t *inst);















int WebRtcNetEQ_RecInInternal(MCUInst_t *MCU_inst, RTPPacket_t *RTPpacket,
                              WebRtc_UWord32 uw32_timeRec);















int WebRtcNetEQ_SplitAndInsertPayload(RTPPacket_t *packet, PacketBuf_t *Buffer_inst,
                                      SplitInfo_t *split_inst, WebRtc_Word16 *flushed);















int WebRtcNetEQ_GetTimestampScaling(MCUInst_t *MCU_inst, int rtpPayloadType);













WebRtc_UWord32 WebRtcNetEQ_ScaleTimestampExternalToInternal(const MCUInst_t *MCU_inst,
                                                            WebRtc_UWord32 externalTS);













WebRtc_UWord32 WebRtcNetEQ_ScaleTimestampInternalToExternal(const MCUInst_t *MCU_inst,
                                                            WebRtc_UWord32 internalTS);
#endif
