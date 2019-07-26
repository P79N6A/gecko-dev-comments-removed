









#ifndef WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_TOOLS_BWE_RTP_H_
#define WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_TOOLS_BWE_RTP_H_

#include <string>

namespace webrtc {
class Clock;
class RemoteBitrateEstimator;
class RemoteBitrateObserver;
class RtpHeaderParser;
namespace rtpplayer {
class RtpPacketSourceInterface;
}
}

bool ParseArgsAndSetupEstimator(
    int argc,
    char** argv,
    webrtc::Clock* clock,
    webrtc::RemoteBitrateObserver* observer,
    webrtc::rtpplayer::RtpPacketSourceInterface** rtp_reader,
    webrtc::RtpHeaderParser** parser,
    webrtc::RemoteBitrateEstimator** estimator,
    std::string* estimator_used);

#endif  
