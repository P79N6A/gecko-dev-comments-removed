








#ifndef VOICE_ENGINE_MAIN_TEST_AUTO_TEST_FAKES_FAKE_EXTERNAL_TRANSPORT_H_
#define VOICE_ENGINE_MAIN_TEST_AUTO_TEST_FAKES_FAKE_EXTERNAL_TRANSPORT_H_

#include "webrtc/common_types.h"

namespace webrtc {
class CriticalSectionWrapper;
class EventWrapper;
class ThreadWrapper;
class VoENetwork;
}

class FakeExternalTransport : public webrtc::Transport {
 public:
  explicit FakeExternalTransport(webrtc::VoENetwork* ptr);
  virtual ~FakeExternalTransport();

  virtual int SendPacket(int channel, const void *data, int len) OVERRIDE;
  virtual int SendRTCPPacket(int channel, const void *data, int len) OVERRIDE;

  void SetDelayStatus(bool enabled, unsigned int delayInMs = 100);

  webrtc::VoENetwork* my_network_;
 private:
  static bool Run(void* ptr);
  bool Process();
 private:
  webrtc::ThreadWrapper* thread_;
  webrtc::CriticalSectionWrapper* lock_;
  webrtc::EventWrapper* event_;
 private:
  unsigned char packet_buffer_[1612];
  int length_;
  int channel_;
  bool delay_is_enabled_;
  int delay_time_in_ms_;
};

#endif  
