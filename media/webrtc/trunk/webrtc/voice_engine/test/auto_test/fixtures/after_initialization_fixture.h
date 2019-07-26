









#ifndef SRC_VOICE_ENGINE_MAIN_TEST_AUTO_TEST_STANDARD_TEST_BASE_AFTER_INIT_H_
#define SRC_VOICE_ENGINE_MAIN_TEST_AUTO_TEST_STANDARD_TEST_BASE_AFTER_INIT_H_

#include <deque>

#include "webrtc/common_types.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/interface/event_wrapper.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/system_wrappers/interface/thread_wrapper.h"
#include "webrtc/voice_engine/test/auto_test/fixtures/before_initialization_fixture.h"

class TestErrorObserver;

class LoopBackTransport : public webrtc::Transport {
 public:
  LoopBackTransport(webrtc::VoENetwork* voe_network)
      : crit_(webrtc::CriticalSectionWrapper::CreateCriticalSection()),
        packet_event_(webrtc::EventWrapper::Create()),
        thread_(webrtc::ThreadWrapper::CreateThread(NetworkProcess, this)),
        voe_network_(voe_network) {
    unsigned int id;
    thread_->Start(id);
  }

  ~LoopBackTransport() { thread_->Stop(); }

  virtual int SendPacket(int channel, const void* data, int len) {
    StorePacket(Packet::Rtp, channel, data, len);
    return len;
  }

  virtual int SendRTCPPacket(int channel, const void* data, int len) {
    StorePacket(Packet::Rtcp, channel, data, len);
    return len;
  }

 private:
  struct Packet {
    enum Type { Rtp, Rtcp, } type;

    Packet() : len(0) {}
    Packet(Type type, int channel, const void* data, int len)
        : type(type), channel(channel), len(len) {
      assert(len <= 1500);
      memcpy(this->data, data, static_cast<size_t>(len));
    }

    int channel;
    uint8_t data[1500];
    int len;
  };

  void StorePacket(Packet::Type type, int channel, const void* data, int len) {
    webrtc::CriticalSectionScoped lock(crit_.get());
    packet_queue_.push_back(Packet(type, channel, data, len));
    packet_event_->Set();
  }

  static bool NetworkProcess(void* transport) {
    return static_cast<LoopBackTransport*>(transport)->SendPackets();
  }

  bool SendPackets() {
    switch (packet_event_->Wait(10)) {
      case webrtc::kEventSignaled:
        packet_event_->Reset();
        break;
      case webrtc::kEventTimeout:
        break;
      case webrtc::kEventError:
        
        return true;
    }

    while (true) {
      Packet p;
      {
        webrtc::CriticalSectionScoped lock(crit_.get());
        if (packet_queue_.empty())
          break;
        p = packet_queue_.front();
        packet_queue_.pop_front();
      }

      switch (p.type) {
        case Packet::Rtp:
          voe_network_->ReceivedRTPPacket(p.channel, p.data, p.len);
          break;
        case Packet::Rtcp:
          voe_network_->ReceivedRTCPPacket(p.channel, p.data, p.len);
          break;
      }
    }
    return true;
  }

  webrtc::scoped_ptr<webrtc::CriticalSectionWrapper> crit_;
  webrtc::scoped_ptr<webrtc::EventWrapper> packet_event_;
  webrtc::scoped_ptr<webrtc::ThreadWrapper> thread_;
  std::deque<Packet> packet_queue_ GUARDED_BY(crit_.get());
  webrtc::VoENetwork* const voe_network_;
};






class AfterInitializationFixture : public BeforeInitializationFixture {
 public:
  AfterInitializationFixture();
  virtual ~AfterInitializationFixture();

 protected:
  webrtc::scoped_ptr<TestErrorObserver> error_observer_;
};

#endif  
