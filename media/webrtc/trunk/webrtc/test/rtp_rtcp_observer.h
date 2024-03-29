








#ifndef WEBRTC_VIDEO_ENGINE_TEST_COMMON_RTP_RTCP_OBSERVER_H_
#define WEBRTC_VIDEO_ENGINE_TEST_COMMON_RTP_RTCP_OBSERVER_H_

#include <map>
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

#include "webrtc/modules/rtp_rtcp/interface/rtp_header_parser.h"
#include "webrtc/test/direct_transport.h"
#include "webrtc/typedefs.h"
#include "webrtc/video_send_stream.h"

namespace webrtc {
namespace test {

class RtpRtcpObserver {
 public:
  virtual ~RtpRtcpObserver() {}
  newapi::Transport* SendTransport() {
    return &send_transport_;
  }

  newapi::Transport* ReceiveTransport() {
    return &receive_transport_;
  }

  virtual void SetReceivers(PacketReceiver* send_transport_receiver,
                            PacketReceiver* receive_transport_receiver) {
    send_transport_.SetReceiver(send_transport_receiver);
    receive_transport_.SetReceiver(receive_transport_receiver);
  }

  void StopSending() {
    send_transport_.StopSending();
    receive_transport_.StopSending();
  }

  virtual EventTypeWrapper Wait() {
    EventTypeWrapper result = observation_complete_->Wait(timeout_ms_);
    observation_complete_->Reset();
    return result;
  }

 protected:
  RtpRtcpObserver(unsigned int event_timeout_ms,
      const FakeNetworkPipe::Config& configuration)
      : crit_(CriticalSectionWrapper::CreateCriticalSection()),
        observation_complete_(EventWrapper::Create()),
        parser_(RtpHeaderParser::Create()),
        send_transport_(crit_.get(),
                        this,
                        &RtpRtcpObserver::OnSendRtp,
                        &RtpRtcpObserver::OnSendRtcp,
                        configuration),
        receive_transport_(crit_.get(),
                           this,
                           &RtpRtcpObserver::OnReceiveRtp,
                           &RtpRtcpObserver::OnReceiveRtcp,
                           configuration),
        timeout_ms_(event_timeout_ms) {}

  explicit RtpRtcpObserver(unsigned int event_timeout_ms)
      : crit_(CriticalSectionWrapper::CreateCriticalSection()),
        observation_complete_(EventWrapper::Create()),
        parser_(RtpHeaderParser::Create()),
        send_transport_(crit_.get(),
                        this,
                        &RtpRtcpObserver::OnSendRtp,
                        &RtpRtcpObserver::OnSendRtcp,
                        FakeNetworkPipe::Config()),
        receive_transport_(crit_.get(),
                           this,
                           &RtpRtcpObserver::OnReceiveRtp,
                           &RtpRtcpObserver::OnReceiveRtcp,
                           FakeNetworkPipe::Config()),
        timeout_ms_(event_timeout_ms) {}

  enum Action {
    SEND_PACKET,
    DROP_PACKET,
  };

  virtual Action OnSendRtp(const uint8_t* packet, size_t length)
      EXCLUSIVE_LOCKS_REQUIRED(crit_) {
    return SEND_PACKET;
  }

  virtual Action OnSendRtcp(const uint8_t* packet, size_t length)
      EXCLUSIVE_LOCKS_REQUIRED(crit_) {
    return SEND_PACKET;
  }

  virtual Action OnReceiveRtp(const uint8_t* packet, size_t length)
      EXCLUSIVE_LOCKS_REQUIRED(crit_) {
    return SEND_PACKET;
  }

  virtual Action OnReceiveRtcp(const uint8_t* packet, size_t length)
      EXCLUSIVE_LOCKS_REQUIRED(crit_) {
    return SEND_PACKET;
  }

 private:
  class PacketTransport : public test::DirectTransport {
   public:
    typedef Action (RtpRtcpObserver::*PacketTransportAction)(const uint8_t*,
                                                             size_t);

    PacketTransport(CriticalSectionWrapper* lock,
                    RtpRtcpObserver* observer,
                    PacketTransportAction on_rtp,
                    PacketTransportAction on_rtcp,
                    const FakeNetworkPipe::Config& configuration)
        : test::DirectTransport(configuration),
          crit_(lock),
          observer_(observer),
          on_rtp_(on_rtp),
          on_rtcp_(on_rtcp) {}

  private:
    virtual bool SendRtp(const uint8_t* packet, size_t length) OVERRIDE {
      EXPECT_FALSE(RtpHeaderParser::IsRtcp(packet, length));
      Action action;
      {
        CriticalSectionScoped lock(crit_);
        action = (observer_->*on_rtp_)(packet, length);
      }
      switch (action) {
        case DROP_PACKET:
          
          return true;
        case SEND_PACKET:
          return test::DirectTransport::SendRtp(packet, length);
      }
      return true;  
    }

    virtual bool SendRtcp(const uint8_t* packet, size_t length) OVERRIDE {
      EXPECT_TRUE(RtpHeaderParser::IsRtcp(packet, length));
      Action action;
      {
        CriticalSectionScoped lock(crit_);
        action = (observer_->*on_rtcp_)(packet, length);
      }
      switch (action) {
        case DROP_PACKET:
          
          return true;
        case SEND_PACKET:
          return test::DirectTransport::SendRtcp(packet, length);
      }
      return true;  
    }

    
    CriticalSectionWrapper* const crit_;

    RtpRtcpObserver* const observer_;
    const PacketTransportAction on_rtp_, on_rtcp_;
  };

 protected:
  const scoped_ptr<CriticalSectionWrapper> crit_;
  const scoped_ptr<EventWrapper> observation_complete_;
  const scoped_ptr<RtpHeaderParser> parser_;

 private:
  PacketTransport send_transport_, receive_transport_;
  unsigned int timeout_ms_;
};
}  
}  

#endif  
