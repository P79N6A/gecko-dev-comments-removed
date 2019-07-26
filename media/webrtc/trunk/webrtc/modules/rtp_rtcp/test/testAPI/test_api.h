









#include "common_types.h"
#include "rtp_rtcp.h"
#include "rtp_rtcp_defines.h"

namespace webrtc {

class FakeRtpRtcpClock : public RtpRtcpClock {
 public:
  FakeRtpRtcpClock() {
    time_in_ms_ = 123456;
  }
  
  
  virtual WebRtc_Word64 GetTimeInMS() {
    return time_in_ms_;
  }
  
  virtual void CurrentNTP(WebRtc_UWord32& secs, WebRtc_UWord32& frac) {
    secs = time_in_ms_ / 1000;
    frac = (time_in_ms_ % 1000) * 4294967;
  }
  void IncrementTime(WebRtc_UWord32 time_increment_ms) {
    time_in_ms_ += time_increment_ms;
  }
 private:
  WebRtc_Word64 time_in_ms_;
};



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
    if (_rtpRtcpModule->IncomingPacket((const WebRtc_UWord8*)data, len) == 0) {
      return len;
    }
    return -1;
  }
  virtual int SendRTCPPacket(int channel, const void *data, int len) {
    if (_rtpRtcpModule->IncomingPacket((const WebRtc_UWord8*)data, len) == 0) {
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

  virtual WebRtc_Word32 OnReceivedPayloadData(
      const WebRtc_UWord8* payloadData,
      const WebRtc_UWord16 payloadSize,
      const webrtc::WebRtcRTPHeader* rtpHeader) {
    EXPECT_LE(payloadSize, kMaxPayloadSize);
    memcpy(_payloadData, payloadData, payloadSize);
    memcpy(&_rtpHeader, rtpHeader, sizeof(_rtpHeader));
    _payloadSize = payloadSize;
    return 0;
  }

  const WebRtc_UWord8* payload_data() const {
    return _payloadData;
  }

  WebRtc_UWord16 payload_size() const {
    return _payloadSize;
  }

  webrtc::WebRtcRTPHeader rtp_header() const {
    return _rtpHeader;
  }

 private:
  WebRtc_UWord8 _payloadData[kMaxPayloadSize];
  WebRtc_UWord16 _payloadSize;
  webrtc::WebRtcRTPHeader _rtpHeader;
};

}  
 
