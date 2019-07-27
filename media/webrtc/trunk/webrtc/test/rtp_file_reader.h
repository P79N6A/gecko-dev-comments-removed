








#ifndef WEBRTC_TEST_RTP_FILE_READER_H_
#define WEBRTC_TEST_RTP_FILE_READER_H_

#include <string>

#include "webrtc/common_types.h"

namespace webrtc {
namespace test {
class RtpFileReader {
 public:
  enum FileFormat {
    kPcap,
    kRtpDump,
  };

  struct Packet {
    
    
    static const size_t kMaxPacketBufferSize = 3500;
    uint8_t data[kMaxPacketBufferSize];
    size_t length;
    
    
    size_t original_length;

    uint32_t time_ms;
  };

  virtual ~RtpFileReader() {}
  static RtpFileReader* Create(FileFormat format,
                               const std::string& filename);

  virtual bool NextPacket(Packet* packet) = 0;
};
}  
}  
#endif
