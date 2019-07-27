









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_FORMAT_H264_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_FORMAT_H264_H_

#include <queue>
#include <string>

#include "webrtc/modules/rtp_rtcp/source/rtp_format.h"

namespace webrtc {

class RtpPacketizerH264 : public RtpPacketizer {
 public:
  
  
  RtpPacketizerH264(FrameType frame_type, size_t max_payload_len);

  virtual ~RtpPacketizerH264();

  virtual void SetPayloadData(
      const uint8_t* payload_data,
      size_t payload_size,
      const RTPFragmentationHeader* fragmentation) OVERRIDE;

  
  
  
  
  
  
  
  virtual bool NextPacket(uint8_t* buffer,
                          size_t* bytes_to_send,
                          bool* last_packet) OVERRIDE;

  virtual ProtectionType GetProtectionType() OVERRIDE;

  virtual StorageType GetStorageType(uint32_t retransmission_settings) OVERRIDE;

  virtual std::string ToString() OVERRIDE;

 private:
  struct Packet {
    Packet(size_t offset,
           size_t size,
           bool first_fragment,
           bool last_fragment,
           bool aggregated,
           uint8_t header)
        : offset(offset),
          size(size),
          first_fragment(first_fragment),
          last_fragment(last_fragment),
          aggregated(aggregated),
          header(header) {}

    size_t offset;
    size_t size;
    bool first_fragment;
    bool last_fragment;
    bool aggregated;
    uint8_t header;
  };
  typedef std::queue<Packet> PacketQueue;

  void GeneratePackets();
  void PacketizeFuA(size_t fragment_offset, size_t fragment_length);
  int PacketizeStapA(size_t fragment_index,
                     size_t fragment_offset,
                     size_t fragment_length);
  void NextAggregatePacket(uint8_t* buffer, size_t* bytes_to_send);
  void NextFragmentPacket(uint8_t* buffer, size_t* bytes_to_send);

  const uint8_t* payload_data_;
  size_t payload_size_;
  const size_t max_payload_len_;
  RTPFragmentationHeader fragmentation_;
  PacketQueue packets_;
  FrameType frame_type_;

  DISALLOW_COPY_AND_ASSIGN(RtpPacketizerH264);
};


class RtpDepacketizerH264 : public RtpDepacketizer {
 public:
  virtual ~RtpDepacketizerH264() {}

  virtual bool Parse(ParsedPayload* parsed_payload,
                     const uint8_t* payload_data,
                     size_t payload_data_length) OVERRIDE;
};
}  
#endif  
