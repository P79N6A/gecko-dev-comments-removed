









#ifndef WEBRTC_MODULES_VIDEO_CODING_MAIN_TEST_RTP_FILE_READER_H_
#define WEBRTC_MODULES_VIDEO_CODING_MAIN_TEST_RTP_FILE_READER_H_

#include <string>

namespace webrtc {
namespace rtpplayer {

class RtpPacketSourceInterface;

RtpPacketSourceInterface* CreateRtpFileReader(const std::string& filename);

}  
}  

#endif  
