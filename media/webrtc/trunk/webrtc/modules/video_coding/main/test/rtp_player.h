









#ifndef WEBRTC_MODULES_VIDEO_CODING_TEST_RTP_PLAYER_H_
#define WEBRTC_MODULES_VIDEO_CODING_TEST_RTP_PLAYER_H_

#include <string>
#include <vector>

#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp_defines.h"
#include "webrtc/modules/video_coding/main/interface/video_coding_defines.h"

namespace webrtc {
class Clock;

namespace rtpplayer {

class PayloadCodecTuple {
 public:
  PayloadCodecTuple(uint8_t payload_type, const std::string& codec_name,
                    VideoCodecType codec_type)
      : name_(codec_name),
        payload_type_(payload_type),
        codec_type_(codec_type) {
  }

  const std::string& name() const { return name_; }
  uint8_t payload_type() const { return payload_type_; }
  VideoCodecType codec_type() const { return codec_type_; }

 private:
  std::string name_;
  uint8_t payload_type_;
  VideoCodecType codec_type_;
};

typedef std::vector<PayloadCodecTuple> PayloadTypes;
typedef std::vector<PayloadCodecTuple>::const_iterator PayloadTypesIterator;



class RtpPacketSourceInterface {
 public:
  virtual ~RtpPacketSourceInterface() {}

  
  
  
  
  virtual int NextPacket(uint8_t* rtp_data, uint32_t* length,
                         uint32_t* time_ms) = 0;
};



class RtpStreamInterface {
 public:
  virtual ~RtpStreamInterface() {}

  
  virtual void ResendPackets(const uint16_t* sequence_numbers,
                             uint16_t length) = 0;

  virtual uint32_t ssrc() const = 0;
  virtual const PayloadTypes& payload_types() const = 0;
};


class PayloadSinkInterface : public RtpData {
 public:
  virtual ~PayloadSinkInterface() {}
};



class PayloadSinkFactoryInterface {
 public:
  virtual ~PayloadSinkFactoryInterface() {}

  
  
  
  virtual PayloadSinkInterface* Create(RtpStreamInterface* stream) = 0;
};


class RtpPlayerInterface {
 public:
  virtual ~RtpPlayerInterface() {}

  virtual int NextPacket(int64_t timeNow) = 0;
  virtual uint32_t TimeUntilNextPacket() const = 0;
  virtual void Print() const = 0;
};

RtpPlayerInterface* Create(const std::string& inputFilename,
    PayloadSinkFactoryInterface* payloadSinkFactory, Clock* clock,
    const PayloadTypes& payload_types, float lossRate, uint32_t rttMs,
    bool reordering);

}  
}  

#endif 
