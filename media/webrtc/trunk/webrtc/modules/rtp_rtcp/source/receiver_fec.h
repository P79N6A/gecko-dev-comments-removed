









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_RECEIVER_FEC_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_RECEIVER_FEC_H_

#include "rtp_rtcp_defines.h"

#include "forward_error_correction.h"

#include "typedefs.h"

namespace webrtc {
class RTPReceiverVideo;

class ReceiverFEC
{
public:
    ReceiverFEC(const int32_t id, RTPReceiverVideo* owner);
    virtual ~ReceiverFEC();

    int32_t AddReceivedFECPacket(const WebRtcRTPHeader* rtpHeader,
                                 const uint8_t* incomingRtpPacket,
                                 const uint16_t payloadDataLength,
                                 bool& FECpacket);

    int32_t ProcessReceivedFEC();

    void SetPayloadTypeFEC(const int8_t payloadType);

private:
    int ParseAndReceivePacket(const ForwardErrorCorrection::Packet* packet);

    int _id;
    RTPReceiverVideo* _owner;
    ForwardErrorCorrection* _fec;
    
    
    
    ForwardErrorCorrection::ReceivedPacketList _receivedPacketList;
    ForwardErrorCorrection::RecoveredPacketList _recoveredPacketList;
    int8_t _payloadTypeFEC;
};
} 

#endif 
