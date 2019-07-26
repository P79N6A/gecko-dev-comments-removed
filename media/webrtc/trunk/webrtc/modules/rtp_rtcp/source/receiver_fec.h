









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
    ReceiverFEC(const WebRtc_Word32 id, RTPReceiverVideo* owner);
    virtual ~ReceiverFEC();

    WebRtc_Word32 AddReceivedFECPacket(const WebRtcRTPHeader* rtpHeader,
                                       const WebRtc_UWord8* incomingRtpPacket,
                                       const WebRtc_UWord16 payloadDataLength,
                                       bool& FECpacket);

    WebRtc_Word32 ProcessReceivedFEC();

    void SetPayloadTypeFEC(const WebRtc_Word8 payloadType);

private:
    int ParseAndReceivePacket(const ForwardErrorCorrection::Packet* packet);

    int _id;
    RTPReceiverVideo* _owner;
    ForwardErrorCorrection* _fec;
    
    
    
    ForwardErrorCorrection::ReceivedPacketList _receivedPacketList;
    ForwardErrorCorrection::RecoveredPacketList _recoveredPacketList;
    WebRtc_Word8 _payloadTypeFEC;
};
} 

#endif 
