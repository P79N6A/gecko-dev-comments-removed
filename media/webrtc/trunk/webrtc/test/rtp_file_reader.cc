









#include "webrtc/test/rtp_file_reader.h"

#include <stdio.h>

#include <map>
#include <string>
#include <vector>

#include "webrtc/base/checks.h"
#include "webrtc/modules/rtp_rtcp/source/rtp_utility.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {
namespace test {

static const size_t kFirstLineLength = 40;
static uint16_t kPacketHeaderSize = 8;

#if 1
# define DEBUG_LOG(text)
# define DEBUG_LOG1(text, arg)
#else
# define DEBUG_LOG(text) (printf(text "\n"))
# define DEBUG_LOG1(text, arg) (printf(text "\n", arg))
#endif

#define TRY(expr)                                      \
  do {                                                 \
    if (!(expr)) {                                     \
      DEBUG_LOG1("FAIL at " __FILE__ ":%d", __LINE__); \
      return false;                                    \
    }                                                  \
  } while (0)

class RtpFileReaderImpl : public RtpFileReader {
 public:
  virtual bool Init(const std::string& filename) = 0;
};



class RtpDumpReader : public RtpFileReaderImpl {
 public:
  RtpDumpReader() : file_(NULL) {}
  virtual ~RtpDumpReader() {
    if (file_ != NULL) {
      fclose(file_);
      file_ = NULL;
    }
  }

  bool Init(const std::string& filename) {
    file_ = fopen(filename.c_str(), "rb");
    if (file_ == NULL) {
      printf("ERROR: Can't open file: %s\n", filename.c_str());
      return false;
    }

    char firstline[kFirstLineLength + 1] = {0};
    if (fgets(firstline, kFirstLineLength, file_) == NULL) {
      DEBUG_LOG("ERROR: Can't read from file\n");
      return false;
    }
    if (strncmp(firstline, "#!rtpplay", 9) == 0) {
      if (strncmp(firstline, "#!rtpplay1.0", 12) != 0) {
        DEBUG_LOG("ERROR: wrong rtpplay version, must be 1.0\n");
        return false;
      }
    } else if (strncmp(firstline, "#!RTPencode", 11) == 0) {
      if (strncmp(firstline, "#!RTPencode1.0", 14) != 0) {
        DEBUG_LOG("ERROR: wrong RTPencode version, must be 1.0\n");
        return false;
      }
    } else {
      DEBUG_LOG("ERROR: wrong file format of input file\n");
      return false;
    }

    uint32_t start_sec;
    uint32_t start_usec;
    uint32_t source;
    uint16_t port;
    uint16_t padding;
    TRY(Read(&start_sec));
    TRY(Read(&start_usec));
    TRY(Read(&source));
    TRY(Read(&port));
    TRY(Read(&padding));

    return true;
  }

  virtual bool NextPacket(Packet* packet) OVERRIDE {
    uint8_t* rtp_data = packet->data;
    packet->length = Packet::kMaxPacketBufferSize;

    uint16_t len;
    uint16_t plen;
    uint32_t offset;
    TRY(Read(&len));
    TRY(Read(&plen));
    TRY(Read(&offset));

    
    len -= kPacketHeaderSize;
    if (packet->length < len) {
      FATAL() << "Packet is too large to fit: " << len << " bytes vs "
              << packet->length
              << " bytes allocated. Consider increasing the buffer "
                 "size";
    }
    if (fread(rtp_data, 1, len, file_) != len) {
      return false;
    }

    packet->length = len;
    packet->original_length = plen;
    packet->time_ms = offset;
    return true;
  }

 private:
  bool Read(uint32_t* out) {
    *out = 0;
    for (size_t i = 0; i < 4; ++i) {
      *out <<= 8;
      uint8_t tmp;
      if (fread(&tmp, 1, sizeof(uint8_t), file_) != sizeof(uint8_t))
        return false;
      *out |= tmp;
    }
    return true;
  }

  bool Read(uint16_t* out) {
    *out = 0;
    for (size_t i = 0; i < 2; ++i) {
      *out <<= 8;
      uint8_t tmp;
      if (fread(&tmp, 1, sizeof(uint8_t), file_) != sizeof(uint8_t))
        return false;
      *out |= tmp;
    }
    return true;
  }

  FILE* file_;

  DISALLOW_COPY_AND_ASSIGN(RtpDumpReader);
};

enum {
  kResultFail = -1,
  kResultSuccess = 0,
  kResultSkip = 1,

  kPcapVersionMajor = 2,
  kPcapVersionMinor = 4,
  kLinktypeNull = 0,
  kLinktypeEthernet = 1,
  kBsdNullLoopback1 = 0x00000002,
  kBsdNullLoopback2 = 0x02000000,
  kEthernetIIHeaderMacSkip = 12,
  kEthertypeIp = 0x0800,
  kIpVersion4 = 4,
  kMinIpHeaderLength = 20,
  kFragmentOffsetClear = 0x0000,
  kFragmentOffsetDoNotFragment = 0x4000,
  kProtocolTcp = 0x06,
  kProtocolUdp = 0x11,
  kUdpHeaderLength = 8,
  kMaxReadBufferSize = 4096
};

const uint32_t kPcapBOMSwapOrder = 0xd4c3b2a1UL;
const uint32_t kPcapBOMNoSwapOrder = 0xa1b2c3d4UL;

#define TRY_PCAP(expr)                                 \
  do {                                                 \
    int r = (expr);                                    \
    if (r == kResultFail) {                            \
      DEBUG_LOG1("FAIL at " __FILE__ ":%d", __LINE__); \
      return kResultFail;                              \
    } else if (r == kResultSkip) {                     \
      return kResultSkip;                              \
    }                                                  \
  } while (0)



class PcapReader : public RtpFileReaderImpl {
 public:
  PcapReader()
    : file_(NULL),
      swap_pcap_byte_order_(false),
#ifdef WEBRTC_ARCH_BIG_ENDIAN
      swap_network_byte_order_(false),
#else
      swap_network_byte_order_(true),
#endif
      read_buffer_(),
      packets_by_ssrc_(),
      packets_(),
      next_packet_it_() {
  }

  virtual ~PcapReader() {
    if (file_ != NULL) {
      fclose(file_);
      file_ = NULL;
    }
  }

  bool Init(const std::string& filename) OVERRIDE {
    return Initialize(filename) == kResultSuccess;
  }

  int Initialize(const std::string& filename) {
    file_ = fopen(filename.c_str(), "rb");
    if (file_ == NULL) {
      printf("ERROR: Can't open file: %s\n", filename.c_str());
      return kResultFail;
    }

    if (ReadGlobalHeader() < 0) {
      return kResultFail;
    }

    int total_packet_count = 0;
    uint32_t stream_start_ms = 0;
    int32_t next_packet_pos = ftell(file_);
    for (;;) {
      TRY_PCAP(fseek(file_, next_packet_pos, SEEK_SET));
      int result = ReadPacket(&next_packet_pos, stream_start_ms,
                              ++total_packet_count);
      if (result == kResultFail) {
        break;
      } else if (result == kResultSuccess && packets_.size() == 1) {
        assert(stream_start_ms == 0);
        PacketIterator it = packets_.begin();
        stream_start_ms = it->time_offset_ms;
        it->time_offset_ms = 0;
      }
    }

    if (feof(file_) == 0) {
      printf("Failed reading file!\n");
      return kResultFail;
    }

    printf("Total packets in file: %d\n", total_packet_count);
    printf("Total RTP/RTCP packets: %d\n", static_cast<int>(packets_.size()));

    for (SsrcMapIterator mit = packets_by_ssrc_.begin();
        mit != packets_by_ssrc_.end(); ++mit) {
      uint32_t ssrc = mit->first;
      const std::vector<uint32_t>& packet_numbers = mit->second;
      uint8_t pt = packets_[packet_numbers[0]].rtp_header.payloadType;
      printf("SSRC: %08x, %d packets, pt=%d\n", ssrc,
             static_cast<int>(packet_numbers.size()), pt);
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    next_packet_it_ = packets_.begin();
    return kResultSuccess;
  }

  virtual bool NextPacket(Packet* packet) OVERRIDE {
    uint32_t length = Packet::kMaxPacketBufferSize;
    if (NextPcap(packet->data, &length, &packet->time_ms) != kResultSuccess)
      return false;
    packet->length = static_cast<size_t>(length);
    packet->original_length = packet->length;
    return true;
  }

  virtual int NextPcap(uint8_t* data, uint32_t* length, uint32_t* time_ms) {
    assert(data);
    assert(length);
    assert(time_ms);

    if (next_packet_it_ == packets_.end()) {
      return -1;
    }
    if (*length < next_packet_it_->payload_length) {
      return -1;
    }
    TRY_PCAP(fseek(file_, next_packet_it_->pos_in_file, SEEK_SET));
    TRY_PCAP(Read(data, next_packet_it_->payload_length));
    *length = next_packet_it_->payload_length;
    *time_ms = next_packet_it_->time_offset_ms;
    next_packet_it_++;

    return 0;
  }

 private:
  
  struct RtpPacketMarker {
    uint32_t packet_number;   
    uint32_t time_offset_ms;
    uint32_t source_ip;
    uint32_t dest_ip;
    uint16_t source_port;
    uint16_t dest_port;
    RTPHeader rtp_header;
    int32_t pos_in_file;      
    uint32_t payload_length;
  };

  typedef std::vector<RtpPacketMarker>::iterator PacketIterator;
  typedef std::map<uint32_t, std::vector<uint32_t> > SsrcMap;
  typedef std::map<uint32_t, std::vector<uint32_t> >::iterator SsrcMapIterator;

  int ReadGlobalHeader() {
    uint32_t magic;
    TRY_PCAP(Read(&magic, false));
    if (magic == kPcapBOMSwapOrder) {
      swap_pcap_byte_order_ = true;
    } else if (magic == kPcapBOMNoSwapOrder) {
      swap_pcap_byte_order_ = false;
    } else {
      return kResultFail;
    }

    uint16_t version_major;
    uint16_t version_minor;
    TRY_PCAP(Read(&version_major, false));
    TRY_PCAP(Read(&version_minor, false));
    if (version_major != kPcapVersionMajor ||
        version_minor != kPcapVersionMinor) {
      return kResultFail;
    }

    int32_t this_zone;  
    uint32_t sigfigs;   
    uint32_t snaplen;   
    uint32_t network;   
    TRY_PCAP(Read(&this_zone, false));
    TRY_PCAP(Read(&sigfigs, false));
    TRY_PCAP(Read(&snaplen, false));
    TRY_PCAP(Read(&network, false));

    
    
    if (network != kLinktypeNull && network != kLinktypeEthernet) {
      return kResultFail;
    }

    return kResultSuccess;
  }

  int ReadPacket(int32_t* next_packet_pos, uint32_t stream_start_ms,
                 uint32_t number) {
    assert(next_packet_pos);

    uint32_t ts_sec;    
    uint32_t ts_usec;   
    uint32_t incl_len;  
    uint32_t orig_len;  
    TRY_PCAP(Read(&ts_sec, false));
    TRY_PCAP(Read(&ts_usec, false));
    TRY_PCAP(Read(&incl_len, false));
    TRY_PCAP(Read(&orig_len, false));

    *next_packet_pos = ftell(file_) + incl_len;

    RtpPacketMarker marker = {0};
    marker.packet_number = number;
    marker.time_offset_ms = CalcTimeDelta(ts_sec, ts_usec, stream_start_ms);
    TRY_PCAP(ReadPacketHeader(&marker));
    marker.pos_in_file = ftell(file_);

    if (marker.payload_length > sizeof(read_buffer_)) {
      printf("Packet too large!\n");
      return kResultFail;
    }
    TRY_PCAP(Read(read_buffer_, marker.payload_length));

    RtpUtility::RtpHeaderParser rtp_parser(read_buffer_, marker.payload_length);
    if (rtp_parser.RTCP()) {
      rtp_parser.ParseRtcp(&marker.rtp_header);
      packets_.push_back(marker);
    } else {
      if (!rtp_parser.Parse(marker.rtp_header, NULL)) {
        DEBUG_LOG("Not recognized as RTP/RTCP");
        return kResultSkip;
      }

      uint32_t ssrc = marker.rtp_header.ssrc;
      packets_by_ssrc_[ssrc].push_back(marker.packet_number);
      packets_.push_back(marker);
    }

    return kResultSuccess;
  }

  int ReadPacketHeader(RtpPacketMarker* marker) {
    int32_t file_pos = ftell(file_);

    
    
    
    
    uint32_t protocol;
    TRY_PCAP(Read(&protocol, true));
    if (protocol == kBsdNullLoopback1 || protocol == kBsdNullLoopback2) {
      int result = ReadXxpIpHeader(marker);
      DEBUG_LOG("Recognized loopback frame");
      if (result != kResultSkip) {
        return result;
      }
    }

    TRY_PCAP(fseek(file_, file_pos, SEEK_SET));

    
    uint16_t type;
    TRY_PCAP(Skip(kEthernetIIHeaderMacSkip));  
    TRY_PCAP(Read(&type, true));
    if (type == kEthertypeIp) {
      int result = ReadXxpIpHeader(marker);
      DEBUG_LOG("Recognized ethernet 2 frame");
      if (result != kResultSkip) {
        return result;
      }
    }

    return kResultSkip;
  }

  uint32_t CalcTimeDelta(uint32_t ts_sec, uint32_t ts_usec, uint32_t start_ms) {
    
    uint64_t t2_ms = ((static_cast<uint64_t>(ts_sec) * 1000000) + ts_usec +
        500) / 1000;
    uint64_t t1_ms = static_cast<uint64_t>(start_ms);
    if (t2_ms < t1_ms) {
      return 0;
    } else {
      return t2_ms - t1_ms;
    }
  }

  int ReadXxpIpHeader(RtpPacketMarker* marker) {
    assert(marker);

    uint16_t version;
    uint16_t length;
    uint16_t id;
    uint16_t fragment;
    uint16_t protocol;
    uint16_t checksum;
    TRY_PCAP(Read(&version, true));
    TRY_PCAP(Read(&length, true));
    TRY_PCAP(Read(&id, true));
    TRY_PCAP(Read(&fragment, true));
    TRY_PCAP(Read(&protocol, true));
    TRY_PCAP(Read(&checksum, true));
    TRY_PCAP(Read(&marker->source_ip, true));
    TRY_PCAP(Read(&marker->dest_ip, true));

    if (((version >> 12) & 0x000f) != kIpVersion4) {
      DEBUG_LOG("IP header is not IPv4");
      return kResultSkip;
    }

    if (fragment != kFragmentOffsetClear &&
        fragment != kFragmentOffsetDoNotFragment) {
      DEBUG_LOG("IP fragments cannot be handled");
      return kResultSkip;
    }

    
    uint16_t header_length = (version & 0x0f00) >> (8 - 2);
    assert(header_length >= kMinIpHeaderLength);
    TRY_PCAP(Skip(header_length - kMinIpHeaderLength));

    protocol = protocol & 0x00ff;
    if (protocol == kProtocolTcp) {
      DEBUG_LOG("TCP packets are not handled");
      return kResultSkip;
    } else if (protocol == kProtocolUdp) {
      uint16_t length;
      uint16_t checksum;
      TRY_PCAP(Read(&marker->source_port, true));
      TRY_PCAP(Read(&marker->dest_port, true));
      TRY_PCAP(Read(&length, true));
      TRY_PCAP(Read(&checksum, true));
      marker->payload_length = length - kUdpHeaderLength;
    } else {
      DEBUG_LOG("Unknown transport (expected UDP or TCP)");
      return kResultSkip;
    }

    return kResultSuccess;
  }

  int Read(uint32_t* out, bool expect_network_order) {
    uint32_t tmp = 0;
    if (fread(&tmp, 1, sizeof(uint32_t), file_) != sizeof(uint32_t)) {
      return kResultFail;
    }
    if ((!expect_network_order && swap_pcap_byte_order_) ||
        (expect_network_order && swap_network_byte_order_)) {
      tmp = ((tmp >> 24) & 0x000000ff) | (tmp << 24) |
          ((tmp >> 8) & 0x0000ff00) | ((tmp << 8) & 0x00ff0000);
    }
    *out = tmp;
    return kResultSuccess;
  }

  int Read(uint16_t* out, bool expect_network_order) {
    uint16_t tmp = 0;
    if (fread(&tmp, 1, sizeof(uint16_t), file_) != sizeof(uint16_t)) {
      return kResultFail;
    }
    if ((!expect_network_order && swap_pcap_byte_order_) ||
        (expect_network_order && swap_network_byte_order_)) {
      tmp = ((tmp >> 8) & 0x00ff) | (tmp << 8);
    }
    *out = tmp;
    return kResultSuccess;
  }

  int Read(uint8_t* out, uint32_t count) {
    if (fread(out, 1, count, file_) != count) {
      return kResultFail;
    }
    return kResultSuccess;
  }

  int Read(int32_t* out, bool expect_network_order) {
    int32_t tmp = 0;
    if (fread(&tmp, 1, sizeof(uint32_t), file_) != sizeof(uint32_t)) {
      return kResultFail;
    }
    if ((!expect_network_order && swap_pcap_byte_order_) ||
        (expect_network_order && swap_network_byte_order_)) {
      tmp = ((tmp >> 24) & 0x000000ff) | (tmp << 24) |
          ((tmp >> 8) & 0x0000ff00) | ((tmp << 8) & 0x00ff0000);
    }
    *out = tmp;
    return kResultSuccess;
  }

  int Skip(uint32_t length) {
    if (fseek(file_, length, SEEK_CUR) != 0) {
      return kResultFail;
    }
    return kResultSuccess;
  }

  FILE* file_;
  bool swap_pcap_byte_order_;
  const bool swap_network_byte_order_;
  uint8_t read_buffer_[kMaxReadBufferSize];

  SsrcMap packets_by_ssrc_;
  std::vector<RtpPacketMarker> packets_;
  PacketIterator next_packet_it_;

  DISALLOW_COPY_AND_ASSIGN(PcapReader);
};

RtpFileReader* RtpFileReader::Create(FileFormat format,
                                     const std::string& filename) {
  RtpFileReaderImpl* reader = NULL;
  switch (format) {
    case kPcap:
      reader = new PcapReader();
      break;
    case kRtpDump:
      reader = new RtpDumpReader();
      break;
  }
  if (!reader->Init(filename)) {
    delete reader;
    return NULL;
  }
  return reader;
}

}  
}  
