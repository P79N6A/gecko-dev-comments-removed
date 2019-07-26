









#ifndef WEBRTC_MODULES_VIDEO_CODING_TEST_PCAP_FILE_READER_H_
#define WEBRTC_MODULES_VIDEO_CODING_TEST_PCAP_FILE_READER_H_

#include <string>

namespace webrtc {
namespace rtpplayer {

class RtpPacketSourceInterface;

RtpPacketSourceInterface* CreatePcapFileReader(const std::string& filename);

}  
}  

#endif  
