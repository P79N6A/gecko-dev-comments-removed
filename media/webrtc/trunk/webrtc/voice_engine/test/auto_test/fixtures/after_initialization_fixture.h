









#ifndef SRC_VOICE_ENGINE_MAIN_TEST_AUTO_TEST_STANDARD_TEST_BASE_AFTER_INIT_H_
#define SRC_VOICE_ENGINE_MAIN_TEST_AUTO_TEST_STANDARD_TEST_BASE_AFTER_INIT_H_

#include "before_initialization_fixture.h"
#include "scoped_ptr.h"
#include "webrtc/common_types.h"

class TestErrorObserver;

class LoopBackTransport : public webrtc::Transport {
 public:
  LoopBackTransport(webrtc::VoENetwork* voe_network)
      : voe_network_(voe_network) {
  }

  virtual int SendPacket(int channel, const void *data, int len) {
    voe_network_->ReceivedRTPPacket(channel, data, len);
    return len;
  }

  virtual int SendRTCPPacket(int channel, const void *data, int len) {
    voe_network_->ReceivedRTCPPacket(channel, data, len);
    return len;
  }

 private:
  webrtc::VoENetwork* voe_network_;
};






class AfterInitializationFixture : public BeforeInitializationFixture {
 public:
  AfterInitializationFixture();
  virtual ~AfterInitializationFixture();
 protected:
  webrtc::scoped_ptr<TestErrorObserver> error_observer_;
};

#endif  
