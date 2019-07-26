









#ifndef WEBRTC_TEST_FAKE_NETWORK_PIPE_H_
#define WEBRTC_TEST_FAKE_NETWORK_PIPE_H_

#include <queue>

#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/system_wrappers/interface/event_wrapper.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class CriticalSectionWrapper;
class NetworkPacket;
class PacketReceiver;






class FakeNetworkPipe {
 public:
  struct Config {
    Config()
        : queue_length(0),
          queue_delay_ms(0),
          delay_standard_deviation_ms(0),
          link_capacity_kbps(0),
          loss_percent(0) {
    }
    
    size_t queue_length;
    
    int queue_delay_ms;
    
    int delay_standard_deviation_ms;
    
    int link_capacity_kbps;
    
    int loss_percent;
  };

  explicit FakeNetworkPipe(const FakeNetworkPipe::Config& config);
  ~FakeNetworkPipe();

  
  void SetReceiver(PacketReceiver* receiver);

  
  void SendPacket(const uint8_t* packet, size_t packet_length);

  
  
  void Process();
  int TimeUntilNextProcess() const;

  
  float PercentageLoss();
  int AverageDelay();
  size_t dropped_packets() { return dropped_packets_; }
  size_t sent_packets() { return sent_packets_; }

 private:
  scoped_ptr<CriticalSectionWrapper> lock_;
  PacketReceiver* packet_receiver_;
  std::queue<NetworkPacket*> capacity_link_;
  std::queue<NetworkPacket*> delay_link_;

  
  Config config_;

  
  size_t dropped_packets_;
  size_t sent_packets_;
  int total_packet_delay_;

  int64_t next_process_time_;

  DISALLOW_COPY_AND_ASSIGN(FakeNetworkPipe);
};

}  

#endif  
