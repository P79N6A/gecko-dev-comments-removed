









#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/common_types.h"
#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp.h"
#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp_defines.h"

namespace webrtc {



class LoopBackTransport : public webrtc::Transport {
 public:
  LoopBackTransport()
    : _count(0),
      _packetLoss(0),
      _rtpRtcpModule(NULL) {
  }
  void SetSendModule(RtpRtcp* rtpRtcpModule) {
    _rtpRtcpModule = rtpRtcpModule;
  }
  void DropEveryNthPacket(int n) {
    _packetLoss = n;
  }
  virtual int SendPacket(int channel, const void *data, int len) {
    _count++;
    if (_packetLoss > 0) {
      if ((_count % _packetLoss) == 0) {
        return len;
      }
    }
    if (_rtpRtcpModule->IncomingPacket((const uint8_t*)data, len) == 0) {
      return len;
    }
    return -1;
  }
  virtual int SendRTCPPacket(int channel, const void *data, int len) {
    if (_rtpRtcpModule->IncomingPacket((const uint8_t*)data, len) == 0) {
      return len;
    }
    return -1;
  }
 private:
  int _count;
  int _packetLoss;
  RtpRtcp* _rtpRtcpModule;
};

class RtpReceiver : public RtpData {
 public:
  enum { kMaxPayloadSize = 1500 };

  virtual int32_t OnReceivedPayloadData(
      const uint8_t* payloadData,
      const uint16_t payloadSize,
      const webrtc::WebRtcRTPHeader* rtpHeader) {
    EXPECT_LE(payloadSize, kMaxPayloadSize);
    memcpy(_payloadData, payloadData, payloadSize);
    memcpy(&_rtpHeader, rtpHeader, sizeof(_rtpHeader));
    _payloadSize = payloadSize;
    return 0;
  }

  const uint8_t* payload_data() const {
    return _payloadData;
  }

  uint16_t payload_size() const {
    return _payloadSize;
  }

  webrtc::WebRtcRTPHeader rtp_header() const {
    return _rtpHeader;
  }

 private:
  uint8_t _payloadData[kMaxPayloadSize];
  uint16_t _payloadSize;
  webrtc::WebRtcRTPHeader _rtpHeader;
};

}  
