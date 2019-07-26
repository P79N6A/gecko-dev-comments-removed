









#ifndef WEBRTC_MODULES_VIDEO_CODING_PACKET_H_
#define WEBRTC_MODULES_VIDEO_CODING_PACKET_H_

#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/modules/video_coding/main/source/jitter_buffer_common.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class VCMPacket {
public:
    VCMPacket();
    VCMPacket(const uint8_t* ptr,
              const uint32_t size,
              const WebRtcRTPHeader& rtpHeader);
    VCMPacket(const uint8_t* ptr,
              uint32_t size,
              uint16_t seqNum,
              uint32_t timestamp,
              bool markerBit);

    void Reset();

    uint8_t           payloadType;
    uint32_t          timestamp;
    uint16_t          seqNum;
    const uint8_t*    dataPtr;
    uint32_t          sizeBytes;
    bool                    markerBit;

    FrameType               frameType;
    webrtc::VideoCodecType  codec;

    bool isFirstPacket;                 
    VCMNaluCompleteness completeNALU;   
    bool insertStartCode;               
                                        
    int width;
    int height;
    RTPVideoHeader codecSpecificHeader;

protected:
    void CopyCodecSpecifics(const RTPVideoHeader& videoHeader);
};

}  
#endif 
