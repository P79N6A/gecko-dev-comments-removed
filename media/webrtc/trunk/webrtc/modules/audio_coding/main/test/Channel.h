









#ifndef CHANNEL_H
#define CHANNEL_H

#include <stdio.h>

#include "audio_coding_module.h"
#include "critical_section_wrapper.h"
#include "rw_lock_wrapper.h"
#include "webrtc/modules/interface/module_common_types.h"

namespace webrtc {

#define MAX_NUM_PAYLOADS   50
#define MAX_NUM_FRAMESIZES  6


struct ACMTestFrameSizeStats
{
    WebRtc_UWord16 frameSizeSample;
    WebRtc_Word16  maxPayloadLen;
    WebRtc_UWord32 numPackets;
    WebRtc_UWord64 totalPayloadLenByte;
    WebRtc_UWord64 totalEncodedSamples;
    double         rateBitPerSec;
    double         usageLenSec;

};

struct ACMTestPayloadStats
{
    bool                  newPacket;
    WebRtc_Word16         payloadType;
    WebRtc_Word16         lastPayloadLenByte;
    WebRtc_UWord32        lastTimestamp;
    ACMTestFrameSizeStats frameSizeStats[MAX_NUM_FRAMESIZES];
};

class Channel: public AudioPacketizationCallback
{
public:

    Channel(
        WebRtc_Word16 chID = -1);
    ~Channel();

    WebRtc_Word32 SendData(
        const FrameType       frameType,
        const WebRtc_UWord8   payloadType,
        const WebRtc_UWord32  timeStamp,
        const WebRtc_UWord8*  payloadData,
        const WebRtc_UWord16  payloadSize,
        const RTPFragmentationHeader* fragmentation);

    void RegisterReceiverACM(
        AudioCodingModule *acm);

    void ResetStats();

    WebRtc_Word16 Stats(
        CodecInst&           codecInst,
        ACMTestPayloadStats& payloadStats);

    void Stats(
        WebRtc_UWord32* numPackets);

    void Stats(
        WebRtc_UWord8*  payloadLenByte,
        WebRtc_UWord32* payloadType);

    void PrintStats(
        CodecInst& codecInst);

    void SetIsStereo(bool isStereo)
    {
        _isStereo = isStereo;
    }

    WebRtc_UWord32 LastInTimestamp();

    void SetFECTestWithPacketLoss(bool usePacketLoss)
    {
        _useFECTestWithPacketLoss = usePacketLoss;
    }

    double BitRate();

private:
    void CalcStatistics(
        WebRtcRTPHeader& rtpInfo,
        WebRtc_UWord16   payloadSize);

    AudioCodingModule*      _receiverACM;
    WebRtc_UWord16          _seqNo;
    
    WebRtc_UWord8           _payloadData[60 * 32 * 2 * 2];

    CriticalSectionWrapper* _channelCritSect;
    FILE*                   _bitStreamFile;
    bool                    _saveBitStream;
    WebRtc_Word16           _lastPayloadType;
    ACMTestPayloadStats     _payloadStats[MAX_NUM_PAYLOADS];
    bool                    _isStereo;
    WebRtcRTPHeader         _rtpInfo;
    bool                    _leftChannel;
    WebRtc_UWord32          _lastInTimestamp;
    
    WebRtc_Word16           _packetLoss;
    bool                    _useFECTestWithPacketLoss;
    WebRtc_UWord64          _beginTime;
    WebRtc_UWord64          _totalBytes;
};

} 

#endif
