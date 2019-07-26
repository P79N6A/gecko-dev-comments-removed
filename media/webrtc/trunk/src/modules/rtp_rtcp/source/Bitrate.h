









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_BITRATE_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_BITRATE_H_

#include "typedefs.h"
#include "rtp_rtcp_config.h"     
#include "common_types.h"            
#include <stdio.h>
#include <list>

namespace webrtc {
class RtpRtcpClock;

class Bitrate
{
public:
    Bitrate(RtpRtcpClock* clock);

    
    void Process();

    
    void Update(const WebRtc_Word32 bytes);

    
    WebRtc_UWord32 PacketRate() const;

    
    WebRtc_UWord32 BitrateLast() const;

    
    WebRtc_UWord32 BitrateNow() const;

protected:
  RtpRtcpClock& _clock;

private:
  WebRtc_UWord32 _packetRate;
  WebRtc_UWord32 _bitrate;
  WebRtc_UWord8 _bitrateNextIdx;
  WebRtc_Word64 _packetRateArray[10];
  WebRtc_Word64 _bitrateArray[10];
  WebRtc_Word64 _bitrateDiffMS[10];
  WebRtc_Word64 _timeLastRateUpdate;
  WebRtc_UWord32 _bytesCount;
  WebRtc_UWord32 _packetCount;
};

}  

#endif 
