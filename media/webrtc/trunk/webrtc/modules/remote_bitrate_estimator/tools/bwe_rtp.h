









#ifndef WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_TOOLS_BWE_RTP_H_
#define WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_TOOLS_BWE_RTP_H_

#include <string>

namespace webrtc {
class Clock;
class RemoteBitrateEstimator;
class RemoteBitrateObserver;
class RtpHeaderParser;
namespace test {
class RtpFileReader;
}
}

bool ParseArgsAndSetupEstimator(
    int argc,
    char** argv,
    webrtc::Clock* clock,
    webrtc::RemoteBitrateObserver* observer,
    webrtc::test::RtpFileReader** rtp_reader,
    webrtc::RtpHeaderParser** parser,
    webrtc::RemoteBitrateEstimator** estimator,
    std::string* estimator_used);

#endif  
