









#include "webrtc/modules/audio_coding/neteq/tools/rtp_file_source.h"

#include <assert.h>
#include <string.h>
#ifdef WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

#include "webrtc/base/checks.h"
#include "webrtc/modules/audio_coding/neteq/tools/packet.h"
#include "webrtc/modules/rtp_rtcp/interface/rtp_header_parser.h"
#include "webrtc/test/rtp_file_reader.h"

namespace webrtc {
namespace test {

RtpFileSource* RtpFileSource::Create(const std::string& file_name) {
  RtpFileSource* source = new RtpFileSource();
  CHECK(source->OpenFile(file_name));
  return source;
}

RtpFileSource::~RtpFileSource() {
}

bool RtpFileSource::RegisterRtpHeaderExtension(RTPExtensionType type,
                                               uint8_t id) {
  assert(parser_.get());
  return parser_->RegisterRtpHeaderExtension(type, id);
}

Packet* RtpFileSource::NextPacket() {
  while (true) {
    RtpFileReader::Packet temp_packet;
    if (!rtp_reader_->NextPacket(&temp_packet)) {
      return NULL;
    }
    if (temp_packet.length == 0) {
      
      
      continue;
    }
    scoped_ptr<uint8_t[]> packet_memory(new uint8_t[temp_packet.length]);
    memcpy(packet_memory.get(), temp_packet.data, temp_packet.length);
    scoped_ptr<Packet> packet(new Packet(packet_memory.release(),
                                         temp_packet.length,
                                         temp_packet.original_length,
                                         temp_packet.time_ms,
                                         *parser_.get()));
    if (!packet->valid_header()) {
      assert(false);
      return NULL;
    }
    if (filter_.test(packet->header().payloadType) ||
        (use_ssrc_filter_ && packet->header().ssrc != ssrc_)) {
      
      continue;
    }
    return packet.release();
  }
}

RtpFileSource::RtpFileSource()
    : PacketSource(),
      parser_(RtpHeaderParser::Create()) {}

bool RtpFileSource::OpenFile(const std::string& file_name) {
  rtp_reader_.reset(RtpFileReader::Create(RtpFileReader::kRtpDump, file_name));
  if (rtp_reader_)
    return true;
  rtp_reader_.reset(RtpFileReader::Create(RtpFileReader::kPcap, file_name));
  if (!rtp_reader_) {
    FATAL() << "Couldn't open input file as either a rtpdump or .pcap. Note "
               "that .pcapng is not supported.";
  }
  return true;
}

}  
}  
