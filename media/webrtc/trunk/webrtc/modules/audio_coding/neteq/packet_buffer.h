













#ifndef PACKET_BUFFER_H
#define PACKET_BUFFER_H

#include "typedefs.h"

#include "webrtc_neteq.h"
#include "rtp.h"


#define PBUFFER_MIN_MEMORY_SIZE	150





typedef struct
{

    
    uint16_t packSizeSamples; 
    int16_t *startPayloadMemory; 
    int memorySizeW16; 
    int16_t *currentMemoryPos; 
    int numPacketsInBuffer; 
    int insertPosition; 
    int maxInsertPositions; 

    
    

    uint32_t *timeStamp; 
    int16_t **payloadLocation; 
    uint16_t *seqNumber; 
    int16_t *payloadType; 
    int16_t *payloadLengthBytes; 
    int16_t *rcuPlCntr; 

    int *waitingTime;

    
    uint16_t discardedPackets; 

} PacketBuf_t;























int WebRtcNetEQ_PacketBufferInit(PacketBuf_t *bufferInst, int maxNoOfPackets,
                                 int16_t *pw16_memory, int memorySize);















int WebRtcNetEQ_PacketBufferFlush(PacketBuf_t *bufferInst);




















int WebRtcNetEQ_PacketBufferInsert(PacketBuf_t *bufferInst, const RTPPacket_t *RTPpacket,
                                   int16_t *flushed, int av_sync);



















int WebRtcNetEQ_PacketBufferExtract(PacketBuf_t *bufferInst, RTPPacket_t *RTPpacket,
                                    int bufferPosition, int *waitingTime);





















int WebRtcNetEQ_PacketBufferFindLowestTimestamp(PacketBuf_t* buffer_inst,
                                                uint32_t current_time_stamp,
                                                uint32_t* time_stamp,
                                                int* buffer_position,
                                                int erase_old_packets,
                                                int16_t* payload_type);






















int WebRtcNetEQ_PacketBufferGetPacketSize(const PacketBuf_t* buffer_inst,
                                          int buffer_pos,
                                          const CodecDbInst_t* codec_database,
                                          int codec_pos, int last_duration,
                                          int av_sync);


















int32_t WebRtcNetEQ_PacketBufferGetSize(const PacketBuf_t* buffer_inst,
                                        const CodecDbInst_t* codec_database,
                                        int av_sync);












void WebRtcNetEQ_IncrementWaitingTimes(PacketBuf_t *buffer_inst);



















int WebRtcNetEQ_GetDefaultCodecSettings(const enum WebRtcNetEQDecoder *codecID,
                                        int noOfCodecs, int *maxBytes,
                                        int *maxSlots,
                                        int* per_slot_overhead_bytes);

#endif 
