









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ_TOOLS_RTP_FILE_SOURCE_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ_TOOLS_RTP_FILE_SOURCE_H_

#include <stdio.h>
#include <string>

#include "webrtc/base/constructormagic.h"
#include "webrtc/common_types.h"
#include "webrtc/modules/audio_coding/neteq/tools/packet_source.h"
#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp_defines.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

class RtpHeaderParser;

namespace test {

class RtpFileReader;

class RtpFileSource : public PacketSource {
 public:
  
  
  static RtpFileSource* Create(const std::string& file_name);

  virtual ~RtpFileSource();

  
  virtual bool RegisterRtpHeaderExtension(RTPExtensionType type, uint8_t id);

  
  
  virtual Packet* NextPacket() OVERRIDE;

 private:
  static const int kFirstLineLength = 40;
  static const int kRtpFileHeaderSize = 4 + 4 + 4 + 2 + 2;
  static const size_t kPacketHeaderSize = 8;

  RtpFileSource();

  bool OpenFile(const std::string& file_name);

  scoped_ptr<RtpFileReader> rtp_reader_;
  scoped_ptr<RtpHeaderParser> parser_;

  DISALLOW_COPY_AND_ASSIGN(RtpFileSource);
};

}  
}  
#endif  
