













#ifndef PACKET_BUFFER_H
#define PACKET_BUFFER_H

#include "typedefs.h"

#include "webrtc_neteq.h"
#include "rtp.h"


#define PBUFFER_MIN_MEMORY_SIZE	150





typedef struct
{

    
    WebRtc_UWord16 packSizeSamples; 
    WebRtc_Word16 *startPayloadMemory; 
    int memorySizeW16; 
    WebRtc_Word16 *currentMemoryPos; 
    int numPacketsInBuffer; 
    int insertPosition; 
    int maxInsertPositions; 

    
    

    WebRtc_UWord32 *timeStamp; 
    WebRtc_Word16 **payloadLocation; 
    WebRtc_UWord16 *seqNumber; 
    WebRtc_Word16 *payloadType; 
    WebRtc_Word16 *payloadLengthBytes; 
    WebRtc_Word16 *rcuPlCntr; 

    int *waitingTime;


    
    WebRtc_UWord16 discardedPackets; 

} PacketBuf_t;























int WebRtcNetEQ_PacketBufferInit(PacketBuf_t *bufferInst, int maxNoOfPackets,
                                 WebRtc_Word16 *pw16_memory, int memorySize);















int WebRtcNetEQ_PacketBufferFlush(PacketBuf_t *bufferInst);



















int WebRtcNetEQ_PacketBufferInsert(PacketBuf_t *bufferInst, const RTPPacket_t *RTPpacket,
                                   WebRtc_Word16 *flushed);



















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
                                          int codec_pos, int last_duration);

















WebRtc_Word32 WebRtcNetEQ_PacketBufferGetSize(const PacketBuf_t* buffer_inst,
                                              const CodecDbInst_t*
                                              codec_database);












void WebRtcNetEQ_IncrementWaitingTimes(PacketBuf_t *buffer_inst);


















int WebRtcNetEQ_GetDefaultCodecSettings(const enum WebRtcNetEQDecoder *codecID,
                                        int noOfCodecs, int *maxBytes, int *maxSlots);

#endif 
