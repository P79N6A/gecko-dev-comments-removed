








#include "webrtc/test/direct_transport.h"

#include "testing/gtest/include/gtest/gtest.h"

#include "webrtc/call.h"
#include "webrtc/system_wrappers/interface/clock.h"

namespace webrtc {
namespace test {

DirectTransport::DirectTransport()
    : lock_(CriticalSectionWrapper::CreateCriticalSection()),
      packet_event_(EventWrapper::Create()),
      thread_(ThreadWrapper::CreateThread(NetworkProcess, this)),
      clock_(Clock::GetRealTimeClock()),
      shutting_down_(false),
      fake_network_(FakeNetworkPipe::Config()) {
  unsigned int thread_id;
  EXPECT_TRUE(thread_->Start(thread_id));
}

DirectTransport::DirectTransport(
    const FakeNetworkPipe::Config& config)
    : lock_(CriticalSectionWrapper::CreateCriticalSection()),
      packet_event_(EventWrapper::Create()),
      thread_(ThreadWrapper::CreateThread(NetworkProcess, this)),
      clock_(Clock::GetRealTimeClock()),
      shutting_down_(false),
      fake_network_(config) {
  unsigned int thread_id;
  EXPECT_TRUE(thread_->Start(thread_id));
}

DirectTransport::~DirectTransport() { StopSending(); }

void DirectTransport::StopSending() {
  {
    CriticalSectionScoped crit_(lock_.get());
    shutting_down_ = true;
  }

  packet_event_->Set();
  EXPECT_TRUE(thread_->Stop());
}

void DirectTransport::SetReceiver(PacketReceiver* receiver) {
  fake_network_.SetReceiver(receiver);
}

bool DirectTransport::SendRtp(const uint8_t* data, size_t length) {
  fake_network_.SendPacket(data, length);
  packet_event_->Set();
  return true;
}

bool DirectTransport::SendRtcp(const uint8_t* data, size_t length) {
  fake_network_.SendPacket(data, length);
  packet_event_->Set();
  return true;
}

bool DirectTransport::NetworkProcess(void* transport) {
  return static_cast<DirectTransport*>(transport)->SendPackets();
}

bool DirectTransport::SendPackets() {
  fake_network_.Process();
  int wait_time_ms = fake_network_.TimeUntilNextProcess();
  if (wait_time_ms > 0) {
    switch (packet_event_->Wait(wait_time_ms)) {
      case kEventSignaled:
        packet_event_->Reset();
        break;
      case kEventTimeout:
        break;
      case kEventError:
        
        return true;
    }
  }
  CriticalSectionScoped crit(lock_.get());
  return shutting_down_ ? false : true;
}
}  
}  
