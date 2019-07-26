









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_VIDEO_CODEC_INFORMATION_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_VIDEO_CODEC_INFORMATION_H_

#include "webrtc/modules/rtp_rtcp/source/rtp_rtcp_config.h"
#include "webrtc/modules/rtp_rtcp/source/rtp_utility.h"

namespace webrtc {
class VideoCodecInformation
{
public:
    virtual void Reset() = 0;

    virtual RtpVideoCodecTypes Type() = 0;
    virtual ~VideoCodecInformation(){};
};
}  

#endif 
